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
#define TSM_FEATURE_NOT_SUPPORTED            (0)

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Global variables
//******************************************************************************
static tag_sw_timer_t tsm_timer;
static tsm_tag_status_beacon_t tsm_status_beacon;
static tsm_tag_ext_status_beacon_t tsm_ext_status_beacon;

//******************************************************************************
// Static functions
//******************************************************************************
/**
 * @brief Update Tag Extended Status Beacon Data
 */
static void tsm_update_tag_extended_status(void)
{
    // Get FW Rev
    memcpy(tsm_ext_status_beacon.fw_rev, firmware_revision, sizeof(firmware_revision));

    // Get Fast Beacon Rate configuration
    tsm_ext_status_beacon.ext_status.fast_beacon_rate = 0;


    // Get LF Gain Reduction Setting
    tsm_ext_status_beacon.ext_status.lf_sensitivity = as39_get_attenuation_set();

    // Get Slow Beacon Rate configuration
    tsm_ext_status_beacon.ext_status.slow_beacon_rate = 0;
}

/**
 * @brief Update Tag Status Beacon Data
 */
static void tsm_update_tag_status(void)
{
    //TODO as features are implemented there should be get interfaces..
    // For now just populate with 0s
#if (TAG_ID == UT3_ID)
    // Get Battery Low Bit
    tsm_status_beacon.status.battery_low_alarm = 0;

    // Get Tamper bit (not supported on UT3)
    tsm_status_beacon.status.tamper_alarm = 0;
    // Get LF Rx Motion Triggered bit (not supported for UT3)
    tsm_status_beacon.status.lf_rx_on_motion = TSM_FEATURE_NOT_SUPPORTED;

    // Get Tag in Use bit
    tsm_status_beacon.status.tag_in_use = 0;

    // Get Tag In Motion bit
    tsm_status_beacon.status.tag_in_motion = 0;

    // Get Deep Sleep Mode bit (1-deep-sleep / 0-normal)
    tsm_status_beacon.status.deep_sleep = 0;

    // Get Ambient Light bit (not supported on UT3)
    tsm_status_beacon.status.ambient_light_alarm = TSM_FEATURE_NOT_SUPPORTED;
#endif

}

//******************************************************************************
// Non Static functions
//******************************************************************************
tsm_tag_status_beacon_t* tsm_get_tag_status_beacon_data(void)
{
    return &tsm_status_beacon;
}

tsm_tag_ext_status_beacon_t* tsm_get_tag_ext_status_beacon_data(void)
{
    return &tsm_ext_status_beacon;
}

/**
 * @brief Tag Status Machine - Extended Status (Slow Task)
 * @details Reports Tag Extended Status Message - Contains FW Revision + Extended Status Bytes
 */
void tag_ext_status_run(void)
{
    tag_sw_timer_tick(&tsm_timer);

    if(tag_sw_timer_is_expired(&tsm_timer)) {

        // Updated Tag Extended Status Data
        tsm_update_tag_extended_status();

        // Send Asynchronous Tag Beacon Event to Tag Beacon Machine
        tbm_set_event(TBM_EXT_STATUS_EVT, true);

        // Reload TSM timer
        tag_sw_timer_reload(&tsm_timer, TMM_TAG_EXT_STATUS_PERIOD_SEC);
    }
}

/**
 * @brief Tag Status Machine (Fast Task)
 * @details Update Tag Status Byte
 */
void tag_status_run(void)
{
    // Update Tag Status Data ( this needs to be updated on every TMM tick )
    tsm_update_tag_status();
}

//! @brief Tag Status Machine Init
uint32_t tsm_init(void)
{
    // Init Tag Status Beacon Data
    tsm_update_tag_status();

    // Init Tag Status Machine internal timers
    tag_sw_timer_reload(&tsm_timer, TMM_TAG_EXT_STATUS_PERIOD_SEC);

    return 0;
}
