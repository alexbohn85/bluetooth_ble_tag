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
static volatile tbm_status_t tbm_status;

//******************************************************************************
// Static functions
//******************************************************************************
char* tbm_get_event_name(tbm_beacon_events_t event)
{
    switch(event) {

        case TBM_TAG_STATUS_EVT:
            return "TBM_TAG_STATUS_EVT";

        case TBM_BUTTON_EVT:
            return "TBM_BUTTON_EVT";

        case TBM_LF_EVT:
            return "TBM_LF_EVT";

        case TBM_FALL_DETECT_EVT:
            return "TBM_FALL_DETECT_EVT";

        case TBM_MOTION_EVT:
            return "TBM_MOTION_EVT";

        case TBM_FIRMWARE_REV_EVT:
            return "TBM_FIRMWARE_REV_EVT";

        case TBM_EXT_STATUS_EVT:
            return "TBM_EXT_STATUS_EVT";

        case TBM_TAMPER_EVT:
            return "TBM_TAMPER_EVT";

        case TBM_TEMPERATURE_EVT:
            return "TBM_TEMPERATURE_EVT";

        case TBM_UPTIME_EVT:
            return "TBM_UPTIME_EVT";

        case TBM_CMD_ACK_EVT:
            return "TBM_CMD_ACK_EVT";

        default:
            return "Unknown Tag Beacon Type";
            break;
    }
}

/**
 * @brief Get sync beacon events
 * @param bit (event bit)
 * @return 0 if event is not pending, return event bit itself otherwise.
 */
static tbm_beacon_events_t tbm_get_sync_event(uint8_t bit)
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

/**
 * @brief Enqueue message and clear event
 * @param bmm_msg_t*
 * @param tbm_beacon_events_t
 */
static uint32_t tbm_dispatch_msg(bmm_msg_t* msg, tbm_beacon_events_t event)
{
    uint32_t status;

    status = bmm_enqueue_msg(msg);

    if (status == 0) {
        // Message was enqueued, we can now clear this beacon event.
        tbm_clear_event(event);
        DEBUG_LOG(DBG_CAT_TAG_BEACON, COLOR_BG_BLUE COLOR_B_CYAN "%s" COLOR_RST " beacon message was queued...", tbm_get_event_name(event));
    } else {
        status = 1;
    }

    return status;
}

static void tbm_send_firmware_rev_beacon(void)
{
    uint32_t status;
    volatile uint8_t i = 0;

    if (!bmm_queue_is_full()) {
        tsm_tag_ext_status_beacon_t *data = tsm_get_tag_ext_status_beacon_data();
        bmm_msg_t msg;
        msg.type = BLE_MSG_TAG_EXT_STATUS;
        msg.data[i++] = data->fw_rev[0];  /* most significant digit of FW rev format */
        msg.data[i++] = data->fw_rev[1];
        msg.data[i++] = data->fw_rev[2];
        msg.data[i++] = data->fw_rev[3];
        msg.length = i + 1;

        // Send msg to BLE Manager queue
        status = tbm_dispatch_msg(&msg, TBM_EXT_STATUS_EVT);

/*
        status = bmm_enqueue_msg(&msg);
        if (status == 0) {
            //Message is queued, we can now clear this event.
            tbm_clear_event(TBM_EXT_STATUS_EVT);
            DEBUG_LOG(DBG_CAT_TAG_BEACON, COLOR_BG_BLUE COLOR_B_CYAN "Tag Extended Status Beacon" COLOR_RST " was queued...");
        } else {
            DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM queue returned an error!");
        }
*/
    } else {
        DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM message queue is full");
    }
}

static void tbm_send_tag_ext_status_beacon(void)
{
    uint32_t status;
    volatile uint8_t i = 0;

    if (!bmm_queue_is_full()) {
        tsm_tag_ext_status_beacon_t *data = tsm_get_tag_ext_status_beacon_data();
        bmm_msg_t msg;
        msg.type = BLE_MSG_TAG_EXT_STATUS;
        msg.data[i++] = data->ext_status.byte;
        msg.data[i++] = data->fw_rev[0];
        msg.data[i++] = data->fw_rev[1];
        msg.data[i++] = data->fw_rev[2];
        msg.data[i++] = data->fw_rev[3];
        msg.length = i + 1;

        status = bmm_enqueue_msg(&msg);

        if (status == 0) {
            // Message is queued, we can now clear this event.
            tbm_clear_event(TBM_EXT_STATUS_EVT);
            DEBUG_LOG(DBG_CAT_TAG_BEACON, COLOR_BG_BLUE COLOR_B_CYAN "Tag Extended Status Beacon" COLOR_RST " was queued...");
        } else {
            DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM queue returned an error!");
        }

    } else {
        DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM message queue is full");
    }

}

static void tbm_send_tag_status_beacon(void)
{
    uint32_t status;
    volatile uint8_t i = 0;

    if (!bmm_queue_is_full()) {
        tsm_tag_status_beacon_t *data = tsm_get_tag_status_beacon_data();
        bmm_msg_t msg;
        msg.type = BLE_MSG_TAG_STATUS;
        msg.data[i++] = data->status.byte;
        msg.length = i + 1;

        tbm_dispatch_msg(&msg, (TBM_MOTION_EVT | TBM_TAMPER_EVT));

/*
        status = bmm_enqueue_msg(&msg);
        if (status == 0) {
            tbm_clear_event(TBM_MOTION_EVT);
            tbm_clear_event(TBM_TAMPER_EVT);
            DEBUG_LOG(DBG_CAT_TAG_BEACON, COLOR_BG_BLUE COLOR_B_CYAN "Tag Beacon" COLOR_RST " was queued...");
        } else {
            DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM queue returned an error!");
        }
*/
    } else {
        DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM message queue is full");
    }
}

static void tbm_send_lf_beacon(void)
{
    uint32_t status;
    volatile uint8_t i = 0;

    if (!bmm_queue_is_full()) {
        lfm_lf_beacon_t *data = lfm_get_beacon_data();
        bmm_msg_t msg;
        msg.type = BLE_MSG_LF_FIELD;
        msg.data[i++] = data->lf_event;
        msg.data[i++] = (data->lf_data.id >> 8);
        msg.data[i++] = (data->lf_data.id);
        msg.data[i++] = data->lf_data.command;
        msg.length = i + 1;

        status = bmm_enqueue_msg(&msg);

        if (status == 0) {
            // Message is queued, we can now clear this event.
            tbm_clear_event(TBM_LF_EVT);
            DEBUG_LOG(DBG_CAT_TAG_BEACON, COLOR_BG_BLUE COLOR_B_CYAN "LF Beacon" COLOR_RST " was queued...");
        } else {
            DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM queue returned an error!");
        }
    } else {
        DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM message queue is full");
    }
}

static void tbm_send_temperature_beacon(void)
{
    uint32_t status;
    volatile uint8_t i = 0;

    if (!bmm_queue_is_full()) {
        int8_t temperature = ttm_get_current_temperature();
        bmm_msg_t msg;
        msg.type = BLE_MSG_TEMPERATURE;
        msg.data[i++] = (uint8_t)temperature;
        msg.length = i + 1;

        status = bmm_enqueue_msg(&msg);

        if (status == 0) {
            // Message is queued, we can now clear this event.
            tbm_clear_event(TBM_TEMPERATURE_EVT);
            DEBUG_LOG(DBG_CAT_TAG_BEACON, COLOR_BG_BLUE COLOR_B_CYAN "Tag Temperature Beacon" COLOR_RST " was queued...");
        } else {
            DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM queue returned an error...");
        }
    } else {
        DEBUG_LOG(DBG_CAT_TAG_BEACON, "BMM message queue is full");
    }

}

#if 0
static void tbm_send_cmd_ack_beacon()
{
    //to be implemented
}

static void tbm_send_fall_detection_beacon()
{
    //to be implemented
}

static void tbm_send_button_beacon()
{
    //to be implemented
}
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

            case TBM_FIRMWARE_REV_EVT:
                tbm_send_firmware_rev_beacon();
                break;

            case TBM_EXT_STATUS_EVT:
                tbm_send_tag_ext_status_beacon();
                break;

            case TBM_CMD_ACK_EVT:
                //TODO to be implemented
                //tbm_send_cmd_ack_beacon();
                break;

            case TBM_MOTION_EVT:
                // This event is handled in "tbm_send_tag_status_beacon"
                break;

#if 0 // Feature not supported on UT3
            case TBM_TAMPER_EVT:
                // This event is handled in "tbm_send_tag_status_beacon"
                break;

            case TBM_FALL_DETECT_EVT:
                tbm_send_fall_detection_beacon();
                break;

            case TBM_BUTTON_EVT:
                tbm_send_button_beacon();
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
/**
 * @brief Sets an event flag for Tag Beacon Machine
 * @param event
 * @param is_async
 */
void tbm_set_event(tbm_beacon_events_t event, bool is_async)
{
    tbm_status.events |= event;

    if (is_async) {
        tbm_status.event_async_flag |= event;
    } else {
        tbm_status.event_async_flag &= ~event;
    }
}

/**
 * @brief Tag Beacon Machine
 * @details
 *     - Check for sync and async Tag Beacon events sent by other modules.
 *     - Get beacon data, build beacon messages and enqueue so BLE Manager can transmit them.
 *     - Controls beacon rate for synchronous Tag Beacon events based on Tag
 *       current status. e.g When Staying in Field tag may use TBM_FAST_BEACON_RATE_SEC
 *       beacon rate rather than TBM_SLOW_BEACON_RATE_SEC beacon rate.
 */
void tag_beacon_run(void)
{

    if (bmm_queue_is_empty()) {
        int8_t bit;

        // Tick Tag Beacon Machine (Synchronous Beacon Rate)
        tag_sw_timer_tick(&tbm_beacon_timer);

        // First add Tag Beacon Message (this message is present on every single transmission)
        if ((tbm_status.event_async_flag != 0) || ( tag_sw_timer_is_expired(&tbm_beacon_timer))) {
            // This message contains status bits and also async event flags like Motion and Tamper
            // So it is sort of special message that does not really falls in async or sync type.
            tbm_send_tag_status_beacon();
        }

        // Then check for async beacon events
        if (tbm_status.event_async_flag != 0) {
            for (bit = 0; bit < TBM_MAX_EVENTS; bit++) {
                tbm_beacon_events_t event = tbm_get_async_event(bit);
                if (event) {
                    tbm_decode_event(event);
                }
            }
        }

        // Then check if it is time to send synchronous beacons
        if (tag_sw_timer_is_expired(&tbm_beacon_timer)) {
            if (tbm_status.events != 0) {
                for (bit = 0; bit < TBM_MAX_EVENTS; bit++) {
                    tbm_beacon_events_t event = tbm_get_sync_event(bit);
                    if (event) {
                        tbm_decode_event(event);
                    }
                }
            }
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
