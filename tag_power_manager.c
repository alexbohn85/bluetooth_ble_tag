/*
 * tag_power_manager.c
 *
 */


#include "em_common.h"
#include "stdio.h"
#include "stdbool.h"
#include "sl_power_manager.h"

#include "dbg_utils.h"
#include "tag_gpio_mapping.h"



//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Global variables
//******************************************************************************
volatile bool sleep_on_isr_exit;

//******************************************************************************
// Static functions
//******************************************************************************

//******************************************************************************
// Non Static functions
//******************************************************************************
/**
 * @brief Called right before sleep
 */
void EMU_EFPEM23PresleepHook(void)
{
    /** Add debug code here.. **/
}

/**
 * @brief Called right after wake up
 */
void EMU_EFPEM23PostsleepHook(void)
{
    /** Add debug code here.. **/
}

/**
 * @brief Enable/Disable going to sleep at an ISR exit.
 * @details If false it means that application will continue running on main context after ISR exit.
 * @param enable
 */
void tag_sleep_on_isr_exit(bool enable)
{
    sleep_on_isr_exit = enable;
    //DEBUG_LOG(DBG_CAT_SYSTEM, "tag_sleep_on_isr_exit = %s", (sleep_on_isr_exit ? "True" : "False"));
}

/**
 * @brief Defines 'Power Manager' behavior after exiting an ISR
 * @details This behavior needs to be dynamic during runtime to allow BLE API to
 *     run in main context only when needed. The main reason we need to decouple
 *     ISRs from BLE API is because BLE Stack requires HFXO clock which takes
 *     too much time to start-up coming from EM2 mode, if we just use HFXO all the
 *     time ISRs like LF Decoder would consume too much power and its performance
 *     would decay due to long wake-up time to restore HFXO clock, same for Tag
 *     Main Machine.
 */
sl_power_manager_on_isr_exit_t app_sleep_on_isr_exit(void)
{
    if (sleep_on_isr_exit) {
        return SL_POWER_MANAGER_SLEEP;
    } else {
        return SL_POWER_MANAGER_WAKEUP;
    }
}
