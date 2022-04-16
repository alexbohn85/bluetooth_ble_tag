/*
 * tag_status_fw_machine.h
 *
 */

#ifndef TAG_STATUS_FW_MACHINE_H_
#define TAG_STATUS_FW_MACHINE_H_

#include "stdio.h"

#include "tag_defines.h"

//******************************************************************************
// Defines
//******************************************************************************
// Initial values for Tag Extended Status and Firmware Revision beacon rates (these values are only used for start-up so messages are pushed right away once tag is powered)
#define TMM_TAG_EXT_STATUS_PERIOD_SEC_STARTUP_ONLY             (1)
#define TMM_TAG_FW_REV_PERIOD_SEC_STARTUP_ONLY                 (1)

// Reload values for Tag Extended Status and Firmware Revision beacon rates.
#if defined(TAG_DEV_MODE_PRESENT)
#define TMM_TAG_EXT_STATUS_PERIOD_SEC                  (120)                    // Reload Period for Tag Extended Status Beacon sent every 2 minutes
#define TMM_TAG_FW_REV_PERIOD_SEC                      (180)                    // Reload Period for Tag Firmware Revision Beacon sent every 4 minutes
#else
#define TMM_TAG_EXT_STATUS_PERIOD_SEC                  (1800)                   // Reload Period for Tag Extended Status Beacon sent every 30 minutes
#define TMM_TAG_FW_REV_PERIOD_SEC                      (1800)                   // Reload Period for Tag Firmware Revision Beacon sent every 30 minutes
#endif


//******************************************************************************
// Extern global variables
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************
typedef enum tsm_tag_status_flags_t {
     TSM_BATTERY_LOW,
     TSM_TAMPER,
     TSM_TAG_IN_USE,
     TSM_TAG_IN_MOTION,
     TSM_AMBIENT_LIGHT,
     TSM_LF_RX_ONLY_IN_MOTION,
     TSM_DEEP_SLEEP
} tsm_tag_status_flags_t;

typedef struct tsm_tag_status_t {
    union {
        /* Byte 0 */
        uint8_t battery_low_alarm: 1;    /* 1 if battery is low */
        uint8_t tamper_alarm: 1;         /* 1 if tamper event was detected */
        uint8_t tag_in_use: 1;           /* 1 if tag is "activated" */
        uint8_t tag_in_motion: 1;        /* 1 if tag motion event was detected */
        uint8_t ambient_light_alarm: 1;  /* 1 if ambient light sensor event was detected */
        uint8_t lf_rx_on_motion: 1;      /* 1 if AS3933 is enabled only when tag is in motion */
        uint8_t deep_sleep: 1;           /* 1 if tag is in deep sleep mode */
        uint8_t : 1;  /* not used */

        uint8_t byte;
    };
} tsm_tag_status_t;

typedef struct tsm_tag_ext_status_t {
    uint8_t length;
    union {
        /* Byte 0 */
        uint8_t slow_beacon_rate: 3;    /* Current setting for Slow Beacon Rate */
        uint8_t fast_beacon_rate: 3;    /* Current setting for Fast Beacon Rate */
        uint8_t lf_sensitivity : 2;     /* Current setting for LF Gain */

        uint8_t bytes[6];
    };
} tsm_tag_ext_status_t;

//******************************************************************************
// Interface
//******************************************************************************
tsm_tag_status_t* tsm_get_tag_status_beacon_data(void);
tsm_tag_ext_status_t* tsm_get_tag_ext_status_beacon_data(void);
void tsm_set_tag_status_flag(tsm_tag_status_flags_t flag);
void tsm_clear_tag_status_flag(tsm_tag_status_flags_t flag);
void tag_extended_status_run(void);
void tag_firmware_rev_run(void);
uint32_t tsm_init(void);

#endif /* TAG_STATUS_FW_MACHINE_H_ */
