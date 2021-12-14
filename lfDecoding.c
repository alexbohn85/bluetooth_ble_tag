/*
 * lfDecoding.c
 *
 *  Created on: 19-Nov-2021
 *      Author: Jenish Rudani
 */

#include "lfDecoding.h"

static volatile struct LFDecoding lf;

void resetLFDecoding (void) {
  lf.isPreambleDetected = false;
  lf.isStartGapDetected = false;
  lf.ignoreNextPulse = false;
  lf.bitCounter = 0;
  lf.currentBit = 0;
  lf.previousBit = 0;
  lf.startBit = 0;
  lf.crcCalculated = 0;
  lf.LFbits = 0;
  lf.teCommand = 0;
  lf.teId = 0;
}
static volatile uint32_t lastCapturedEdge;
static volatile uint32_t measuredPeriod;
static volatile uint16_t dummyLfData;

void pauseLFDecoding () {

  RTCC_IntDisable(RTCC_IEN_CC0);      // Disabling Interrupt from RTCC CC0
  NVIC_ClearPendingIRQ(RTCC_IRQn);
  NVIC_DisableIRQ(RTCC_IRQn);
}

void resumeLFDecoding () {

  RTCC_IntEnable(RTCC_IEN_CC0);  // Enabling Interrupt from RTCC CC0
  NVIC_ClearPendingIRQ(RTCC_IRQn);
  NVIC_EnableIRQ(RTCC_IRQn);
}

uint16_t getLfData () {
  if (dummyLfData > 0) {
    return dummyLfData;
  } else {
    return 0;
  }
}

void RTCC_IRQHandler (void) {

//  uint8_t frst_bit = 0, second_bit = 0;

  // Acknowledge the interrupt
  RTCC_IntClear(RTCC_IF_CC0);

  // Read the capture value from the CC register
  uint32_t currentEdge = RTCC_ChannelCaptureValueGet(RTCC_PRS_CH);

  // Check if a capture event occurred
  if (RTCC_IF_CC0) {
    measuredPeriod = 0;
    // Calculate period in microseconds, while compensating for overflows, LFXO Freq = 32768 Hz
    measuredPeriod = (currentEdge - lastCapturedEdge) * 2000000 / 32768;
    lastCapturedEdge = currentEdge;

    if (lf.ignoreNextPulse) {
      lf.ignoreNextPulse = false;
      return;
    }

//    printf("%ld\n", measuredPeriod);

    // Check if Preamble. Timings in Micro Seconds
    if (measuredPeriod > 2500 && measuredPeriod < 5800) {
      resetLFDecoding();
      lf.isPreambleDetected = true;
      return;
    }

    if (lf.isPreambleDetected) {
      if (measuredPeriod > 1100 && measuredPeriod < 1500) {
        lf.isStartGapDetected = true;
        lf.isPreambleDetected = false;  // We reset The preamble status as if it's detected again then we start again from preamble
        return;
      } else {
        lf.isPreambleDetected = false;  // Reset the Algorithm
      }
    }

    // Start bit check
    if (lf.isStartGapDetected) {
      if (measuredPeriod >= 180 && measuredPeriod < 360) {
        lf.startBit = 1;
        lf.isStartGapDetected = false;
        return;
      } else {
        lf.isPreambleDetected = false;
      }
    }

    if (lf.startBit && lf.bitCounter <= 23) {
      if (measuredPeriod >= 180 && measuredPeriod < 360) {
        lf.currentBit = 0;
        lf.ignoreNextPulse = true;
      } else if (measuredPeriod >= 380 && measuredPeriod < 600) {
        lf.currentBit = 1;  //!(previous_bit);
      } else {
        // TODO: reset AS3930 for 15ms here
        lf.isPreambleDetected = false;
        return;
      }
      lf.previousBit = lf.currentBit;
      // if (lf.bitCounter < 17)
      // {    //   frst_bit = ((lf.crcCalculated &BIT1) ^ (current_bit & 0x01));
      //   second_bit = ((frst_bit & BIT1) ^ ((lf.crcCalculated &0x10) >> 4));
      //   lf.crcCalculated = lf.crcCalculated >> 1;
      //   lf.crcCalculated ^= (-frst_bit ^ lf.crcCalculated) &(1UL << 6);
      //   lf.crcCalculated ^= (-second_bit ^ lf.crcCalculated) &(1UL << 3);
      // }
      lf.LFbits = (lf.LFbits << 1) | (lf.currentBit & 0x01);
      if (lf.bitCounter > 22) {
        pauseLFDecoding();
        // if ((lf.LFbits &0x0000007F) == lf.crcCalculated)
        // {  printf("\n0x%04x\n", lf.LFbits);
        lf.teCommand = ((lf.LFbits >> 7) & 0x3F);
        lf.teId = ((lf.LFbits >> 13) & 0x07FF);
        dummyLfData = lf.teId;
        sl_bt_external_signal(0x1);
#if DEBUG_PRINT_SWO
        printf("0x%03x\r\n", lf.teId);
#elif DEBUG_PRINT_LCD
        printOnDisplay(lf.teId);
#endif
        resetLFDecoding();
        startLEtimer();
        return;
        // }
      }
      lf.bitCounter++;
      return;
    }
  }
}

void initRTCC () {

  RTCC_Init_TypeDef rtccInit = RTCC_INIT_DEFAULT;   // Configure the RTCC with default parameters
  rtccInit.presc = rtccCntPresc_2;                  // Prescaler 2

  RTCC_CCChConf_TypeDef rtccConfiguration = RTCC_CH_INIT_CAPTURE_DEFAULT;
  rtccConfiguration.prsSel = RTCC_PRS_CH;
  rtccConfiguration.inputEdgeSel = rtccInEdgeBoth;

  CMU_ClockSelectSet(cmuClock_RTCCCLK, RTCC_CLOCK_SOURCE);  // Setting RTCC clock source
  CMU_ClockEnable(cmuClock_RTCC, true);                     // Enable RTCC bus clock
  RTCC_ChannelInit(RTCC_PRS_CH, &rtccConfiguration);
  RTCC_Init(&rtccInit);          // Initialize and start counting
  RTCC_IntEnable(RTCC_IEN_CC0);  // Enabling Interrupt from RTCC CC0
  NVIC_ClearPendingIRQ(RTCC_IRQn);
  NVIC_EnableIRQ(RTCC_IRQn);
}
