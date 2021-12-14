/*
 * leTimer.c
 *
 *  Created on: 26-Nov-2021
 *      Author: atom
 */

#include "leTimer.h"

// Desired LETIMER frequency in Hz
#define LETIMER_FREQ 1

#define TOP_VALUE  (uint32_t)(32768 / LETIMER_FREQ)

/**************************************************************************//**
 * @brief  LETIMER Handler
 *****************************************************************************/

void LETIMER0_IRQHandler () {

#if DEBUG_PRINT
  printf("IRQ\r\n");
#endif

  uint32_t flags = LETIMER_IntGet(LETIMER0);
  // Clear LETIMER interrupt flags
  stopLEtimer();
  resumeLFDecoding();
  LETIMER_IntClear(LETIMER0, flags);
  return;
}

__attribute__((section(".ram")))
void stopLEtimer (void) {

  // Clear any previous interrupt flags
  LETIMER_IntClear(LETIMER0, _LETIMER_IF_MASK);

  // Disable underflow interrupts
  LETIMER_IntDisable(LETIMER0, LETIMER_IEN_UF);

  // Disable LETIMER interrupts
  NVIC_ClearPendingIRQ(LETIMER0_IRQn);
  NVIC_DisableIRQ(LETIMER0_IRQn);

}


void startLEtimer (void) {

  LETIMER_CounterSet(LETIMER0, 0);
  LETIMER_TopSet(LETIMER0, TOP_VALUE);
  // Clear any previous interrupt flags
  LETIMER_IntClear(LETIMER0, _LETIMER_IF_MASK);

  // Enable underflow interrupts
  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_UF);

  // Enable LETIMER interrupts
  NVIC_ClearPendingIRQ(LETIMER0_IRQn);
  NVIC_EnableIRQ(LETIMER0_IRQn);

}

void initLEtimer (void) {

  LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;

  // Enable LETIMER0 clock tree
  CMU_ClockEnable(cmuClock_LETIMER0, true);

  // Reload top on underflow, No Output Action, and run in free mode
  //  letimerInit.comp0Top = false;
  letimerInit.topValue = TOP_VALUE;
  letimerInit.repMode = letimerRepeatFree;

  // Initialize LETIMER
  LETIMER_Init(LETIMER0, &letimerInit);

  // Clear any previous interrupt flags
  LETIMER_IntClear(LETIMER0, _LETIMER_IF_MASK);

}
