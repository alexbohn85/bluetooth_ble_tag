/*******************************************************************************
 * @file
 * @brief Tag Main Machine
 *
 * Here are all the Tag functionalities state machines. Main routine "tag_main_run"
 * runs every TMM_DEFAULT_TIMER_PERIOD_MS millisecond based on RTCC timer.
 * It is cooperative multitasking which means every Machine called is responsible
 * to yield back to the main machine super loop.
 *
 */

#ifndef TAG_MAIN_MACHINE_H_
#define TAG_MAIN_MACHINE_H_

#include "stdio.h"
#include "stdbool.h"

//******************************************************************************
// Defines
//******************************************************************************
/*! @brief Tag Main Machine tick period (in mS) */
#define TMM_DEFAULT_TIMER_PERIOD_MS                 (250)

#define TMM_DEFAULT_TIMER_RELOAD                    ((32768 * TMM_DEFAULT_TIMER_PERIOD_MS)/1000)

/*! @brief Tag Main Machine slow task tick period (in mS) */
#define TMM_DEFAULT_SLOW_TIMER_PERIOD_MS            (1000)

#define TMM_DEFAULT_SLOW_TIMER_RELOAD               (TMM_DEFAULT_SLOW_TIMER_PERIOD_MS / TMM_DEFAULT_TIMER_PERIOD_MS)

/*! @brief Tag Main Machine RTCC channel */
#define TMM_RTCC_CC1                                (1)

//******************************************************************************
// Extern global variables
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************
//! @brief Tag Main Machine op. modes
typedef enum tmm_modes_t {
    TMM_NORMAL_MODE,
    TMM_MANUFACTURING_MODE,
    TMM_PAUSED,
} tmm_modes_t;

//******************************************************************************
// Interface
//******************************************************************************
void tag_main_run(void);

void tmm_pause(void);

void tmm_resume(void);

//! @brief Return TMM current mode
tmm_modes_t tmm_get_mode(void);

//! @brief Start Tag Main Machine in Manufacturing Mode State
void tmm_start_manufacturing_mode(void);

 //! @brief Start Tag Main Machine in Normal Mode State
void tmm_start_normal_mode(void);

//! @brief Get Tag Main Machine main tick period
uint32_t tmm_get_tick_period_ms(void);

//! @brief Get Tag Main Machine slow task tick period
uint32_t tmm_get_slow_tick_period(void);


#endif /* TAG_MAIN_MACHINE_H_ */
