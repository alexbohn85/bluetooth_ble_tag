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
// Globals
//******************************************************************************
static volatile int8_t ttm_tag_current_temperature = 0;
static tag_sw_timer_t ttm_report_timer;

//******************************************************************************
// Static Functions
//******************************************************************************
static void ttm_read_temperature(void)
{
    volatile float temp_celsius = EMU_TemperatureGet();

#if defined(TAG_LOG_PRESENT)
    int int_part = (int)temp_celsius;
    int dec_part = ((int)(temp_celsius * TEMP_SENSOR_PRECISION) % TEMP_SENSOR_PRECISION);
    DEBUG_LOG(DBG_CAT_TEMP,"Temperature: %d.%d C", int_part, dec_part);
#endif

    if (temp_celsius < INT8_MIN) {
        temp_celsius = INT8_MIN;
    } else if (temp_celsius > INT8_MAX) {
        temp_celsius = INT8_MAX;
    }

    //! Rounding to signed char
    ttm_tag_current_temperature = (((int8_t) (temp_celsius + 0.5 - INT8_MIN)) + INT8_MIN);
}

//******************************************************************************
// Non-Static Functions
//******************************************************************************
//! @brief Get current sensor temperature value
//! @return temperature value
int8_t ttm_get_current_temperature(void)
{
    return ttm_tag_current_temperature;
}

//! @brief Temperature Machine
//! @details Reads and sends Tag Temperature Beacon events
void temperature_run(void)
{
    //! Tick Temperature Report Timer
    tag_sw_timer_tick(&ttm_report_timer);

    //! Measure SoC temperature every TTM_TIMER_PERIOD_SEC seconds and
    //! Send Tag Temperature Beacon event to Tag Beacon Machine.
    if (tag_sw_timer_is_expired(&ttm_report_timer)) {
        //! Reload TTM Report Timer
        tag_sw_timer_reload(&ttm_report_timer, TTM_TIMER_PERIOD_SEC);

        //! Read current temperature.
        ttm_read_temperature();

        //! Send Temperature Beacon event to Tag Beacon Machine
        //tbm_set_event(TBM_TEMPERATURE_EVT);
    }
}

uint32_t ttm_init(void)
{
    tag_sw_timer_reload(&ttm_report_timer, 0);
    ttm_read_temperature();
    return 0;
}
