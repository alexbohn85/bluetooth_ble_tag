/*
 * onBoardDisplay.h
 *
 *  Created on: 19-Nov-2021
 *      Author: Jenish Rudani
 */

#ifndef ONBOARDDISPLAY_H_
#define ONBOARDDISPLAY_H_

#include "glib.h"
#include "dmd.h"
#include "sl_board_control.h"
#include "em_assert.h"
#include "stdio.h"
#include "allDefinesForCodeControl.h"

void printOnDisplay(uint16_t teId);
void memlcd_app_init(void);

#endif /* ONBOARDDISPLAY_H_ */
