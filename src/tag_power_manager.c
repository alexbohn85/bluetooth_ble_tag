/*
 * tag_power_manager.c
 *
 */


#include "em_common.h"
#include "stdio.h"
#include "stdbool.h"
#include "sl_power_manager.h"
#include "sl_sleeptimer.h"
#include "rail.h"

#include "dbg_utils.h"
#include "lf_decoder.h"
#include "em_rtcc.h"
#include "cli.h"
#include "tag_power_manager.h"
#include "tag_gpio_mapping.h"


//******************************************************************************
// Defines
//******************************************************************************
#define TAG_LOWEST_POWER_MODE    (SL_POWER_MANAGER_EM2)
// RFSENSE Sync Word Length in bytes, 1-4 bytes.
#define SYNC_WORD_SIZE    (2U)
// RFSENSE Sync Word Value.
#define SYNC_WORD         (0xB16FU)

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Global variables
//******************************************************************************
RAIL_Handle_t railHandle;
static sl_sleeptimer_timer_handle_t tpm_sleeptimer;
volatile bool sleep_on_isr_exit;

// Configure the receiving node (EFR32XG22) for RF Sense.
RAIL_RfSenseSelectiveOokConfig_t config = {
  .band = RAIL_RFSENSE_2_4GHZ,
  .syncWordNumBytes = SYNC_WORD_SIZE,
  .syncWord = SYNC_WORD,
  .cb = (void*)NULL
};

//******************************************************************************
// Static functions
//******************************************************************************
static void tpm_delay_system_reset(sl_sleeptimer_timer_handle_t *handle, void *data)
{
    (void)(data);
    (void)(handle);

    NVIC_SystemReset();
}

//******************************************************************************
// Non Static functions
//******************************************************************************
void tag_check_wakeup_from_deep_sleep(void)
{
    if(RAIL_IsRfSensed(&railHandle)) {
        DEBUG_LOG(DBG_CAT_SYSTEM, "Waking up from Deep Sleep (EM4) by RFSENSE...");
    }

    // Here we can add some timeout to receive some additional command to confirm this
    // or not..
}

void tag_enter_deep_sleep(void)
{
    RAIL_StartSelectiveOokRfSense(railHandle, &config);
    sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM2);
    sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM4);
    cli_stop();
    while(1) {
        EMU_EnterEM4();
    }
}

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


void tpm_schedule_system_reset(uint32_t timeout_ms)
{
    sl_sleeptimer_start_timer_ms( &tpm_sleeptimer,
                                  timeout_ms,
                                  tpm_delay_system_reset,
                                  (void*)NULL,
                                  0,
                                  0);
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
        tpm_schedule_system_reset(reset_delay_ms);
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

void tag_power_settings(void)
{
    // Set what is the lowest power mode for the Power Manager Service can enter.
    // For instance: LF Decoder requires EM2 to run with 32KHz LFXO crystal and it needs to run all the time.
    sl_power_manager_add_em_requirement(TAG_LOWEST_POWER_MODE);

    DEBUG_LOG(DBG_CAT_SYSTEM, "Power Manager lowest power mode enabled is EM%d", TAG_LOWEST_POWER_MODE);

    // Tell Power Manager if we want to stay awake in main context or go to sleep immediately after an ISR.
    tag_sleep_on_isr_exit(true);
}
