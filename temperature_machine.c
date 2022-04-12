/*
 * temperature.c
 *
 *  This module runs from Tag Main Machine ISR context.
 *
 *  Reads Sensor Temperature every "TTM_TIMER_PERIOD_SEC" seconds and Sends Temperature
 *  Beacon Events to Tag Beacon Machine.
 *
 */

#include "em_common.h"
#include "em_emu.h"
#include "stdio.h"
#include "stdbool.h"
#include "dbg_utils.h"
#include "tag_sw_timer.h"
#include "temperature_machine.h"
#include "tag_beacon_machine.h"


//******************************************************************************
// Defines
//******************************************************************************
#define TEMP_SENSOR_PRECISION        (100)

//******************************************************************************
// Data types
//******************************************************************************


//******************************************************************************
// Globals
//******************************************************************************
static volatile int8_t ttm_tag_current_temperature;
static tag_sw_timer_t ttm_report_timer;

//******************************************************************************
// Static Functions
//******************************************************************************
static void ttm_read_temperature(void)
{
    volatile float temp_celsius = EMU_TemperatureGet();

#if defined(TAG_LOG_PRESENT)
    // For debug only
    uint8_t int_part = (uint8_t)temp_celsius;
    float frac = temp_celsius - int_part;
    uint16_t dec_part = ((uint16_t)(frac * TEMP_SENSOR_PRECISION));
    DEBUG_LOG(DBG_CAT_TEMP,"Temperature: %1d.%02d C", int_part, dec_part);
#endif

    if (temp_celsius < INT8_MIN) {
        temp_celsius = INT8_MIN;
    } else if (temp_celsius > INT8_MAX) {
        temp_celsius = INT8_MAX;
    }

    // Rounding to signed char
    ttm_tag_current_temperature = (((int8_t)(temp_celsius + 0.5 - INT8_MIN)) + INT8_MIN);
}

//******************************************************************************
// Non-Static Functions
//******************************************************************************
/**
 * @brief Return latest sensor temperature read value
 * @return int8_t - @ref ttm_tag_current_temperature
 */
int8_t ttm_get_current_temperature(void)
{
    return ttm_tag_current_temperature;
}

/**
 * @brief Tag Temperature Machine
 * @details Reports Temperature read events to Tag Beacon Machine
 */
void temperature_run(void)
{
    // Tick Temperature Report Timer
    tag_sw_timer_tick(&ttm_report_timer);

    // Measure SoC temperature and send Tag Temperature Beacon event to Tag Beacon Machine.
    if (tag_sw_timer_is_expired(&ttm_report_timer)) {

        // Read current temperature.
        ttm_read_temperature();

        // Send an Async Temperature Beacon event to Tag Beacon Machine (this message will be sent immediately)
        //tbm_set_event(TBM_TEMPERATURE_EVT, true);

        // Send an Sync Temperature Beacon event to Tag Beacon Machine (this message will synchronize with the slow or fast beacon rate)
        tbm_set_event(TBM_TEMPERATURE_EVT, false);

        // Reload TTM Report Timer
        tag_sw_timer_reload(&ttm_report_timer, TTM_TIMER_PERIOD_SEC_RELOAD);

    }
}

uint32_t ttm_init(void)
{
    // Load internal timer for temperature sampling and temperature report interval
    tag_sw_timer_reload(&ttm_report_timer, TTM_TIMER_PERIOD_SEC_RELOAD);
    ttm_read_temperature();
    return 0;
}
