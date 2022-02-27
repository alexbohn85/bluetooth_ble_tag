/*
 * tag_main_machine.c
 *
 * Tag Main Machine (super loop):
 *
 * Here are all the Tag functionalities state machines. This is a callback
 * from sleeptimer API. This timer expires every "x" mS. This is not a thread!
 * It is cooperative multitasking which means every FSM call is responsible
 * to yield back to the main machine super loop.
 *
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
#include "wdog.h"
#include "lf_machine.h"
#include "tag_beacon_machine.h"
#include "temperature_machine.h"
#include "ble_manager_machine.h"
#include "tag_main_machine.h"

//TODO remove this
//#include "em_usart.h"

#define DEBUG_TTM(format, args...)     DEBUG_LOG(DBG_CAT_TAG_MAIN, format, ##args)


//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Global shared variables
//******************************************************************************
static volatile tmm_modes_t tmm_current_mode = TMM_PAUSED;
static volatile tag_sw_timer_t tmm_slow_timer;

//******************************************************************************
// Static functions
//******************************************************************************
static void tmm_rtcc_update(void)
{
    //! Update SysTick
    uint32_t timer_offset = ((uint32_t)TMM_DEFAULT_TICK_PERIOD) + RTCC_CounterGet();
    RTCC_ChannelCompareValueSet(TMM_RTCC_CC1, timer_offset);
}

static char* tmm_mode_to_string(void)
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

static void tmm_print_info(void)
{
    DEBUG_TTM("Tag Main Machine started...");
    DEBUG_TTM("TMM current mode: %s", tmm_mode_to_string());
    DEBUG_TTM("TMM tick period: %lu mS", tmm_get_tick_period_ms());
    DEBUG_TTM("TMM slow tick period: %lu mS", tmm_get_slow_tick_period());
}

/*!
 *  @brief Reload Tag Main Slow Timer value
 */
static void tmm_slow_timer_reload(void)
{
    tmm_slow_timer.value = (uint32_t)TMM_DEFAULT_SLOW_TIMER_TICK_PERIOD;
}

/*!
 *  @brief Update Tag Main Slow Timer value
 */
static void tmm_slow_timer_tick(void)
{
    if (tmm_slow_timer.value > 0) {
        --tmm_slow_timer.value;
    }
}

/*!
 *  @brief Tag Main Slow Timer Machine
 *  @detail Add here tasks that can run less periodically
 */
static void tmm_slow_tasks_run(void)
{
    tmm_slow_timer_tick();

    if (tmm_slow_timer.value == 0) {
        tmm_slow_timer_reload();

        //! Add machines here
        temperature_run();
    }
}

/*!
 *  @brief Get Tag Main Machine current state
 *  @return tmm_states_t
 */
static tmm_modes_t tmm_get_mode(void)
{
    return tmm_current_mode;
}

/*!
 *  @brief Set Tag Main Machine state
 */
static void tmm_set_mode(tmm_modes_t state)
{
    tmm_current_mode = state;
}


/*!  @brief Tag Main Machine running on RTCC CC Channel 1 *
 *   @details Here is the "entry point" of the Tag Main Process this routine
 *   will be called on RTCC CC1 compare interrupt to process all the synchronous
 *   and asynchronous events by modules or machines responsible for a certain
 *   feature.
 *
 *   @note
 *   We call all the features state machine here keep in mind that you have
 *   the total control here and not every single state machine needs to run
 *   every single time, unless they have something to do.

 *   For example: update sw timers (in case they are implemented
 *   inside each module).

 *   The order of the state machine calls also matter, you may want to first
 *   check the asynchronous events before calling tag beacon machine
 *   to start transmitting messages. Making wrong choices here will affect
 *   overall system responsiveness and power consumption.

 *   @param[in] nothing
 *   @return nothing
*******************************************************************************/
void tag_main_run(void)
{

    tmm_modes_t mode = tmm_get_mode();

    //!DEBUG Power Manager
    //sl_power_manager_debug_print_em_requirements();

    switch(mode)
    {
        case TMM_NORMAL_MODE:
            tmm_slow_tasks_run();
            lf_run();
            //tag_beacon_run();
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

    //! Update next tick
    tmm_rtcc_update();

    //! TODO Need better understanding and decide which module is responsible for this. Keep it here for now
    //! Check if we still need HFXO
    if (bmm_adv_running) {
        tag_sleep_on_isr_exit(false);
    } else {
        tag_sleep_on_isr_exit(true);
    }

#if defined(TAG_WDOG_PRESENT)
    //TODO Add the warning IRQ handler to print some debug info before reset
    //TODO Do a burn in test
    WDOGn_Feed(WDOG0);
#endif

}

static uint32_t tmm_init_machines(void)
{
    volatile uint32_t status;

    //! Add here all the machines init functions
#if (TAG_TYPE == TAG_UT3)
    status = lfm_init();
    status = ttm_init();
    status = bmm_init();
#endif

    return status;
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

    //! Initiate TMM Timer handle
    //! Note: This timer is based on a HW Timer and it defines the "time"
    //! perception for all the machines running inside Tag Main Machine context.
    //! Which means all machines SW timers are in sync with this tick.
    tmm_timer_handle = &tmm_sleeptimer;

    status = sl_sleeptimer_is_timer_running(tmm_timer_handle, &running);

    if (status) {
        DEBUG_TTM("Error status = %lu", status);
        DEBUG_TRAP();
    }

    if (!running) {
        //! Start TMM Timer
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
            //! Start initializing all the machines (call init functions)
            tmm_set_mode(mode);
            tmm_init_machines();
            tmm_print_info();
        }

    } else {
        //! Start initializing all the machines (call init functions)
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

void tmm_start_normal_mode(void)
{
    DEBUG_TTM("Starting Tag Main Machine in NORMAL MODE...");
    tmm_init(TMM_NORMAL_MODE);
}

void tmm_start_manufacturing_mode(void)
{
    DEBUG_TTM("Starting Tag Main Machine in MANUFACTURING MODE...");
    tmm_init(TMM_MANUFACTURING_MODE);
}

uint32_t tmm_get_tick_period_ms(void)
{
    return (uint32_t)TMM_DEFAULT_TICK_PERIOD_MS;
}

uint32_t tmm_get_slow_tick_period(void)
{
    return (uint32_t)TMM_DEFAULT_SLOW_TIMER_TICK_PERIOD_MS;
}

/*!
 * @brief Start Tag Main Machine Process
 */
void tmm_init(tmm_modes_t mode)
{

    //! Initiate TTM mode and machines
    tmm_set_mode(mode);
    tmm_init_machines();

    //! Setup RTCC CC1 Channel for Tag Main Machine SysTick
    RTCC_CCChConf_TypeDef cc1_cfg = RTCC_CH_INIT_COMPARE_DEFAULT;
    cc1_cfg.chMode = rtccCapComChModeCompare;
    cc1_cfg.compMatchOutAction = rtccCompMatchOutActionPulse;
    cc1_cfg.prsSel = 0;
    cc1_cfg.inputEdgeSel = rtccInEdgeNone;
    cc1_cfg.compBase = rtccCompBaseCnt;
    RTCC_ChannelInit(TMM_RTCC_CC1, &cc1_cfg);

    //! Initiate RTCC CC1 Compare value
    uint32_t timer_offset = ((uint32_t)TMM_DEFAULT_TICK_PERIOD) + RTCC_CounterGet();
    RTCC_ChannelCompareValueSet(TMM_RTCC_CC1, timer_offset);

    //! Enable NVIC and RTCC CC1 Interrupt
    RTCC_IntClear(RTCC_IEN_CC1);
    RTCC_IntEnable(RTCC_IEN_CC1);
    NVIC_ClearPendingIRQ(RTCC_IRQn);
    NVIC_EnableIRQ(RTCC_IRQn);
    RTCC_Start();
}
