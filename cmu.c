/*
 * cmu.c
 *
 *  Created on: 26-Nov-2021
 *      Author: atom
 */

#include "cmu.h"

/**************************************************************************//**
 * @brief Clock initialization
 *****************************************************************************/
void selectClock (void) {
  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_FSRCO);
#if defined(_CMU_EM01GRPACLKCTRL_MASK)
  CMU_ClockSelectSet(cmuClock_EM01GRPACLK, cmuSelect_FSRCO);
#endif
#if defined(_CMU_EM01GRPBCLKCTRL_MASK)
  CMU_ClockSelectSet(cmuClock_EM01GRPBCLK, cmuSelect_FSRCO);
#endif
  CMU_ClockSelectSet(cmuClock_EM23GRPACLK, cmuSelect_LFXO);
  CMU_ClockSelectSet(cmuClock_EM4GRPACLK, cmuSelect_LFXO);
#if defined(RTCC_PRESENT)
  CMU_ClockSelectSet(cmuClock_RTCC, cmuSelect_LFXO);
#endif

}

