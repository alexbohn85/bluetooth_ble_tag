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

#if 0
/*!
 * @brief Start Tag Main Machine Timer (DEPRECATED)
 * @return 0 if success, otherwise failure.
 */
static uint32_t tmm_init(tmm_modes_t mode)
{
    volatile uint32_t status;
    bool running;

    // Initiate TMM Timer handle
    // Note: This timer is based on a HW Timer and it defines the "time"
    // perception for all the machines running inside Tag Main Machine context.
    // Which means all machines SW timers are in sync with this tick.
    tmm_timer_handle = &tmm_sleeptimer;

    status = sl_sleeptimer_is_timer_running(tmm_timer_handle, &running);

    if (status) {
        DEBUG_TTM("Error status = %lu", status);
        DEBUG_TRAP();
    }

    if (!running) {
        // Start TMM Timer
        status = sl_sleeptimer_start_periodic_timer_ms(tmm_timer_handle,
                                                       tmm_tick_period_ms,
                                                       tag_main_run,
                                                       (void*)NULL,
                                                       5,
                                                       SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
        if (status) {
            DEBUG_TTM("Error status = %lu", status);
            DEBUG_TRAP();
        } else {
            // Start initializing all the machines (call init functions)
            tmm_set_mode(mode);
            tmm_init_machines();
            tmm_print_info();
        }

    } else {
        // Start initializing all the machines (call init functions)
        tmm_set_mode(mode);
        tmm_init_machines();
        tmm_print_info();
        DEBUG_TTM("Tag Main Machine is already running...");
    }
    return status;
}
#endif

//******************************************************************************
// Non Static functions (Interface)
//******************************************************************************
/*!  @brief Tag Main Machine running on RTCC CC Channel 1
 *   @details Tag Main Machine entry point. This routine will be called on RTCC
 *       CC1 compare interrupt to process all the synchronous and asynchronous
 *       events. Each feature will have its own machines that will be called
 *       within this ISRs context. The main tick period is defined at @ref
 *       TMM_DEFAULT_TIMER_PERIOD_MS. *
 *   @note The concept of machines here are just like "tasks". We are using "machine"
 *       term to avoid confusion with RTOS-like tasks. These machines here are running
 *       periodically just like in an RTOS scheduler but with the big difference that
 *       we do not have a preemptive scheduler to handle process switches. This is an
 *       cooperative multitasking where each machine has full control of the CPU time.
 *       So it is up to each machine to make fair use of CPU time.
 *   @note The concept of elapsed time is based on simple counters (@ref tag_sw_timer_t).
 *       Since we are running on a hardware timer ISR with precise clock we can assume
 *       this is a deterministic event and "precise enough" to measure elapsed time.
 *       Maximum jitter would be defined by the worst case scenario of ISRs in the
 *       system. This is not trivial to calculate and it is better to just make sure
 *       ISRs events are prioritized and completed as fast as possible. We sacrifice
 *       precision with simplicity and power consumption, this is what tag application
 *       really needs.
 *   @note Another important concept of Tag Main Machine is the ordering of machine calls.
 *       Machines should be processed in an order that maximizes work per tick to
 *       improve overall responsiveness of the tag.
 *   @note \b Example 1\b: You may want to process Tag Temperature Machine (TTM) before calling
 *       Tag Beacon Machine (TBM), if you do the opposite BLE transmission of that event
 *       would only happen on the next main tick.
 *   @note \b Example 2\b: If at a button press event you have some LEDs or Buzzer indication,
 *       you may want to process a Push-Button Machine before process Annunciation
 *       Machine (LEDs, Buzzer) so the responsiveness of this chained events are reduced
 *       to the worst-case tick period @ref TMM_DEFAULT_TIMER_PERIOD_MS. *
 *   @note Making wrong choices in ordering will affect overall system responsiveness and
 *       power consumption.
*******************************************************************************/
void tag_main_run(void)
{

    tmm_modes_t mode = tmm_get_mode();

    //DEBUG Power Manager
    //sl_power_manager_debug_print_em_requirements();

    switch(mode)
    {
        case TMM_NORMAL_MODE:
            tmm_slow_tasks_run();
            lf_run();
            tag_beacon_run();
            ble_manager_run();
            break;

        case TMM_MANUFACTURING_MODE:
            // to be implemented
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

    // TODO Need better understanding and decide which module is responsible for this. Keep it here for now
    // Check if we still need to go to main after this ISR
    // Wrap this in a function and call at the end of every ISR
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

#if 0
uint32_t tmm_stop(void)
{
    volatile uint32_t status;

    if (tmm_timer_handle == NULL) {
        status = 1;
        DEBUG_TRAP();
    } else {
        status = sl_sleeptimer_stop_timer(tmm_timer_handle);
        if (status != 0) {
            DEBUG_TRAP();
        } else {
            tmm_timer_handle = NULL;
        }
    }

    return status;
}
#endif

#if 0
uint32_t tmm_pause(void)
{
    volatile uint32_t status;

    bool running;
    status = sl_sleeptimer_is_timer_running(tmm_timer_handle, &running);

    if (running)
    {
        uint8_t mode = tmm_get_mode();
        if (mode != TMM_PAUSED) {
            tmm_resume_mode = mode;
            tmm_set_mode(TMM_PAUSED);
            status = 0;
        }
    } else {
        DEBUG_TRAP();
        status = 1;
    }

    return status;
}

uint32_t tmm_resume(void)
{
    volatile uint32_t status;

    bool running;
    status = sl_sleeptimer_is_timer_running(tmm_timer_handle, &running);

    if (running) {
        if (tmm_get_mode() == TMM_PAUSED) {
            tmm_set_mode(tmm_resume_mode);
            status = 0;
        }
    } else {
        DEBUG_TRAP();
        status = 1;
    }

    return status;
}
#endif

/*!
 *  @brief Get Tag Main Machine current state
 *  @return @ref tmm_states_t
 */
tmm_modes_t tmm_get_mode(void)
{
    return tmm_current_mode;
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



