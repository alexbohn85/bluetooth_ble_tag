/*
 * tag_power_manager.h
 *
 */

#ifndef TAG_POWER_MANAGER_H_
#define TAG_POWER_MANAGER_H_

#include "stdio.h"
#include "stdbool.h"

//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Extern global variables
//******************************************************************************
extern volatile bool sleep_on_isr_exit;

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Interface
//******************************************************************************
void tag_sleep_on_isr_exit(bool enable);
void tpm_schedule_system_reset(uint32_t timeout_ms);
void tpm_enter_current_draw_mode(uint32_t reset_delay_ms);
void tag_enter_deep_sleep(void);
void tag_check_wakeup_from_deep_sleep(void);
void tag_power_settings(void);

#endif /* TAG_POWER_MANAGER_H_ */
