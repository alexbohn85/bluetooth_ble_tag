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
#include "tag_main_machine.h"
#include "tag_beacon_machine.h"


//******************************************************************************
// Defines
//******************************************************************************
#define TBM_MAX_EVENTS                 (31)


//******************************************************************************
// Data types
//******************************************************************************


//******************************************************************************
// Global variables
//******************************************************************************
static volatile tbm_beacon_events_t event_flags;


//******************************************************************************
// Static functions
//******************************************************************************
//! @brief Get event flag
//! @param value between 0 to TBM_MAX_EVENTS
//! @return 0 if event is not pending, or the event itself otherwise.
static tbm_beacon_events_t tbm_get_event(uint8_t bit)
{
    return (event_flags & (1 << bit));
}

static void tbm_clear_event(tbm_beacon_events_t event)
{
    event_flags &= ~(event);
}

static void tbm_send_lf_beacon(void)
{
    //uint32_t status;
    //status = ble_manager_queue_msg(type, data, len, priority);
    //if (status == 0) {
        tbm_clear_event(TBM_LF_EVT);
    //}
}

static void tbm_send_temperature_beacon(void)
{

    //uint32_t status;
    //status = ble_manager_queue_msg(type, data, len, priority);
    //if (status == 0) {
        tbm_clear_event(TBM_TEMPERATURE_EVT);
    //}

}

#if 0
static void tbm_send_cmd_ack_beacon()
{
    //to be implemented
}

static void tbm_send_fw_rev_beacon()
{
    //to be implemented
}

static void tbm_send_battery_beacon()
{
    //to be implemented
}

static void tbm_send_status_beacon()
{
    //to be implemented
}

//static void tbm_send_mandown_beacon();
//static void tbm_send_button_beacon();
//static void tbm_send_motion_beacon();
#endif

//! @brief Decode pending events, Prioritize messages, Sends messages to BLE Manager Queue
static void tbm_decode_event(uint8_t bit)
{

    tbm_beacon_events_t event = tbm_get_event(bit);

    if (event != 0) {
        switch (event) {
            case TBM_LF_EVT:
                tbm_send_lf_beacon();
                break;
            case TBM_TEMPERATURE_EVT:
                tbm_send_temperature_beacon();
                break;
            case TBM_CMD_ACK_EVT:
                //tbm_send_cmd_ack_beacon();
                break;
            case TBM_FW_REV_EVT:
                //tbm_send_fw_rev_beacon();
                break;
            case TBM_STATUS_EVT:
                //tbm_send_status_beacon();
                break;
            case TBM_BATTERY_EVT:
                //tbm_send_battery_beacon();
                break;
            case TBM_MAN_DOWN_EVT:
                //tbm_send_mandown_beacon();
                break;
            case TBM_BUTTON_EVT:
                //tbm_send_button_beacon();
                break;
            case TBM_MOTION_EVT:
                //tbm_send_motion_beacon();
                break;
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
void tbm_set_event(tbm_beacon_events_t event)
{
    event_flags |= event;
}

#if 0
void tbm_sync(void)
{
    //! This will enforce Tag Beacon Machine to process events immediately.
    tbm_timer = 0;
}

void tbm_timer_reload(uint16_t value)
{
    tbm_timer = value;
}

void tbm_tick(void)
{
    if (tbm_timer > 0) {
        --tbm_timer;
    }
}
#endif

//!  @brief Tag Beacon Machine
void tag_beacon_run(void)
{
    int8_t bit;

    //! Check if there is any pending beacon event
    if (event_flags != 0) {
        //! If true, handle them.
        for (bit = 0; bit < TBM_MAX_EVENTS; bit++) {
            tbm_decode_event(bit);
        }
    }
}

void tbm_init(void)
{
    //! We init globals here (this will be called when Tag Main Machine Timer
    //! gets started.
    event_flags = 0;
}
