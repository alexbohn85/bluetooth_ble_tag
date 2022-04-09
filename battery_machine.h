/*
 * battery.h
 *
 * Reads Battery Voltage using IADC peripheral, keeps track of sending report events
 * to Tag Beacon Machine.
 */

#ifndef BATTERY_H_
#define BATTERY_H_

#define BTM_TIMER_PERIOD_SEC         (30)  //seconds

#include "stdint.h"
#include "em_device.h"
#include "em_chip.h"
#include "em_core.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_iadc.h"
#include "dbg_utils.h"


uint32_t btm_init(void);
void battery_machine_run(void);

#endif /* BATTERY_H_ */
