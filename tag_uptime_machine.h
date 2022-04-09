/*
 * tag_uptime_machine.h
 *
 */

#ifndef TAG_UPTIME_MACHINE_H_
#define TAG_UPTIME_MACHINE_H_

//******************************************************************************
// Defines
//******************************************************************************
#define TUM_TIMER_PERIOD_SEC         (3600)  /* Sends Uptime Message every 1 hour */
#define TUM_TIMER_PERIOD_SEC_INIT    (120)   /* Init with 2 minutes */

//******************************************************************************
// Extern global variables
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Interface
//******************************************************************************
uint16_t tum_get_beacon_data(void);
void tag_uptime_run(void);
uint32_t tum_init(void);

#endif /* TAG_UPTIME_MACHINE_H_ */