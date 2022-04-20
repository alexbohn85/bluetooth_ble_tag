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

#include "stdbool.h"

//******************************************************************************
// Defines
//******************************************************************************
#define TBM_INIT_BEACON_RATE_SEC              (3)                               // Init value (only first time)
#define TBM_INIT_BEACON_RATE_SEC_RELOAD       ((TBM_INIT_BEACON_RATE_SEC * 1000) / TMM_RTCC_TIMER_PERIOD_MS)

#define TBM_FAST_BEACON_RATE_SEC              (12)                              // When tag is in certain states (e.g Staying In Field, Tamper Alert, Man Down, etc)
#define TBM_FAST_BEACON_RATE_RELOAD           ((TBM_FAST_BEACON_RATE_SEC * 1000) / TMM_RTCC_TIMER_PERIOD_MS)

#define TBM_SLOW_BEACON_RATE_SEC              (600)                             // Default is 10 minutes
#define TBM_SLOW_BEACON_RATE_RELOAD           ((TBM_SLOW_BEACON_RATE_SEC * 1000) / TMM_RTCC_TIMER_PERIOD_MS)

//******************************************************************************
// Data types
//******************************************************************************
typedef enum tbm_beacon_events_t {
    TBM_TAG_STATUS_EVT     = (1 << 0),  /* used for some async events like motion, tamper, etc */
    TBM_FALL_DETECT_EVT    = (1 << 1),
    TBM_LF_EVT             = (1 << 2),
    TBM_BUTTON_EVT         = (1 << 3),
    TBM_EXT_STATUS_EVT     = (1 << 6),
    TBM_CMD_ACK_EVT        = (1 << 7),
    TBM_TEMPERATURE_EVT    = (1 << 8),
    TBM_FIRMWARE_REV_EVT   = (1 << 9),
    TBM_UPTIME_EVT         = (1 << 10)
} tbm_beacon_events_t;

typedef struct tbm_nvm_data_t {
    bool is_erased;           /* if is_erased = "true" factory defined values will be loaded instead during machine init */
    uint16_t fast_rate;
    uint16_t slow_rate;
} tbm_nvm_data_t;

//******************************************************************************
// Interface
//******************************************************************************
void tbm_set_event(tbm_beacon_events_t event, bool is_async);
void tbm_apply_new_settings(tbm_nvm_data_t *b);
uint16_t tbm_get_fast_beacon_rate(void);
uint16_t tbm_get_slow_beacon_rate(void);
void tag_beacon_run(void);
uint32_t tbm_init(void);

#endif /* TAG_BEACON_MACHINE_H_ */
