/*
 * tag_status_machine.c
 *
 */


#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "em_common.h"
#include "app_assert.h"

#include "dbg_utils.h"
#include "as393x.h"
#include "boot.h"
#include "tag_defines.h"
#include "tag_beacon_machine.h"
#include "tag_sw_timer.h"
#include "version.h"
#include "tag_status_machine.h"

//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************
typedef enum tsm_tag_status_flags_t {
    TSM_BATTERY_LOW_EVT,
    TSM_TAMPER,
    TSM_TAG_IN_USE,
    TSM_TAG_IN_MOTION,
    TSM_AMBIENT_LIGHT,
    TSM_LF_RX_MOTION,
    TSM_DEEP_SLEEP,
    TSM_LF_SENSITIVITY,
    TSM_SLOW_BEACON_RATE,
    TSM_FAST_BEACON_RATE,
    TSM_STATUS_BYTE_EXTENDER
} tsm_tag_status_flags_t;

//******************************************************************************
// Global variables
//******************************************************************************
static tag_sw_timer_t tms_timer;
static tsm_tag_status_beacon_t tsm_beacon_data;

//******************************************************************************
// Static functions
//******************************************************************************
/**
 * @brief Update Tag Status Data Struct
 */
static void tsm_update_tag_status(void)
{
    // Get FW Rev
    memcpy(tsm_beacon_data.fw_rev, firmware_revision, sizeof(firmware_revision));

    //TODO as features are implemented there should be get interfaces..
    // For now just populate with 0s
    // Get Battery Low Bit
    tsm_beacon_data.tag_status.battery_low_bit = 0;
    // Get Tamper bit
    tsm_beacon_data.tag_status.tamper = 0;
    // Get Tag in Use bit
    tsm_beacon_data.tag_status.tag_in_use = 0;
    // Get Tag In Motion bit
    tsm_beacon_data.tag_status.tag_in_motion = 0;
    // Get Ambient Light bit
    tsm_beacon_data.tag_status.ambient_light = 0;
    // Get LF Rx Motion Triggered bit
    tsm_beacon_data.tag_status.lf_rx_motion = 0;
    // Get Deep Sleep Mode bit
    tsm_beacon_data.tag_status.deep_sleep = 0;
    // Get LF Sensitivity Bit 1-High / 0-Low
    tsm_beacon_data.tag_status.lf_sensitivity = 0;
    // Get Slow Beacon Rate configuration
    tsm_beacon_data.tag_status.slow_beacon_rate = 0;
    // Get Fast Beacon Rate configuration
    tsm_beacon_data.tag_status.fast_beacon_rate = 0;

    // Set length
    tsm_beacon_data.tag_status.length = 2;

}

//******************************************************************************
// Non Static functions
//******************************************************************************
tsm_tag_status_beacon_t* tsm_get_tag_status_beacon_data(void)
{
    return &tsm_beacon_data;
}

/**
 * @brief Tag Status Machine (Slow Task)
 * @details Reports Tag Status Message - Contains FW Revision + Tag Status Bytes
 */
void tag_status_run(void)
{
    tag_sw_timer_tick(&tms_timer);
    if(tag_sw_timer_is_expired(&tms_timer)) {

        // Update Tag Status Data
        tsm_update_tag_status();

        // Send Asynchronous Tag Beacon Event to Tag Beacon Machine
        tbm_set_event(TBM_STATUS_EVT, true);

        // Reload TSM timer
        tag_sw_timer_reload(&tms_timer, TMM_TAG_STATUS_PERIOD_SEC);
    }
}

//! @brief Tag Status Machine Init
uint32_t tsm_init(void)
{
    // Init Tag Status Beacon Data
    tsm_update_tag_status();

    // Init Tag Status Machine internal timers
    tag_sw_timer_reload(&tms_timer, TMM_TAG_STATUS_PERIOD_SEC);

    return 0;
}
