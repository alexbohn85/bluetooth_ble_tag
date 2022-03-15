/*
 * tag_status_machine.c
 *
 */


#include "stdio.h"
#include "stdbool.h"
#include "em_common.h"
#include "app_assert.h"

#include "dbg_utils.h"
#include "as393x.h"
#include "boot.h"
#include "tag_defines.h"
#include "tag_beacon_machine.h"
#include "tag_sw_timer.h"
#include "tag_status_machine.h"

//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************
typedef enum tsm_tag_status_evt_t {
    TSM_TAG_RESET_CAUSE_EVT,
    TSM_LF_ATTENUATION_EVT,
    TSM_LF_ENABLE_EVT,
    TSM_MOTION_ENABLE_EVT,
    TSM_FALL_DETECTION_ENABLE_EVT,
    TSM_DEEP_SLEEP_MODE_ENABLE_EVT,
    TSM_TAG_OP_MODE_EVT,
    TSM_BATTERY_LOW_EVT
} tsm_tag_status_evt_t;

//******************************************************************************
// Global variables
//******************************************************************************
static tag_sw_timer_t tms_timer;
static tsm_tag_status_beacon_t tag_status_beacon_data = { .fw_rev = {FW_B4, FW_B3, FW_B2, FW_B1},
                                                          .tag_status = {0} };

//******************************************************************************
// Static functions
//******************************************************************************
/**
 * @brief Helper function to set Tag Status Data Struct
 */
static void tsm_set_tag_status(tsm_tag_status_evt_t event, bool enable, uint8_t data)
{
    switch(event) {
        case TSM_TAG_RESET_CAUSE_EVT:
            tag_status_beacon_data.tag_status.last_reset_cause = data;
            break;

        case TSM_LF_ATTENUATION_EVT:
            tag_status_beacon_data.tag_status.lf_attenuation = (0x07 & data);
            break;

        case TSM_LF_ENABLE_EVT:
            tag_status_beacon_data.tag_status.lf_enabled = enable;
            break;

        case TSM_BATTERY_LOW_EVT:
            tag_status_beacon_data.tag_status.battery_low_bit = enable;
            break;

        default:
            break;
    }
}

/**
 * @brief Update Tag Status Data Struct
 */
static void tsm_update_tag_status(void)
{
    // Get last reset cause
    uint8_t reset_cause = (uint8_t)boot_get_reset_cause();

    // Get current AS393x Antenna Attenuation (Gain Reduction)
    as39_settings_handle_t as39_settings;
    as39_get_settings_handler(&as39_settings);
    uint8_t antenna_attenuation = as39_settings->registers.reg_4.GR;



    // Update Tag Status Data
#if TAG_TYPE == TAG_UT3_ID
    tsm_set_tag_status(TSM_LF_ATTENUATION_EVT, NULL, antenna_attenuation);
    tsm_set_tag_status(TSM_TAG_RESET_CAUSE_EVT, NULL, reset_cause);

#endif

}

//******************************************************************************
// Non Static functions
//******************************************************************************


tsm_tag_status_beacon_t* tsm_get_tag_status_beacon_data(void)
{
    return &tag_status_beacon_data;
}

/*!  @brief Tag Status Machine (Slow Task)
 *   @details Reports Tag Status Messages containing FW Revision and other state bytes
 ******************************************************************************/
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
    //! Init machine timer
    tag_sw_timer_reload(&tms_timer, TMM_TAG_STATUS_PERIOD_SEC);
    return 0;
}
