/*
 * tag_power_manager.c
 *
 */


#include "em_common.h"
#include "stdio.h"
#include "stdbool.h"
#include "sl_power_manager.h"
#include "sl_sleeptimer.h"

#include "dbg_utils.h"
#include "lf_decoder.h"
#include "em_rtcc.h"
#include "cli.h"
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
static sl_sleeptimer_timer_handle_t tpm_sleeptimer;
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

static void tpm_delay_system_reset(sl_sleeptimer_timer_handle_t *handle, void *data)
{
    (void)(data);
    (void)(handle);

    NVIC_SystemReset();
}

/**
 * @brief Disable some features to allow for idle current draw measurements during
 *     manufacturing tests. The idea of current draw measurement is to detect for failures
 *     in assembly that could cause an increase in quiescent current, so dynamic loads
 *     need to be decreased so quiescent current variations can be detected more accurately.
 *
 *     Ultimately this could be improved to include special HW conditions.
 *
 * @param uint32_t (if 0 tag will stay in current draw mode indefinitely)
 *                 (otherwise tag will reset after timer expires)
 */
void tpm_enter_current_draw_mode(uint32_t reset_delay_ms)
{
    // This function will turn off all interrupts and put tag in EM2 mode permanently
    // until a reset is performed. (Only used for manufacturing tests)

    // Turn off LF Decoder and Disable AS393x antenna
    lf_decoder_enable(false);

    // Stop RTCC
    RTCC_Enable(false);

    // Stop CLI
    cli_stop();

    if (reset_delay_ms > 0) {
        sl_sleeptimer_start_timer_ms( &tpm_sleeptimer,
                                      reset_delay_ms,
                                      tpm_delay_system_reset,
                                      (void*)NULL,
                                      0,
                                      0);
        //printf(COLOR_B_WHITE"\n\n-> Entering current draw mode... MCU will reset in %d ms\n", reset_delay_ms);
    } else {
        //printf(COLOR_B_WHITE"\n\n-> Entering current draw mode... HW Reset exit current draw mode\n");
    }
    //printf(COLOR_RST);
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
