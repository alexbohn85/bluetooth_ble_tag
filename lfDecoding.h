/*
 * lfDecoding.h
 *
 *  Created on: 19-Nov-2021
 *      Author: atom
 */

#ifndef LFDECODING_H_
#define LFDECODING_H_

#include "em_cmu.h"
#include "em_common.h"
#include "em_rtcc.h"
#include "printf.h"
#include "sl_status.h"
#include "sl_bluetooth.h"
#include "onBoardDisplay.h"

#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif  // SL_COMPONENT_CATALOG_PRESENT

#include "leTimer.h"
#include "allDefinesForCodeControl.h"

#define RTCC_PRS_CH 0              // RTCC PRS output channel
#define RTCC_CLOCK_SOURCE cmuSelect_LFXO // RTCC clock source

struct LFDecoding {
  bool isPreambleDetected;
  bool isStartGapDetected;
  bool ignoreNextPulse;
  uint8_t bitCounter;
  uint8_t currentBit;
  uint8_t previousBit;
  uint8_t startBit;
  uint16_t crcCalculated;
  uint32_t LFbits;
  uint16_t teCommand;
  uint16_t teId;
};

void initRTCC (void);
void resumeLFDecoding (void);
void pauseLFDecoding ();

uint16_t getLfData ();

#endif /* LFDECODING_H_ */
