/*******************************************************************************
 * @file
 * @brief Tag Main Machine
 *
 * Here are all the Tag functionalities state machines. Main routine "tag_main_run"
 * runs every TMM_DEFAULT_TIMER_PERIOD_MS millisecond based on RTCC timer.
 * It is cooperative multitasking which means every Machine called is responsible
 * to yield back to the main machine super loop.
 */


#include "em_common.h"
#include "em_cmu.h"
#include "em_ramfunc.h"
#include "em_rtcc.h"

#include "em_core.h"
#include "app_assert.h"
#include "sl_sleeptimer.h"
#include "sl_power_manager.h"
#include "sl_power_manager_debug.h"

#include "stdio.h"
#include "stdbool.h"

#include "dbg_utils.h"
#include "tag_sw_timer.h"
#include "wdog.h"
#include "lf_machine.h"
#include "tag_beacon_machine.h"
#include "tag_status_machine.h"
#include "temperature_machine.h"
#include "ble_manager_machine.h"
#include "tag_main_machine.h"


//******************************************************************************
// Defines
//******************************************************************************
//! @brief DEBUG_LOG macro for Tag Main Machine
#define DEBUG_TTM(format, args...)     DEBUG_LOG(DBG_CAT_TAG_MAIN, format, ##args)

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Global shared variables
//******************************************************************************
//! @brief Tag Main Machine current op. mode
static volatile tmm_modes_t tmm_current_mode;
static volatile tmm_modes_t tmm_stored_mode;

//! @brief SW timer for slow tasks.
static tag_sw_timer_t tmm_slow_timer;

//******************************************************************************
// Static functions
//******************************************************************************
//! @brief Convert tmm_current_mode state to string.
static char* tmm_current_mode_to_string(void)
{
    switch(tmm_current_mode) {
        case TMM_NORMAL_MODE:
            return "Normal Mode";
            break;
        case TMM_MANUFACTURING_MODE:
            return "Manufacturing Mode";
            break;
        case TMM_PAUSED:
            return "Paused";
            break;
        default:
            return "Unknown";
            break;
    }
}

//! @brief Reload Tag Main Machine RTCC compare. a.k.a "Tag Main SysTick"
static void tmm_rtcc_update(void)
{
    //! Update SysTick
    uint32_t timer_offset = ((uint32_t)TMM_DEFAULT_TIMER_RELOAD) + RTCC_CounterGet();
    RTCC_ChannelCompareValueSet(TMM_RTCC_CC1, timer_offset);
}

/*!
 *  @brief Tag Main Machine (Slow Tasks)
 *  @details Add here tasks that can run less periodically (on seconds base)
 *  @note Tick period is defined in @ref TMM_DEFAULT_SLOW_TIMER_RELOAD
 */
static void tmm_slow_tasks_run(void)
{
    tag_sw_timer_tick(&tmm_slow_timer);

    if (tag_sw_timer_is_expired(&tmm_slow_timer)) {
        tag_sw_timer_reload(&tmm_slow_timer, TMM_DEFAULT_SLOW_TIMER_RELOAD);

        //! Add slower machines here
        temperature_run();
        tag_status_run();
    }
}

/*!
 *  @brief Set Tag Main Machine current state
 *  @param @ref tmm_modes_t
 */
static void tmm_set_mode(tmm_modes_t state)
{
    tmm_current_mode = state;
}

//! @brief It calls all the machine init routines.
static uint32_t tmm_init_machines(void)
{
    volatile uint32_t status;

    // Add here all the machines init functions
#if (TAG_TYPE == TAG_UT3_ID)
    status = lfm_init();  //! LF Machine
    status = ttm_init();  //! Tag Temp Machine
    status = tsm_init();  //! Tag Status Machine
    status = tbm_init();  //! Tag Beacon Machine
    status = bmm_init();  //! BLE Manager Machine
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
 *   @details Tag Main Machine entry point. This routine will be called on RTCC
 *       CC1 compare interrupt to process all the synchronous and asynchronous
 *       events. Each feature will have its own machines that will be called
 *       within this ISRs context. The main tick period is defined at @ref
 *       TMM_DEFAULT_TIMER_PERIOD_MS.
*******************************************************************************/
void tag_main_run(void)
{

    tmm_modes_t mode = tmm_get_mode();

    switch(mode)
    {
        case TMM_NORMAL_MODE:
            tmm_slow_tasks_run();
            lf_run();
            tag_beacon_run();
            ble_manager_run();
            break;

        case TMM_MANUFACTURING_MODE:
            // to be implemented (if needed)
            break;

        case TMM_PAUSED:
            // Do nothing...
            break;

        default:
            DEBUG_TTM("Error unknown mode = %lu", (uint32_t)mode);
            DEBUG_TRAP();
            break;
    }

    // Update next tick
    tmm_rtcc_update();

    if (bmm_adv_running) {
        tag_sleep_on_isr_exit(false);
    } else {
        tag_sleep_on_isr_exit(true);
    }

#if defined(TAG_WDOG_PRESENT)
    //TODO Do a burn in test
    WDOGn_Feed(WDOG0);
#endif

}

/*!
 *  @brief Get Tag Main Machine current state
 *  @return @ref tmm_modes_t
 */
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

//! @brief Start Tag Main Machine in @ref TMM_NORMAL_MODE
void tmm_start_normal_mode(void)
{
    DEBUG_TTM("Starting Tag Main Machine in NORMAL MODE...");
    tmm_init(TMM_NORMAL_MODE);
}

//! @brief Start Tag Main Machine in @ref TMM_MANUFACTURING_MODE
void tmm_start_manufacturing_mode(void)
{
    DEBUG_TTM("Starting Tag Main Machine in MANUFACTURING MODE...");
    tmm_init(TMM_MANUFACTURING_MODE);
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



