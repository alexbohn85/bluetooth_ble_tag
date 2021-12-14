/*
 * main.c
 *
 *  Created on: 01-Nov-2021
 *      Author: Jenish Rudani
 */

#include <stdio.h>
#include "em_gpio.h"
#include "em_prs.h"
#include "em_ramfunc.h"
#include "em_timer.h"
#include "em_letimer.h"

#include "sl_component_catalog.h"
#include "sl_power_manager.h"
#include "sl_system_init.h"
#include "sl_system_process_action.h"
#include "app.h"


#include "lfDecoding.h"
#include "onBoardDisplay.h"
#include "allDefinesForCodeControl.h"
#include "cmu.h"

#define CAPTURE_INPUT_PORT gpioPortA
#define CAPTURE_INPUT_PIN 7
#define GPIO_PRS_CHANNEL 0

void setupGPIO (void) {
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinModeSet(CAPTURE_INPUT_PORT, CAPTURE_INPUT_PIN, gpioModeInput, 0);
  GPIO_IntConfig(CAPTURE_INPUT_PORT, CAPTURE_INPUT_PIN, false, false, false);
}


void initPRS (void) {
  CMU_ClockEnable(cmuClock_PRS, true);
  PRS_SourceAsyncSignalSet(GPIO_PRS_CHANNEL, PRS_ASYNC_CH_CTRL_SOURCESEL_GPIO, CAPTURE_INPUT_PIN);
  PRS_ConnectConsumer(GPIO_PRS_CHANNEL, prsTypeAsync, prsConsumerRTCC_CC0);   // Select PRS channel for RTCC CC0
}

void printCoreClockSpeed (void) {
  uint32_t temp = CMU_ClockFreqGet(cmuClock_CORE);
  printf("Clock Core: %ld MHz \r\n", temp);
}

int main (void) {

  sl_system_init();
  app_init();
//  selectClock();
  setupGPIO();
  initPRS();
  initRTCC();
  initLEtimer();

#if DEBUG_PRINT_SWO
  printCoreClockSpeed();
#elif DEBUG_PRINT_LCD
  memlcd_app_init();
#endif

  while (1) {
    sl_system_process_action();
    sl_power_manager_sleep();
  }
}
