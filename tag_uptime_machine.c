/*
 * tag_uptime_machine.c
 *
 */

#include "stdbool.h"
#include "stdio.h"

#include "tag_sw_timer.h"
#include "tag_beacon_machine.h"
#include "dbg_utils.h"
#include "tag_uptime_machine.h"


//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Global variables
//******************************************************************************
static tag_sw_timer_t uptime_report_tmr;
static uint16_t uptime_days = 0;
static uint16_t uptime_hours = 0;
static uint16_t uptime_seconds = 0;

//******************************************************************************
// Static functions
//******************************************************************************
void tum_uptime_process(void)
{
    uptime_seconds++;
    if (uptime_seconds == 3600) {
        uptime_seconds = 0;
        uptime_hours++;
        if (uptime_hours == 24) {
            uptime_hours = 0;
            uptime_days++;
        }
    }
}

//******************************************************************************
// Non Static functions
//******************************************************************************
uint16_t tum_get_beacon_data(void)
{
    return uptime_days;
}

/**
 * @brief Tag Uptime Machine
 * @details Keep track of number of days running, Sends Uptime Report Event to Tag Beacon Machine
 */
void tag_uptime_run(void)
{

    tag_sw_timer_tick(&uptime_report_tmr);

    tum_uptime_process();

    // Check if it is time to send message
    if (tag_sw_timer_is_expired(&uptime_report_tmr)) {
        tag_sw_timer_reload(&uptime_report_tmr, TUM_TIMER_PERIOD_SEC);
        tbm_set_event(TBM_UPTIME_EVT, true);
    }
}

uint32_t tum_init(void)
{
    tag_sw_timer_reload(&uptime_report_tmr, TUM_TIMER_PERIOD_SEC_INIT);
    return 0;
}
