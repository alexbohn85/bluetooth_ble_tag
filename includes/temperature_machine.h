/*
 * temperature.h
 *
 *  This module runs from Tag Main Machine ISR context.
 *
 *  Reads Sensor Temperature every "TTM_TIMER_PERIOD_SEC" seconds and Sends Temperature
 *  Beacon Events to Tag Beacon Machine.
 *
 */

#ifndef TEMPERATURE_MACHINE_H_
#define TEMPERATURE_MACHINE_H_

#if defined(TAG_DEV_MODE_PRESENT)
#define TTM_REPORT_TIMER_RELOAD             (600)   /* Tag Temperature Message is (triggered) every 10 minute */
#else
#define TTM_REPORT_TIMER_RELOAD             (1800)  /* Tag Temperature Message is (triggered) every 30 minutes */
#endif

#define TTM_READ_TIMER_RELOAD               (600)   /* Read Temperature Sensor every 10 minutes */

//******************************************************************************
// Interface
//******************************************************************************
int8_t ttm_get_current_temperature(void);
void temperature_run(void);
uint32_t ttm_init(void);

#endif /* TEMPERATURE_MACHINE_H_ */
