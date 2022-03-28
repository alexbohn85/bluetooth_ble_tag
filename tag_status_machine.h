/*
 * tag_status_machine.h
 *
 */

#ifndef TAG_STATUS_MACHINE_H_
#define TAG_STATUS_MACHINE_H_

#include "tag_defines.h"

//******************************************************************************
// Defines
//******************************************************************************
#if defined(TAG_DEV_MODE_PRESENT)
#define TMM_TAG_STATUS_PERIOD_SEC                  (120)                        // T Tag Status Beacon sent every 20 seconds
#else
#define TMM_TAG_STATUS_PERIOD_SEC                  (1800)                      // T Tag Status Beacon sent every 30 minutes
#endif

//#define TSM_LF_HIGH_SENS_ENABLED                  (1<<0)
//#define TSM_MOTION_ENABLED                        (1<<1)
//#define TSM_FALL_DETECTION_ENABLED                (1<<2)
//#define TSM_LF_ENABLED                            (1<<3)
//#define TSM_DEEP_SLEEP_MODE_ENABLED               (1<<4)
//#define TSM_MANUFACTURING_MODE_ENABLED            (1<<5)
//#define TSM_LOW_BATTERY_LEVEL                     (1<<6)

//******************************************************************************
// Extern global variables
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************
// Tag Status Data Struct
typedef struct tsm_tag_status_t {
    uint8_t length;
    union {
        uint16_t battery_low_bit: 1;
        uint16_t tamper: 1;
        uint16_t tag_in_use: 1;
        uint16_t tag_in_motion: 1;
        uint16_t ambient_light: 1;
        uint16_t lf_rx_motion: 1;
        uint16_t deep_sleep: 1;
        uint16_t lf_sensitivity: 1;
        uint16_t slow_beacon_rate: 3;
        uint16_t fast_beacon_rate: 3;
        uint16_t reserved: 1;
        uint16_t status_byte_extender: 1;
        uint8_t byte[2];
    };
} tsm_tag_status_t;

// Tag Status Beacon Data Struct
typedef struct tsm_tag_status_beacon_t {
    uint8_t fw_rev[4];
    tsm_tag_status_t tag_status;
} tsm_tag_status_beacon_t;

//******************************************************************************
// Interface
//******************************************************************************
tsm_tag_status_beacon_t* tsm_get_tag_status_beacon_data(void);
void tag_status_run(void);
uint32_t tsm_init(void);

#endif /* TAG_STATUS_MACHINE_H_ */
