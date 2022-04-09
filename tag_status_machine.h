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
#define TMM_TAG_EXT_STATUS_PERIOD_SEC                  (120)                    // Period for Tag Extended Status Beacon sent every 2 minutes
#else
#define TMM_TAG_EXT_STATUS_PERIOD_SEC                  (1800)                   // Period for Tag Extended Status Beacon sent every 30 minutes
#endif


//******************************************************************************
// Extern global variables
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************

// -----------------------------------------------------------------------------
// Tag Status Data Types
// -----------------------------------------------------------------------------
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
    union {
        /* Byte 0 */
        uint8_t slow_beacon_rate: 3;    /* Current setting for Slow Beacon Rate */
        uint8_t fast_beacon_rate: 3;    /* Current setting for Fast Beacon Rate */
        uint8_t lf_sensitivity : 2;     /* Current setting for LF Gain */

        uint8_t byte;
    };
} tsm_tag_ext_status_t;

// -----------------------------------------------------------------------------
// Tag Status Beacon Data Types
// -----------------------------------------------------------------------------
typedef struct tsm_tag_status_beacon_t {
    tsm_tag_status_t status;
} tsm_tag_status_beacon_t;

typedef struct tsm_tag_ext_status_beacon_t {
    uint8_t fw_rev[4];
    tsm_tag_ext_status_t ext_status;
} tsm_tag_ext_status_beacon_t;


//******************************************************************************
// Interface
//******************************************************************************
/**
 * @brief Return pointer to Tag Status Beacon Data
 * @return tsm_tag_status_beacon_t*
 */
tsm_tag_status_beacon_t* tsm_get_tag_status_beacon_data(void);

/**
 * @brief Return pointer to Tag Extended Status Beacon Data
 * @return tsm_tag_ext_status_beacon_t*
 */
tsm_tag_ext_status_beacon_t* tsm_get_tag_ext_status_beacon_data(void);

void tag_ext_status_run(void);
void tag_status_run(void);
uint32_t tsm_init(void);

#endif /* TAG_STATUS_MACHINE_H_ */
