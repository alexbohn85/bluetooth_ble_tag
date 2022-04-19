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
#include "version.h"
#include "temperature_machine.h"
#include "ble_manager_machine.h"
#include "tag_main_machine.h"
#include "tag_status_fw_machine.h"
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
            return "Tag Status";

        case TBM_BUTTON_EVT:
            return "Button Press";

        case TBM_LF_EVT:
            return "LF Field";

        case TBM_FALL_DETECT_EVT:
            return "Fall Detection";

        case TBM_FIRMWARE_REV_EVT:
            return "Firmware Revision";

        case TBM_EXT_STATUS_EVT:
            return "Tag Extended Status";

        case TBM_TEMPERATURE_EVT:
            return "Tag Temperature";

        case TBM_UPTIME_EVT:
            return "Tag Uptime";

        case TBM_CMD_ACK_EVT:
            return "Tag Command Acknowledgment";

        default:
            return COLOR_BG_RED COLOR_B_YELLOW "Unknown Tag Beacon Type" COLOR_RST;
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
 * @brief Enqueue message and clear TBM event
 * @param bmm_msg_t*
 * @param tbm_beacon_events_t
 */
static void tbm_dispatch_msg(bmm_msg_t* msg, tbm_beacon_events_t event)
{
    uint32_t status;

    status = bmm_enqueue_msg(msg);

    if (status == 0) {
        // Message was enqueued, we can now clear this TBM event.
        tbm_clear_event(event);
        DEBUG_LOG(DBG_CAT_TAG_BEACON, COLOR_BG_BLUE COLOR_B_CYAN "%s" COLOR_RST " beacon message was enqueued...", tbm_get_event_name(event));
    } else {
        DEBUG_LOG(DBG_CAT_WARNING, "Error to enqueue beacon message...");
    }
}

static void tbm_send_firmware_rev_beacon(void)
{
    volatile uint8_t i = 0;

    if (!bmm_queue_is_full()) {
        bmm_msg_t msg;
        msg.type = BLE_MSG_FIRMWARE_REV;
        msg.data[i++] = firmware_revision[0];  /* most significant digit of FW rev format */
        msg.data[i++] = firmware_revision[1];
        msg.data[i++] = firmware_revision[2];
        msg.data[i++] = firmware_revision[3];
        msg.length = i + 1;

        // Send msg to BLE Manager queue and clear TBM event.
        tbm_dispatch_msg(&msg, TBM_FIRMWARE_REV_EVT);

    } else {
        DEBUG_LOG(DBG_CAT_WARNING, "BMM message queue is full");
    }
}

static void tbm_send_tag_ext_status_beacon(void)
{
    volatile uint8_t i = 0;
    volatile uint8_t j;
    bmm_msg_t msg;

    if (!bmm_queue_is_full()) {
        tsm_tag_ext_status_t *data = tsm_get_tag_ext_status_beacon_data();
        msg.type = BLE_MSG_TAG_EXT_STATUS;
        msg.data[i++] = data->length;
        for (j = 0; j < data->length; j++) {
            msg.data[i++] = data->bytes[j];
        }
        msg.length = i + 1;

        // Send msg to BLE Manager queue and clear TBM event.
        tbm_dispatch_msg(&msg, TBM_EXT_STATUS_EVT);

    } else {
        DEBUG_LOG(DBG_CAT_WARNING, "BMM message queue is full");
    }
}

static void tbm_send_tag_status_beacon(void)
{
    volatile uint8_t i = 0;

    if (!bmm_queue_is_full()) {
        tsm_tag_status_t *data = tsm_get_tag_status_beacon_data();
        bmm_msg_t msg;
        msg.type = BLE_MSG_TAG_STATUS;
        msg.data[i++] = data->byte;
        msg.length = i + 1;

        // Send msg to BLE Manager queue and clear TBM event.
        tbm_dispatch_msg(&msg, TBM_TAG_STATUS_EVT);

    } else {
        DEBUG_LOG(DBG_CAT_WARNING, "BMM message queue is full");
    }
}

static void tbm_send_lf_beacon(void)
{
    volatile uint8_t i = 0;

    if (!bmm_queue_is_full()) {
        lfm_lf_beacon_t *data = lfm_get_beacon_data();
        bmm_msg_t msg;
        msg.type = BLE_MSG_LF_FIELD;
        msg.data[i++] = data->lf_data[0];
        msg.data[i++] = data->lf_data[1];
        msg.length = i + 1;

        // Send msg to BLE Manager queue and clear TBM event.
        tbm_dispatch_msg(&msg, TBM_LF_EVT);

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

static void tbm_send_uptime_beacon(void)
{
    uint32_t status;
    volatile uint8_t i = 0;

    if (!bmm_queue_is_full()) {
        uint16_t uptime_days = tum_get_beacon_data();
        bmm_msg_t msg;
        msg.type = BLE_MSG_UPTIME;
        msg.data[i++] = (uint8_t)(uptime_days >> 8);
        msg.data[i++] = (uint8_t)(uptime_days);
        msg.length = i + 1;

        status = bmm_enqueue_msg(&msg);

        if (status == 0) {
            // Message is queued, we can now clear this event.
            tbm_clear_event(TBM_UPTIME_EVT);
            DEBUG_LOG(DBG_CAT_TAG_BEACON, COLOR_BG_BLUE COLOR_B_CYAN "Tag Uptime Beacon" COLOR_RST " was queued...");
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

            case TBM_UPTIME_EVT:
                tbm_send_uptime_beacon();
                break;

#if 0 // Feature not supported on UT3

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
 * @brief Sets a msg event flag for Tag Beacon Machine
 * @param tbm_beacon_events_t
 * @param bool (if this message is async it will be sent immediately,
 *     otherwise it will sync with next "fast" or "slow" beacon rate timer)
 */
void tbm_set_event(tbm_beacon_events_t event, bool is_async)
{
    tbm_status.events |= event;

    if (is_async) {
        // set flag
        tbm_status.event_async_flag |= event;
    } else {
        // clear flag
        tbm_status.event_async_flag &= ~event;
    }
}

/**
 * @brief Tag Beacon Machine
 * @details
 *     - Check for sync and async Tag Beacon events sent by other modules.
 *     - Get beacon data, build beacon messages and enqueue so BLE Manager can transmit them.
 */
void tag_beacon_run(void)
{

    if (bmm_queue_is_empty()) {
        int8_t bit;

        // Tick Synchronous Beacon timer
        tag_sw_timer_tick(&tbm_beacon_timer);

        // Note: - Async means messages that are not synchronized with "slow" or "fast" beacon rate and will be transmitted immediately.
        //       - Sync means messages that are to be synchronized with "slow" or "fast" beacon rate so it will only be sent
        //             once 'tbm_beacon_timer' expires.

        // Check if there are async or sync tag beacon events
        if ((tbm_status.event_async_flag != 0) || ( tag_sw_timer_is_expired(&tbm_beacon_timer))) {

            // First add Tag Status Message (this message is present on every single transmission)
            // This message also handles some tag features events like tamper, battery low, motion, ambient light sensor, etc.
            tbm_send_tag_status_beacon();

            // Then check for async beacon events (these are usually critical events like entering lf field, etc..)
            if (tbm_status.event_async_flag != 0) {
                for (bit = 0; bit < TBM_MAX_EVENTS; bit++) {
                    tbm_beacon_events_t event = tbm_get_async_event(bit);
                    if (event) {
                        tbm_decode_event(event);
                    }
                }
            }

            // Then check if it is time to send synchronous beacons (these are usually periodic events like staying in field, firmware rev, uptime.. etc)
            if (tag_sw_timer_is_expired(&tbm_beacon_timer)) {
                if (tbm_status.events != 0) {
                    for (bit = 0; bit < TBM_MAX_EVENTS; bit++) {
                        tbm_beacon_events_t event = tbm_get_sync_event(bit);
                        if (event) {
                            tbm_decode_event(event);
                        }
                    }
                }
            }

            // Reload Synchronous Beacon timer
            // Check for feature states and decide if next beacon is "slow" or "fast" interval.
            if ((lfm_get_lf_status() & (LFM_STAYING_IN_FIELD_FLAG | LFM_ENTERING_FIELD_FLAG))) {
                tag_sw_timer_reload(&tbm_beacon_timer, TBM_FAST_BEACON_RATE_RELOAD);
            } else {
                //tag_sw_timer_reload(&tbm_beacon_timer, TBM_FAST_BEACON_RATE_RELOAD);
                tag_sw_timer_reload(&tbm_beacon_timer, TBM_SLOW_BEACON_RATE_RELOAD);
            }
        }
    }
}

uint32_t tbm_init(void)
{
    tag_sw_timer_reload(&tbm_beacon_timer, TBM_INIT_BEACON_RATE_SEC);
    return 0;
}
