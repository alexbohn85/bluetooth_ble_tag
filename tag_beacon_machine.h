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
    TBM_MAN_DOWN_EVT       = (1 << 0),
    TBM_LF_EVT             = (1 << 1),
    TBM_BUTTON_EVT         = (1 << 2),
    TBM_MOTION_EVT         = (1 << 3),
    TBM_TAMPER_EVT         = (1 << 4),
    TBM_STATUS_EVT         = (1 << 5),
    TBM_CMD_ACK_EVT        = (1 << 6),
    TBM_BATTERY_EVT        = (1 << 7),
    TBM_TEMPERATURE_EVT    = (1 << 8)
} tbm_beacon_events_t;

//******************************************************************************
// Interface
//******************************************************************************
void tbm_set_event(tbm_beacon_events_t event, bool is_async);
void tbm_set_fast_beacon_rate(bool enable);
void tag_beacon_run(void);
uint32_t tbm_init(void);

#endif /* TAG_BEACON_MACHINE_H_ */
