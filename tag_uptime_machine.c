/*
 * tag_uptime_machine.c
 *
 */

#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "sl_sleeptimer.h"

#include "tag_sw_timer.h"
#include "boot.h"
#include "tag_beacon_machine.h"
#include "dbg_utils.h"
#include "tag_uptime_machine.h"

//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************
typedef struct tum_uptime_t {
    uint16_t days;
    uint8_t hours;
    uint8_t mins;
    uint8_t secs;
} tum_uptime_t;

//******************************************************************************
// Global variables
//******************************************************************************
static tag_sw_timer_t uptime_report_tmr;
// Keep this in .custom section to avoid initialization during c startup.
tum_uptime_t __attribute__ ((section(".custom"))) uptime;

//******************************************************************************
// Static functions
//******************************************************************************
static void tum_uptime_process(void)
{
    uptime.secs++;

    if (uptime.secs == 60) {
        uptime.secs = 0;
        uptime.mins++;

        if (uptime.mins == 60) {
            uptime.mins = 0;
            uptime.hours++;

            if (uptime.hours == 24) {
                uptime.hours = 0;
                uptime.days++;
            }
        }
    }
}

//******************************************************************************
// Non Static functions
//******************************************************************************
void tum_print_timestamp(void)
{
    printf("\n%.2d:%.2d:%.2d ", uptime.hours, uptime.mins, uptime.secs);
}

void tum_get_full_uptime_string(char *buf, size_t size)
{
    snprintf(buf, size, "%d days %.2d:%.2d:%.2d ", (int)uptime.days, (int)uptime.hours, (int)uptime.mins, (int)uptime.secs);
}

uint16_t tum_get_beacon_data(void)
{
    return uptime.days;
}

/**
 * @brief Tag Uptime Machine
 * @details Keep track of number of days running, Sends Uptime Report Event to Tag Beacon Machine
 */
void tag_uptime_run(void)
{
    tag_sw_timer_tick(&uptime_report_tmr);

    tum_uptime_process();

    // Check if it is time to report uptime message.
    if (tag_sw_timer_is_expired(&uptime_report_tmr)) {
        tag_sw_timer_reload(&uptime_report_tmr, TUM_TIMER_PERIOD_SEC);

        // Send Async Tag Uptime Beacon
        //tbm_set_event(TBM_UPTIME_EVT, true);

        // Send Sync Tag Uptime Beacon
        tbm_set_event(TBM_UPTIME_EVT, false);
    }
}

uint32_t tum_init(void)
{
    tag_sw_timer_reload(&uptime_report_tmr, TUM_TIMER_PERIOD_SEC_INIT);
    return 0;
}

/**
 * @brief Initialize uptime only when Power-On Reset happens otherwise it would not mean anything.
 * @details 'uptime' is placed in RAM section .custom so it will not be zeroed during c startup.
 */
void tum_preinit(void)
{
    // If this is a Power On Reset then clear 'uptime'
    if (boot_get_reset_cause() & EMU_RSTCAUSE_POR) {
        memset(&uptime, 0, sizeof(uptime));
    }
}
