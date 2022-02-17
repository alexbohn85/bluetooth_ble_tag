/*
 * wdog.c
 *
 */

#include "em_cmu.h"
#include "em_wdog.h"
#include "wdog.h"

//TODO fine tune init parameters
WDOG_Init_TypeDef wdog_init =
    {
      true,                     /* Start watchdog when initialization is done. */
      true,                     /* WDOG is not counting during debug halt. */
      false,                    /* WDOG is not counting when in EM2. */
      false,                    /* WDOG is not counting when in EM3. */
      false,                    /* EM4 can be entered. */
      false,                    /* Do not lock WDOG configuration. */
      wdogPeriod_1k,            /* 1025 clock periods */
      wdogWarnDisable,          /* Disable warning interrupt. */
      wdogIllegalWindowDisable, /* Disable illegal window interrupt. */
      false                     /* Do not disable reset. */
    };


void watchdog_init(void)
{
    // Select 1000Hz clock
    CMU_ClockSelectSet(cmuClock_WDOG0CLK, cmuSelect_ULFRCO);
    // Enable clock to WDG
    CMU_ClockEnable(cmuClock_WDOG0, true);
    //TODO Implement watch dog init here (use em_wdog.h interface)
    WDOGn_Init(WDOG0, &wdog_init);
    WDOGn_Enable(WDOG0, true);
}
