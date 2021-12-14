/*
 * boot.c
 *
 */
///!----------------------------------------------------------------------------
///
///   SERMAC uses custom JLINK scripts to control RESET and UART lines
///   This allows SERMAC to execute the BOOT SELECT sequencing on BLE Tag.
///
///   Below is the documentation for J-link custom scripts:
///      -> https://wiki.segger.com/J-Link_script_files
///-----------------------------------------------------------------------------


#include "em_core.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "boot.h"
#include "sl_udelay.h"
#include "tag_defines.h"

static uint8_t boot_mode;

static void boot_select(void)
{
#if defined(TAG_BOOT_SELECT_PRESENT)
  CMU_ClockEnable(cmuClock_GPIO, true);

  //! Set pin as input add PULL-UP with glitch filter.
  GPIO_PinModeSet(BOOTSEL_B1_PORT, BOOTSEL_B1_PIN, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(BOOTSEL_B2_PORT, BOOTSEL_B2_PIN, gpioModeInputPullFilter, 1);

  //! Allow for some capacitance that may be present
  sl_udelay_wait(1000);

  //! Read pins
  uint8_t pin_1 = GPIO_PinInGet(BOOTSEL_B1_PORT, BOOTSEL_B1_PIN);
  uint8_t pin_2 = GPIO_PinInGet(BOOTSEL_B2_PORT, BOOTSEL_B2_PIN);

  boot_mode = 0;
  boot_mode = ((pin_2 << 1) | pin_1);

  do {
      sl_udelay_wait(1000);
  } while((GPIO_PinInGet(BOOTSEL_B1_PORT, BOOTSEL_B1_PIN) == 0) | (GPIO_PinInGet(BOOTSEL_B2_PORT, BOOTSEL_B2_PIN) == 0));

  if (boot_mode == BOOT_ESCAPE_HATCH) {
      // allows debugger to connect in case we are stuck in some low power mode
    __BKPT(0);
  } else {
    GPIO_PinModeSet(BOOTSEL_B1_PORT, BOOTSEL_B1_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(BOOTSEL_B2_PORT, BOOTSEL_B2_PIN, gpioModeDisabled, 0);
  }
#endif
}

uint8_t boot_read_mode(void)
{
    return boot_mode;
}

void boot_init(void)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    boot_mode = 0;
    boot_select();
    CORE_EXIT_CRITICAL();
}
