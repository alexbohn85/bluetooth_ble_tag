/*
 * wdog.c
 *
 */

#include "em_cmu.h"
#include "em_core.h"
#include "em_wdog.h"
#include "sl_sleeptimer.h"
#include "sl_power_manager.h"

#include "dbg_utils.h"
#include "wdog.h"

#define WDOG_DEBUG

static sl_sleeptimer_timer_handle_t wdog_timer;

WDOG_Init_TypeDef wdog_init =
    {
      true,                     /* Start watchdog when initialization is done. */
      true,                     /* WDOG is not counting during debug halt. */
      true,                     /* WDOG is not counting when in EM2. */
      true,                     /* WDOG is not counting when in EM3. */
      false,                    /* EM4 can be entered. */
      false,                    /* Do not lock WDOG configuration. */
      wdogPeriod_1k,            /* 1025 clock periods */
      wdogWarnTime75pct,        /* Enable warning interrupt. */
      wdogIllegalWindowDisable, /* Disable illegal window interrupt. */
      true                      /* Disable Watchdog reset output if true */
    };

WDOG_Init_TypeDef wdog_init_reset =
    {
      true,                     /* Start watchdog when initialization is done. */
      true,                     /* WDOG is not counting during debug halt. */
      true,                     /* WDOG is not counting when in EM2. */
      true,                     /* WDOG is not counting when in EM3. */
      false,                    /* EM4 can be entered. */
      false,                    /* Do not lock WDOG configuration. */
      wdogPeriod_9,             /* 9 clock periods */
      wdogWarnDisable,          /* Disable warning interrupt. */
      wdogIllegalWindowDisable, /* Disable illegal window interrupt. */
      false                     /* Enable Watchdog reset output if true */
    };

void wdog_reset_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
    (void)(data);
    (void)(handle);

    DEBUG_LOG(DBG_CAT_WARNING, "Watch Dog reset...");
    WDOGn_Init(WDOG0, &wdog_init_reset);
    WDOGn_Enable(WDOG0, true);
}

void WDOG0_IRQHandler(void)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
    uint32_t irq_flag;

    irq_flag = WDOGn_IntGet(WDOG0);
    WDOGn_IntClear(WDOG0, (irq_flag & (WDOG_IF_WARN | WDOG_IF_TOUT)));

    CORE_EXIT_ATOMIC();

    if (irq_flag & WDOG_IF_WARN) {
        DEBUG_LOG(DBG_CAT_WARNING, "Watch Dog 75%% warning...");
        // Add here some prints of store data for analysis.
    }

    if (irq_flag & WDOG_IF_TOUT) {
        DEBUG_LOG(DBG_CAT_WARNING, "Watch Dog timeout expired, reset in 1 sec...");

        // Disable watch dog
        WDOGn_Enable(WDOG0, false);

        // Schedule a wdog reset with 1s delay.
        sl_sleeptimer_start_timer_ms(&wdog_timer,
                                     1000,
                                     wdog_reset_callback,
                                     (void*)NULL,
                                     0,
                                     SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
    }

}

void watchdog_init(void)
{

#if defined(WDOG_DEBUG)
    dbg_log_enable(true);
#endif

    // Select 1000Hz clock
    CMU_ClockSelectSet(cmuClock_WDOG0CLK, cmuSelect_ULFRCO);
    // Enable clock to WDG
    CMU_ClockEnable(cmuClock_WDOG0, true);

    // Init WDOG, timeout set to 1 second and warning at 75%.
    WDOGn_Init(WDOG0, &wdog_init);

    // Enable WDOG Warning Interrupt
    WDOGn_IntClear(WDOG0, WDOG_IF_WARN | WDOG_IF_TOUT);
    WDOGn_IntEnable(WDOG0, WDOG_IF_WARN | WDOG_IF_TOUT);
    NVIC_ClearPendingIRQ(WDOG0_IRQn);
    NVIC_EnableIRQ(WDOG0_IRQn);

    // Enable WDOG
    WDOGn_Enable(WDOG0, true);
}
