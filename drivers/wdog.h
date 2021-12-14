/*
 * wdog.h
 *
 */

#ifndef DRIVERS_WDOG_H_
#define DRIVERS_WDOG_H_

#include "em_wdog.h"

extern WDOG_TypeDef wdog_handle;
extern WDOG_Init_TypeDef wdog_init;

void watchdog_init(void);
#endif /* DRIVERS_WDOG_H_ */
