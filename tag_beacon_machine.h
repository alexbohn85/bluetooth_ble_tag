/*
 * tag_beacon_machine.h
 *
 *  This module runs from Tag Main Machine ISR context.
 *
 *  It is responsible for receiving messages from other modules, prioritizing
 *  and queuing for BLE Manager module.
 *
 */

#ifndef TAG_BEACON_MACHINE_H_
#define TAG_BEACON_MACHINE_H_

//******************************************************************************
// Data types
//******************************************************************************
typedef enum tbm_beacon_events_t {
    TBM_TEMPERATURE_EVT  = (1 << 0),
    TBM_STATUS_EVT       = (1 << 1),
    TBM_LF_EVT           = (1 << 2),
    TBM_MOTION_EVT       = (1 << 3),
    TBM_FW_REV_EVT       = (1 << 4),
    TBM_MAN_DOWN_EVT     = (1 << 5),
    TBM_BUTTON_EVT       = (1 << 6),
    TBM_CMD_ACK_EVT      = (1 << 7),
    TBM_BATTERY_EVT      = (1 << 8),
    TBM_TAMPER_EVT       = (1 << 9)
} tbm_beacon_events_t;

void tbm_set_event(tbm_beacon_events_t event);
void tag_beacon_run(void);
void tbm_init(void);

#endif /* TAG_BEACON_MACHINE_H_ */
