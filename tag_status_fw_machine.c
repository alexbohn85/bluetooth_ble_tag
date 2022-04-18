/*
 * tag_status_fw_machine.c
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
#include "tag_status_fw_machine.h"

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
static tag_sw_timer_t extended_status_timer;
static tag_sw_timer_t fw_revision_timer;
static tsm_tag_status_t tsm_tag_status;
static tsm_tag_ext_status_t tsm_tag_ext_status;

//******************************************************************************
// Static functions
//******************************************************************************
/**
 * @brief Update Tag Extended Status Beacon Data
 */
static void tsm_update_tag_extended_status(void)
{
    //TODO All hard coded values now for later implementation of configurable fast_beacon_rate, slow_beacon_rate, etc..

    // Get Fast Beacon Rate configuration
    tsm_tag_ext_status.fast_beacon_rate = 0x04;

    // Get Slow Beacon Rate configuration
    tsm_tag_ext_status.slow_beacon_rate = 0x04;

    // Get LF Gain Reduction Setting
    tsm_tag_ext_status.lf_sensitivity = 0; // values (0, 0dBm), (1, -4dBm), (2, -16dBm), (3, -24dBm)

    // For now we only have 1 byte so...
    tsm_tag_ext_status.length = 1;

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
 * @brief Tag Extended Status Report (Slow Task)
 */
void tag_extended_status_run(void)
{
    tag_sw_timer_tick(&extended_status_timer);

    if (tag_sw_timer_is_expired(&extended_status_timer)) {

        // Updated Tag Extended Status Data
        tsm_update_tag_extended_status();

        // Send Asynchronous Tag Beacon Event to Tag Beacon Machine (this message will be sent immediately)
        //tbm_set_event(TBM_EXT_STATUS_EVT, true);

        // Send Sync Tag Beacon Event to Tag Beacon Machine (this message will synchronize with the slow or fast beacon rate)
        tbm_set_event(TBM_EXT_STATUS_EVT, false);

        // Reload Tag Extended Status report timer
        tag_sw_timer_reload(&extended_status_timer, TMM_TAG_EXT_STATUS_PERIOD_SEC);
    }
}

/**
 * @brief Tag Firmware Revision Report (Slow Task)
 */
void tag_firmware_rev_run(void)
{
    tag_sw_timer_tick(&fw_revision_timer);

    if (tag_sw_timer_is_expired(&fw_revision_timer)) {

        // Send Asynchronous Tag Firmware Revision Beacon Event to Tag Beacon Machine (this message will be sent immediately)
        //tbm_set_event(TBM_FIRMWARE_REV_EVT, true);

        // Send Synchronous Tag Firmware Revision Beacon Event to Tag Beacon Machine (this message will synchronize with the slow or fast beacon rate)
        tbm_set_event(TBM_FIRMWARE_REV_EVT, false);

        // Reload Firmware Rev report timer
        tag_sw_timer_reload(&fw_revision_timer, TMM_TAG_FW_REV_PERIOD_SEC);
    }
}

/**
 * @brief Tag Status Machine Init
 */
uint32_t tsm_init(void)
{
    // Init Tag Extended Status Byte
    tsm_update_tag_extended_status();

    // Init Tag Extended Status Report Timer
    tag_sw_timer_reload(&extended_status_timer, TMM_TAG_EXT_STATUS_PERIOD_SEC_STARTUP_ONLY);

    // Init Tag Firmware Rev Report Timer
    tag_sw_timer_reload(&fw_revision_timer, TMM_TAG_FW_REV_PERIOD_SEC_STARTUP_ONLY);

    return 0;
}
