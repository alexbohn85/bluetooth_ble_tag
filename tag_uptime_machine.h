/*
 * tag_uptime_machine.h
 *
 */

#ifndef TAG_UPTIME_MACHINE_H_
#define TAG_UPTIME_MACHINE_H_

//******************************************************************************
// Defines
//******************************************************************************
#if defined(TAG_DEV_MODE_PRESENT)
#define TUM_TIMER_PERIOD_SEC         (120)  /* Sends Uptime Message every 1 hour */
#define TUM_TIMER_PERIOD_SEC_INIT    (120)   /* Init with 2 minutes */
#else
#define TUM_TIMER_PERIOD_SEC         (3600)  /* Sends Uptime Message every 1 hour */
#define TUM_TIMER_PERIOD_SEC_INIT    (120)   /* Init with 2 minutes */
#endif

//******************************************************************************
// Extern global variables
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Interface
//******************************************************************************
void tum_print_timestamp(void);
void tum_get_full_uptime_string(char *buf, size_t size);
uint16_t tum_get_beacon_data(void);
void tag_uptime_run(void);
uint32_t tum_init(void);
void tum_preinit(void);

#endif /* TAG_UPTIME_MACHINE_H_ */
