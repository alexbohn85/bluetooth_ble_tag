/*!*****************************************************************************
 * @file
 * @brief Boot
 *******************************************************************************
 *
 * This module provide means to select tag operation mode @ref boot_mode based
 * on GPIO reads during reset.
 *
 */

#include "em_core.h"
#include "em_rmu.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "sl_udelay.h"

#include "dbg_utils.h"
#include "tag_defines.h"
#include "tag_gpio_mapping.h"
#include "boot.h"


//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Global variables
//******************************************************************************
static uint8_t boot_mode;
static uint32_t reset_cause;

//******************************************************************************
// Static functions
//******************************************************************************
static void boot_select(void)
{
#if defined(TAG_BOOT_SELECT_PRESENT)
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Set pin as input add PULL-UP with glitch filter.
  GPIO_PinModeSet(BOOTSEL_B1_PORT, BOOTSEL_B1_PIN, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(BOOTSEL_B2_PORT, BOOTSEL_B2_PIN, gpioModeInputPullFilter, 1);

  // Allow for some capacitance that may be present
  sl_udelay_wait(1000);

  // Read pins
  uint8_t pin_1 = GPIO_PinInGet(BOOTSEL_B1_PORT, BOOTSEL_B1_PIN);
  uint8_t pin_2 = GPIO_PinInGet(BOOTSEL_B2_PORT, BOOTSEL_B2_PIN);

  boot_mode = 0;
  boot_mode = ((pin_2 << 1) | pin_1);

  do {
      sl_udelay_wait(1000);
  } while((GPIO_PinInGet(BOOTSEL_B1_PORT, BOOTSEL_B1_PIN) == 0) || (GPIO_PinInGet(BOOTSEL_B2_PORT, BOOTSEL_B2_PIN) == 0));

  if (boot_mode == BOOT_ESCAPE_HATCH) {
      // allows debugger to connect in case we get stuck in some low power mode
    __BKPT(0);
  } else {
    GPIO_PinModeSet(BOOTSEL_B1_PORT, BOOTSEL_B1_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(BOOTSEL_B2_PORT, BOOTSEL_B2_PIN, gpioModeDisabled, 0);
  }
#endif
}

static void _boot_print_reset_cause(const char *msg)
{
    if(dbg_is_log_enabled()) {
        DEBUG_LOG(DBG_CAT_WARNING, "%s" COLOR_RST, msg);
    } else {
        printf(COLOR_BG_RED COLOR_B_YELLOW "\n%s" COLOR_RST "\n", msg);
    }
}
//******************************************************************************
// Non Static functions
//******************************************************************************
uint32_t boot_get_reset_cause(void)
{
    return reset_cause;
}

void boot_print_reset_cause(void)
{
    // WDOG
    if (reset_cause & EMU_RSTCAUSE_WDOG0) {
        _boot_print_reset_cause("[Last Reset Cause] Watch Dog Reset EMU_RSTCAUSE_WDOG0");
    }

    // POR
    if (reset_cause & EMU_RSTCAUSE_POR) {
        _boot_print_reset_cause("[Last Reset Cause] Power On Reset EMU_RSTCAUSE_POR");
    }

    // RESET PIN
    if (reset_cause & EMU_RSTCAUSE_PIN) {
        _boot_print_reset_cause("[Last Reset Cause] Reset Pin EMU_RSTCAUSE_PIN");
    }

    // M33 LOCKUP
    if (reset_cause & EMU_RSTCAUSE_LOCKUP) {
        _boot_print_reset_cause("[Last Reset Cause] M33 Core Lockup Reset EMU_RSTCAUSE_LOCKUP");
    }

    // SYSREQ
    if (reset_cause & EMU_RSTCAUSE_SYSREQ) {
        _boot_print_reset_cause("[Last Reset Cause] M33 Core System Reset EMU_RSTCAUSE_SYSREQ");
    }

    // VDD BOD (Brown-Out Reset)
    if (reset_cause & EMU_RSTCAUSE_DVDDBOD) {
        _boot_print_reset_cause("[Last Reset Cause] Brown Out Reset EMU_RSTCAUSE_DVDDBOD");
    }

    // Wake-up from Deep Sleep (EM4) (see tag_enter_deep_sleep())
    if (reset_cause & EMU_RSTCAUSE_EM4) {
        _boot_print_reset_cause("[Last Reset Cause] Wake Up from Deep Sleep EMU_RSTCAUSE_EM4");
    }
}

uint8_t boot_get_mode(void)
{
    return boot_mode;
}

void boot_init(void)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    // Get reset cause flags
    reset_cause = RMU_ResetCauseGet();

    // Clear reset cause flags
    RMU_ResetCauseClear();

    // Get HW Boot Select Mode
    boot_mode = 0;
    boot_select();
    CORE_EXIT_CRITICAL();
}
