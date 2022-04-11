/*******************************************************************************
 * @file
 * @brief Tag Main Machine
 *
 * Here are all the Tag functionalities state machines. Main routine "tag_main_run"
 * runs every TMM_DEFAULT_TIMER_PERIOD_MS millisecond based on RTCC timer.
 * It is cooperative multitasking which means every Machine called is responsible
 * to yield back to the main machine super loop.
 */

#include <tag_status_fw_machine.h>
#include "stdio.h"
#include "stdbool.h"

#include "em_common.h"
#include "em_cmu.h"
#include "em_ramfunc.h"
#include "em_rtcc.h"
#include "em_core.h"
#include "sl_sleeptimer.h"
#include "sl_power_manager.h"
#include "sl_power_manager_debug.h"
#include "app_assert.h"


#include "dbg_utils.h"
#include "tag_power_manager.h"
#include "tag_sw_timer.h"
#include "wdog.h"
#include "lf_machine.h"
#include "tag_beacon_machine.h"
#include "temperature_machine.h"
#include "battery_machine.h"
#include "tag_uptime_machine.h"
#include "ble_manager_machine.h"
#include "tag_main_machine.h"


//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Global shared variables
//******************************************************************************
static volatile tmm_modes_t tmm_current_mode;
static volatile tmm_modes_t tmm_stored_mode;
static tag_sw_timer_t tmm_slow_timer;

//******************************************************************************
// Static functions
//******************************************************************************
/**
 *  @brief Set Tag Main Machine current state
 *  @param @ref tmm_modes_t
 */
static void tmm_set_mode(tmm_modes_t state)
{
    tmm_current_mode = state;
}

//! @brief Reload Tag Main Machine RTCC compare
static void tmm_rtcc_update(void)
{
    // Update Tag Main Machine RTCC CC1 value (for next tick)
    uint32_t timer_offset = ((uint32_t)TMM_DEFAULT_TIMER_RELOAD) + RTCC_CounterGet();
    RTCC_ChannelCompareValueSet(TMM_RTCC_CC1, timer_offset);
}

/**
 *  @brief Tag Main Machine (Slow Tasks)
 *  @details Add here tasks that can run less periodically (on seconds base)
 *  @note Tick period is defined in @ref TMM_DEFAULT_SLOW_TIMER_RELOAD
 */
static void tmm_slow_tasks_run(void)
{
    tag_sw_timer_tick(&tmm_slow_timer);

    if (tag_sw_timer_is_expired(&tmm_slow_timer)) {

        // Reload Slow Task Timer
        tag_sw_timer_reload(&tmm_slow_timer, TMM_DEFAULT_SLOW_TIMER_RELOAD);

        //**********************************************************************
        // Add below all slow tasks (tick at tmm_slow_timer)
        //**********************************************************************

        // Battery Machine Process
        //battery_machine_run();

        // Tag Firmware Revision Process
        tag_firmware_rev_run();

        // Tag Uptime Machine Process
        tag_uptime_run();

        // Temperature Machine Process
        temperature_run();

        // Tag Extended Status Machine Process
        tag_extended_status_run();
    }
}

//! @brief It calls all the machine init routines.
static uint32_t tmm_init_machines(void)
{
    uint32_t status = 0;

#if TAG_ID == UT3_ID
    lfm_init();      // LF Machine
    ttm_init();      // Tag Temp Machine
    tsm_init();      // Tag Status Machine
    tbm_init();      // Tag Beacon Machine
    bmm_init();      // BLE Manager Machine
    tum_init();      // Tag Uptime Machine
    //btm_init();    // Battery Machine
#endif

    return status;
}

//! @brief Start Tag Main Machine Process
static void tmm_init(tmm_modes_t mode)
{

    // Initiate TTM mode and machines
    tmm_set_mode(mode);
    tmm_init_machines();

    // Initiate sw timer for slow tasks machines
    tag_sw_timer_reload(&tmm_slow_timer, TMM_DEFAULT_SLOW_TIMER_RELOAD);

    // Setup RTCC CC1 Channel for Tag Main Machine SysTick
    RTCC_CCChConf_TypeDef cc1_cfg = RTCC_CH_INIT_COMPARE_DEFAULT;
    cc1_cfg.chMode = rtccCapComChModeCompare;
    cc1_cfg.compMatchOutAction = rtccCompMatchOutActionPulse;
    cc1_cfg.prsSel = 0;
    cc1_cfg.inputEdgeSel = rtccInEdgeNone;
    cc1_cfg.compBase = rtccCompBaseCnt;
    RTCC_ChannelInit(TMM_RTCC_CC1, &cc1_cfg);

    // Initiate RTCC CC1 Compare value
    uint32_t timer_offset = ((uint32_t)TMM_DEFAULT_TIMER_RELOAD) + RTCC_CounterGet();
    RTCC_ChannelCompareValueSet(TMM_RTCC_CC1, timer_offset);

    // Enable NVIC and RTCC CC1 Interrupt
    RTCC_IntClear(RTCC_IEN_CC1);
    RTCC_IntEnable(RTCC_IEN_CC1);
    NVIC_ClearPendingIRQ(RTCC_IRQn);
    NVIC_EnableIRQ(RTCC_IRQn);
}

//******************************************************************************
// Non Static functions (Interface)
//******************************************************************************
/*!  @brief Tag Main Machine running on RTCC CC Channel 1
 *   @details Tag Main Machine "super loop" entry point. This routine will be called on RTCC
 *       CC1 compare interrupt to process all the synchronous and asynchronous
 *       events. Each feature will have its own machines that will be called
 *       within this ISRs context. The main tick period is defined at @ref
 *       TMM_DEFAULT_TIMER_PERIOD_MS.
*******************************************************************************/
void tag_main_machine_isr(void)
{

    tmm_modes_t mode = tmm_get_mode();

    switch(mode)
    {
        case TMM_RUNNING:

            lfm_run();
            tmm_slow_tasks_run();

            //------------------------------------------------------------------
            // Keep below calls together in this order.
            //------------------------------------------------------------------

            // Check for all Beacon Events and dispatch messages to BLE Manager Queue
            tag_beacon_run();

            // Build packet and transmit BLE Advertising Beacons
            ble_manager_run();
            break;

        case TMM_PAUSED:
            // Do nothing...
            break;

        default:
            DEBUG_LOG(DBG_CAT_TAG_MAIN,"Error unknown mode = %d", mode);
            DEBUG_TRAP();
            break;
    }

    // Update TMM RTCC Counter Value (next tick)
    tmm_rtcc_update();

    // If BLE is not running we can skip going into main context by allowing power manager to return
    // to low power mode at the end of ISRs.
    if (bmm_adv_running) {
        tag_sleep_on_isr_exit(false);
    } else {
        tag_sleep_on_isr_exit(true);
    }

#if defined(TAG_WDOG_PRESENT)
    WDOGn_Feed(WDOG0);
#endif

}

tmm_modes_t tmm_get_mode(void)
{
    return tmm_current_mode;
}

void tmm_pause(void)
{
    tmm_stored_mode = tmm_current_mode;
    tmm_current_mode = TMM_PAUSED;
}

void tmm_resume(void)
{
    tmm_current_mode = tmm_stored_mode;
}

//! @brief Start Tag Main Machine
void tmm_start(tmm_modes_t mode)
{
    DEBUG_LOG(DBG_CAT_TAG_MAIN, "Starting Tag Main Machine process...");
    tmm_init(mode);
}

//! @brief Get Tag Main Machine main tick period
uint32_t tmm_get_tick_period_ms(void)
{
    return (uint32_t)TMM_DEFAULT_TIMER_PERIOD_MS;
}

//! @brief Get Tag Main Machine slow task tick period
uint32_t tmm_get_slow_tick_period(void)
{
    return (uint32_t)TMM_DEFAULT_SLOW_TIMER_PERIOD_MS;
}



