/*
 * tag_beacon_machine.c
 *
 *  This module runs from Tag Main Machine ISR context.
 *
 *  It is responsible for receiving messages from other modules, prioritizing
 *  and sending events to BLE Manager module.
 *
 */

#include "stdio.h"
#include "tag_defines.h"
#include "dbg_utils.h"
#include "tag_sw_timer.h"
#include "lf_machine.h"
#include "tag_status_machine.h"
#include "temperature_machine.h"
#include "ble_manager_machine.h"
#include "tag_main_machine.h"
#include "tag_beacon_machine.h"


//******************************************************************************
// Defines
//******************************************************************************
#define TBM_MAX_EVENTS                        (31)

#define TBM_FAST_BEACON_RATE_SEC              (12)                              //! When tag is in certain states (e.g Staying In Field, Tamper Alert, Man Down, etc)
#define TBM_FAST_BEACON_RATE_RELOAD           ((TBM_FAST_BEACON_RATE_SEC * 1000) / TMM_DEFAULT_TIMER_PERIOD_MS)

#define TBM_DEFAULT_BEACON_RATE_SEC           (600)                             //! Default is 10 minutes
#define TBM_DEFAULT_BEACON_RATE_RELOAD        ((TBM_DEFAULT_BEACON_RATE_SEC * 1000) / TMM_DEFAULT_TIMER_PERIOD_MS)

//******************************************************************************
// Data types
//******************************************************************************
typedef struct tbm_status_t {
    tbm_beacon_events_t events;
    uint32_t event_async_flag;
} tbm_status_t;

//******************************************************************************
// Global variables
//******************************************************************************
static tag_sw_timer_t tbm_beacon_timer;
//TODO this variable is DEPRECATED
static volatile tbm_beacon_events_t event_flags;
static volatile tbm_status_t tbm_status;

//******************************************************************************
// Static functions
//******************************************************************************
//! @brief Get event flag
//! @param value between 0 to TBM_MAX_EVENTS
//! @return 0 if event is not pending, or the event itself otherwise.
static tbm_beacon_events_t tbm_get_event(uint8_t bit)
{
    return (tbm_status.events & (1 << bit));
}

static tbm_beacon_events_t tbm_get_async_event(uint8_t bit)
{
    return (tbm_status.event_async_flag & (1 << bit));
}

static void tbm_clear_event(tbm_beacon_events_t event)
{
    tbm_status.events &= ~(event);
    tbm_status.event_async_flag &= ~(event);
}

static void tbm_send_tag_status_beacon(void)
{
    uint32_t status;
    volatile uint8_t i = 0;

    if (!bmm_queue_is_full()) {
        tsm_tag_status_beacon_t *data = tsm_get_tag_status_beacon_data();
        bmm_msg_t msg;
        msg.msg_type = BLE_MSG_TAG_STATUS;
        msg.msg_data[i++] = data->fw_rev[0];
        msg.msg_data[i++] = data->fw_rev[1];
        msg.msg_data[i++] = data->fw_rev[2];
        msg.msg_data[i++] = data->fw_rev[3];
        msg.msg_data[i++] = data->tag_status.last_reset_cause;
        msg.msg_data[i++] = data->tag_status.flags[1];
        msg.msg_data[i++] = data->tag_status.flags[0];
        msg.length = i + 1;

        status = bmm_enqueue_msg(&msg);

        if (status == 0) {
            //! Message is queued, we can now clear this event.
            tbm_clear_event(TBM_STATUS_EVT);
            DEBUG_LOG(DBG_CAT_TAG_BEACON, COLOR_BG_BLUE COLOR_B_CYAN "Tag Status Beacon" COLOR_RST " was queued...");
        } else {
            DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM queue returned an error!");
        }
    }
}

static void tbm_send_lf_beacon(void)
{
    uint32_t status;
    volatile uint8_t i = 0;

    if (!bmm_queue_is_full()) {
        lfm_lf_beacon_t *data = lfm_get_beacon_data();
        bmm_msg_t msg;
        msg.msg_type = BLE_MSG_LF;
        msg.msg_data[i++] = data->lf_event;
        msg.msg_data[i++] = (data->lf_data.id >> 8);
        msg.msg_data[i++] = (data->lf_data.id);
        msg.msg_data[i++] = data->lf_data.command;
        msg.length = i + 1;

        status = bmm_enqueue_msg(&msg);

        if (status == 0) {
            //! Message is queued, we can now clear this event.
            tbm_clear_event(TBM_LF_EVT);
            DEBUG_LOG(DBG_CAT_TAG_BEACON, COLOR_BG_BLUE COLOR_B_CYAN "LF Beacon" COLOR_RST " was queued...");
        } else {
            DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM queue returned an error!");
        }
    }
}

static void tbm_send_temperature_beacon(void)
{
    uint32_t status;
    volatile uint8_t i = 0;

    if (!bmm_queue_is_full()) {
        bmm_msg_t msg;
        msg.msg_type = BLE_MSG_TEMPERATURE;
        msg.msg_data[i++] = (uint8_t)ttm_get_current_temperature();
        msg.length = i + 1;

        status = bmm_enqueue_msg(&msg);

        if (status == 0) {
            //! Message is queued, we can now clear this event.
            tbm_clear_event(TBM_TEMPERATURE_EVT);
            DEBUG_LOG(DBG_CAT_TAG_BEACON, COLOR_BG_BLUE COLOR_B_CYAN "Tag Temperature Beacon" COLOR_RST " was queued...");
        } else {
            DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM queue returned an error...");
        }
    } else {
        DEBUG_LOG(DBG_CAT_TAG_BEACON, "Error! BMM queue is full...");
    }

}

static void tbm_send_simple_beacon(void)
{
    uint32_t status;

    if (!bmm_queue_is_full()) {
        bmm_msg_t msg = {
           .length = 2,
           .msg_data = {0},
           .msg_type = BLE_MSG_SIMPLE_BEACON
        };

        status = bmm_enqueue_msg(&msg);

        if (status == 0) {
            DEBUG_LOG(DBG_CAT_TAG_BEACON, COLOR_BG_BLUE COLOR_B_CYAN "Simple beacon" COLOR_RST " was queued...");
        } else {
            DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM queue returned an error...");
        }
    } else {
        DEBUG_LOG(DBG_CAT_TAG_BEACON, "Error! BMM queue is full...");
    }

}

#if 0
static void tbm_send_cmd_ack_beacon()
{
    //to be implemented
}

static void tbm_send_battery_beacon()
{
    //to be implemented
}

static void tbm_send_tag_status_beacon()
{
    //to be implemented
}

//static void tbm_send_mandown_beacon();
//static void tbm_send_button_beacon();
//static void tbm_send_motion_beacon();
#endif

//! @brief Decode pending events, Prioritize messages, Sends messages to BLE Manager Queue
static void tbm_decode_event(tbm_beacon_events_t event)
{
    if (event != 0) {
        switch (event) {
            case TBM_LF_EVT:
                tbm_send_lf_beacon();
                break;

            case TBM_TEMPERATURE_EVT:
                tbm_send_temperature_beacon();
                break;

            case TBM_STATUS_EVT:
                tbm_send_tag_status_beacon();
                break;

            case TBM_BATTERY_EVT:
                //tbm_send_battery_beacon();
                break;

            case TBM_CMD_ACK_EVT:
                //tbm_send_cmd_ack_beacon();
                break;

#if 0 //! Not implemented for UT3
            case TBM_MAN_DOWN_EVT:
                //tbm_send_mandown_beacon();
                break;

            case TBM_BUTTON_EVT:
                //tbm_send_button_beacon();
                break;

            case TBM_MOTION_EVT:
                //tbm_send_motion_beacon();
                break;
#endif
            default:
                break;
        }
    }
}


//******************************************************************************
// Non Static functions
//******************************************************************************

//! @brief Sets an event flag for Tag Beacon Machine
//! @details Sets a beacon event flag (we are not expecting queuing for beacon events)
//! So if a certain event is triggered multiple times between Tag Beacon Machine
//! Process period, only 1 event is registered. Each machine is supposed to
//! implement their own queues (if necessary).
//!
//! There might be certain types of events that needs to queue so we do not
//! miss them. Due to the Tag Main Machine period duration some events may
//! need to be handled faster or implement queues. Luckily for tag applications
//! we do not have many of these types of events. Maybe one example would be button
//! press intervals. If you press push button 2x between a Tag Main Process
//! tick it wont register 2 events but one. This is actually desirable to
//! avoid users from abusing the resources. We need to make sure to send
//! the button beacon event but the level of responsiveness VS battery life
//! needs to be taken into account.
void tbm_set_event(tbm_beacon_events_t event, bool is_async)
{
    //! TODO Debug to check if there is any change this approach could drop async messages.
    //! In theory all async messages are critical and need to be transmitted.
    //! Async messages are less critical, they usually report current states while async report transitions.

    tbm_status.events |= event;
    if (is_async) {
        tbm_status.event_async_flag |= event;
    } else {
        tbm_status.event_async_flag &= ~event;
    }
}

//!  @brief Tag Beacon Machine
void tag_beacon_run(void)
{
    //! we only process this machine if BLE Manager Transmission Queue is empty.
    //! otherwise, we just wait another tick to process more messages. Normally the
    //! queue will be emptied on the same tick but there might be cases we may
    //! accumulate multiple messages in BLE Manager queue that exceed ADV_PDU max size.
    //! In that case BLE Manager will require more ticks to transmit all the messages.

    if (bmm_queue_is_empty()) {
        int8_t bit;
        tag_sw_timer_tick(&tbm_beacon_timer);

        //! First check for beacon events with asynchronous flag, they are higher priority
        //! than synchronous beacons and must be transmitted first.
        if (tbm_status.event_async_flag != 0) {
            for (bit = 0; bit < TBM_MAX_EVENTS; bit++) {
                tbm_beacon_events_t event = tbm_get_async_event(bit);
                if (event) {
                    tbm_decode_event(event);
                }
            }
        }

        //! Then check for beacons events that are synchronized to Tag Beacon Machine timer.
        if (tag_sw_timer_is_expired(&tbm_beacon_timer)) {
            //! Check synchronous pending beacon event
            if (tbm_status.events != 0) {
                for (bit = 0; bit < TBM_MAX_EVENTS; bit++) {
                    tbm_beacon_events_t event = tbm_get_event(bit);
                    if (event) {
                        tbm_decode_event(event);
                    }
                }
            } else {
                //! If none of the modules had beacon event to send then we just send a simple plain beacon message
                tbm_send_simple_beacon();
            }
            //! And reload Tag Beacon Machine timer based on module status.
            //! Some modules can change the base beacon rate interval of the synchronous
            //! beacons. For example when "Staying in Field" tag beacons every 12 seconds
            //! and when Tag is not "Staying in field" then the base beacon rate changes
            //! to 10 minutes. Another example would be "Tag in Motion", we usually beacon
            //! every 12 seconds while in that state. If not than we can return to 10 minutes
            //! beacon rate.

            //! So, here we can check for module status and decide what is the beacon rate
            //! we want to keep for the synchronous beacons events.
            /*
            if (lfm_get_lf_status() && LFM_STAYING_IN_FIELD_FLAG) {
                //! 12 seconds for next synchronous beacon event.
            } else {
                //! 10 minutes for next synchronous beacon event.
                tag_sw_timer_reload(&tbm_beacon_timer, TBM_DEFAULT_BEACON_RATE);
            }
            */
            tag_sw_timer_reload(&tbm_beacon_timer, TBM_FAST_BEACON_RATE_RELOAD);

        }
    }
}

uint32_t tbm_init(void)
{
    //! We init globals here (this will be called when Tag Main Machine Timer
    //! gets started.
    tbm_status.event_async_flag = 0;
    tbm_status.events = 0;

    //! Initialize TBM timer to 2 seconds so we dont have to wait too much for the
    //! first beacons when reset or POR occurs.
    tag_sw_timer_reload(&tbm_beacon_timer, ((2*1000)/TMM_DEFAULT_TIMER_PERIOD_MS));
    return 0;
}
