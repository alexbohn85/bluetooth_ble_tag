/*
 * tag_main_machine.h
 *
 * Header file
 */

#ifndef TAG_MAIN_MACHINE_H_
#define TAG_MAIN_MACHINE_H_

#include "stdio.h"
#include "stdbool.h"

//******************************************************************************
// Defines
//******************************************************************************
#define TMM_DEFAULT_TICK_PERIOD_MS                  (250)                       // T Main Period = 250 mS
#define TMM_DEFAULT_SLOW_TIMER_TICK_PERIOD_MS       (1000)                      // T Slow Period = 1000 mS

#define TMM_DEFAULT_TICK_PERIOD                     ((32768 * TMM_DEFAULT_TICK_PERIOD_MS)/1000)
#define TMM_DEFAULT_SLOW_TIMER_TICK_PERIOD          (TMM_DEFAULT_SLOW_TIMER_TICK_PERIOD_MS / TMM_DEFAULT_TICK_PERIOD_MS)
#define TMM_RTCC_CC1                                (1)

//******************************************************************************
// Extern global variables
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************

// FSM States
typedef enum tmm_states_t {
    TMM_NORMAL_MODE,
    TMM_MANUFACTURING_MODE,
    TMM_PAUSED,
    TMM_CURRENT_DRAW_MODE
} tmm_modes_t;


//TODO tag_sw_timer_t WIP it might be worth to elaborate these timers a bit more OR NOT!
typedef struct tag_sw_timer_t {
    uint32_t value;
    //uint32_t reload_value;
    //uint32_t type; //periodic, timeout.
    //bool expired; // for timeouts
    //bool running; // for periodic
} tag_sw_timer_t;


//******************************************************************************
// Interface
//******************************************************************************

//TODO debug TMM on RTCC
void tag_main_run(void);
void tmm_init(tmm_modes_t mode);

/*!
 * @brief Stop Tag Main Machine (Note: this will stop the timer)
 * @return nothing
 */
uint32_t tmm_stop(void);

/*!
 * @brief Pause Tag Main Machine (Note: timer will be running while paused)
 * @return 0 if success, otherwise failure.
 */
uint32_t tmm_pause(void);

/*!
 * @brief Resume Tag Main Machine
 * @return 0 if success, otherwise failure.
 */
uint32_t tmm_resume(void);

/*!
 * @brief Start Tag Main Machine in Manufacturing Mode State
 * @return nothing
 */
void tmm_start_manufacturing_mode(void);

/*!
 * @brief Start Tag Main Machine in Normal Mode State
 * @return nothing
 */
void tmm_start_normal_mode(void);

/*!
 * @brief Start Tag Main Machine in Normal Mode State
 * @return <uint32_t> ticks in millisecond
 */
uint32_t tmm_get_tick_period_ms(void);
uint32_t tmm_get_slow_tick_period(void);


#endif /* TAG_MAIN_MACHINE_H_ */
