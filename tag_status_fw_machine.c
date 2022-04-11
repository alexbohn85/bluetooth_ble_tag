/*
 * tag_status_fw_machine.c
 *
 */


#include <tag_status_fw_machine.h>
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
static tag_sw_timer_t ext_status_report_tmr;
static tag_sw_timer_t fw_rev_report_tmr;
static tsm_tag_status_t tsm_tag_status;
static tsm_tag_ext_status_t tsm_tag_ext_status;

//******************************************************************************
// Static functions
//******************************************************************************
/**
 * @brief Update Tag EXTENDED Status Beacon Data
 */
static void tsm_update_tag_extended_status(void)
{
    //TODO TO BE IMPLEMENTED!!

    // Get Fast Beacon Rate configuration
    tsm_tag_ext_status.fast_beacon_rate = 0;

    // Get LF Gain Reduction Setting
    tsm_tag_ext_status.lf_sensitivity = as39_get_attenuation_set();

    // Get Slow Beacon Rate configuration
    tsm_tag_ext_status.slow_beacon_rate = 0;
}

/**
 * @brief Update Tag Status Beacon Data
 */
static void tsm_update_tag_status(tsm_tag_status_flags_t flag, bool value)
{

    switch(flag) {
        case TSM_BATTERY_LOW:
            tsm_tag_status.battery_low_alarm = value;
            break;
        case TSM_TAMPER:
            tsm_tag_status.tamper_alarm = value;
            break;
        case TSM_TAG_IN_MOTION:
            tsm_tag_status.tag_in_motion = value;
            break;
        case TSM_AMBIENT_LIGHT:
            tsm_tag_status.ambient_light_alarm = value;
            break;
        case TSM_TAG_IN_USE:
            tsm_tag_status.tag_in_use = value;
            break;
        case TSM_DEEP_SLEEP:
            tsm_tag_status.deep_sleep = value;
            break;
        case TSM_LF_RX_ONLY_IN_MOTION:
            tsm_tag_status.lf_rx_on_motion = value;
            break;
        default:
            break;
    }

#if (TAG_ID == UT3_ID)
    tsm_tag_status.tamper_alarm = TSM_FEATURE_NOT_SUPPORTED;
    tsm_tag_status.lf_rx_on_motion = TSM_FEATURE_NOT_SUPPORTED;
    tsm_tag_status.ambient_light_alarm = TSM_FEATURE_NOT_SUPPORTED;
#endif
}

//******************************************************************************
// Non Static functions
//******************************************************************************
tsm_tag_status_t* tsm_get_tag_status_beacon_data(void)
{
    return &tsm_tag_status;
}

tsm_tag_ext_status_t* tsm_get_tag_ext_status_beacon_data(void)
{
    return &tsm_tag_ext_status;
}

void tsm_set_tag_status_flag(tsm_tag_status_flags_t flag)
{
    tsm_update_tag_status(flag, 1);
}

void tsm_clear_tag_status_flag(tsm_tag_status_flags_t flag)
{
    tsm_update_tag_status(flag, 0);
}

/**
 * @brief Tag Status Machine - Extended Status (Slow Task)
 * @details Reports Tag Extended Status Message - Contains FW Revision + Extended Status Bytes
 */
void tag_extended_status_run(void)
{
    tag_sw_timer_tick(&ext_status_report_tmr);

    if (tag_sw_timer_is_expired(&ext_status_report_tmr)) {

        // Updated Tag Extended Status Data
        tsm_update_tag_extended_status();

        // Send Asynchronous Tag Beacon Event to Tag Beacon Machine
        tbm_set_event(TBM_EXT_STATUS_EVT, true);

        // Reload TSM timer
        tag_sw_timer_reload(&ext_status_report_tmr, TMM_TAG_EXT_STATUS_PERIOD_SEC);
    }
}

/**
 * @brief Tag Firmware Revision Report (Slow Task)
 */
void tag_firmware_rev_run(void)
{
    tag_sw_timer_tick(&fw_rev_report_tmr);

    if (tag_sw_timer_is_expired(&fw_rev_report_tmr)) {

        tag_sw_timer_reload(&ext_status_report_tmr, TMM_TAG_EXT_STATUS_PERIOD_SEC);

        // Send Asynchronous Tag Firmware Revision Beacon Event to Tag Beacon Machine
        tbm_set_event(TBM_FIRMWARE_REV_EVT, true);
    }
}

//! @brief Tag Status Machine Init
uint32_t tsm_init(void)
{

    // Init Tag Extended Status
    tsm_update_tag_extended_status();

    // Init Tag Extended Status Report Timer
    tag_sw_timer_reload(&ext_status_report_tmr, TMM_TAG_EXT_STATUS_PERIOD_SEC_INIT);

    // Init Tag Firmware Rev Report Timer
    tag_sw_timer_reload(&ext_status_report_tmr, TMM_TAG_FW_REV_PERIOD_SEC_INIT);

    return 0;
}
