/*
 * lf_machine.c
 *
 *  This module runs from Tag Main Machine ISR context.
 *
 *  This module track LF Decoder decoded data and provide logic for Entering, Staying
 *  and Exiting LF Field feature and report to Tag Beacon Machine.
 *
 */

#include "em_common.h"
#include "sl_spidrv_instances.h"
#include "stdio.h"
#include "stdbool.h"

#include "dbg_utils.h"
#include "tag_sw_timer.h"
#include "as393x.h"
#include "tag_main_machine.h"
#include "tag_beacon_machine.h"
#include "lf_decoder.h"
#include "lf_machine.h"


//******************************************************************************
// Defines
//******************************************************************************

// LF Commands definitions
#define TA_CONFIRM_CMD_N_TIMES                 3       /* Protocol to confirm TA command, tag needs to receive
                                                          same command "n" times in a row before it can execute */
#define LF_CMD_NOP                             (0x00)
#define LF_CMD_MT_BATT_LOW                     (0x1E)
#define LF_CMD_MT                              (0x1F)
#define LF_CMD_EXAMPLE                         (0xFF)

// LF Exit timer
#define LFM_TIMER_A_PERIOD_MS                  3000
#define LFM_TIMER_A_PERIOD_RELOAD              (LFM_TIMER_A_PERIOD_MS / TMM_RTCC_TIMER_PERIOD_MS)

//******************************************************************************
// Data types
//******************************************************************************
enum lfm_exciter_types_t {
    EXCITER_TE_PTE,
    EXCITER_MOTHER_TAG,
};

typedef enum lfm_states_t {
    INIT,
    CHECK_EXIT_TIMEOUT,
    PROCESS_TAG_IN_FIELD,
    DECODE_COMMAND,
    EXIT
} lfm_states_t;

typedef struct lfm_data_t {
    uint8_t status;
    uint8_t ta_cmd_counter;      /* used to handle Tag Activator command confirmation protocol */
    uint8_t ta_cmd;              /* used to handle Tag Activator command confirmation protocol */
    lf_decoder_data_t buffer_0;  /* we use this to save new lf data */
    lf_decoder_data_t buffer_1;  /* we use this to store previous lf data */
} lfm_data_t;

typedef struct lfm_fsm_t {
    lfm_states_t state;
    lfm_states_t return_state;
} lfm_fsm_t;

//******************************************************************************
// Global variables
//******************************************************************************
static tag_sw_timer_t timer_exit_field;
static lfm_data_t lfm_data;
static volatile lfm_fsm_t lfm_fsm;
static volatile bool lfm_running;
static lfm_lf_beacon_t lf_beacon_data;

//******************************************************************************
// Static functions
//******************************************************************************
static void lfm_clear_status_flag(uint8_t flag)
{
    lfm_data.status &= ~(flag);
}

static void lfm_set_status_flag(uint8_t flag)
{
    lfm_data.status |= flag;
}

static void lfm_clear_ta_cmd(void)
{
    lfm_data.ta_cmd = 0;
    lfm_data.ta_cmd_counter = 0;
}

static void lfm_dispatch_command(uint8_t lf_command)
{
    switch(lf_command) {
        //TODO add here calls to execute tag commands
        case LF_CMD_EXAMPLE:
            // Call the function that executes this tag command..
            //tag_exec_command(CMD_EXAMPLE);
            break;
        default:
            break;
    }
}

static void lfm_decode_command(uint8_t lf_command)
{
    // Check if command if the same as before and increment counter.
    if (lfm_data.ta_cmd == lf_command) {
        DEBUG_LOG(DBG_CAT_TAG_LF, "ta_cmd_counter = %d", (int)lfm_data.ta_cmd_counter);
        lfm_data.ta_cmd_counter++;
    } else {
        // If not, then initiate confirmation protocol.
        lfm_data.ta_cmd = lf_command;
        lfm_data.ta_cmd_counter = 0;
    }

    // Check if we received enough samples of the command to confirm it.
    if (lfm_data.ta_cmd_counter == TA_CONFIRM_CMD_N_TIMES) {
        // If so, clear state variables and go execute LF command.
        lfm_data.ta_cmd = 0;
        lfm_data.ta_cmd_counter = 0;
        lfm_clear_status_flag(LFM_WTA_DELAYED_CMD_EXEC_FLAG);
        lfm_dispatch_command(lf_command);
    }
}

static char* lfm_lf_events_to_string(lfm_lf_events_t event)
{
    switch (event) {
        case ENTERING_FIELD:
            return "Entering Field";

        case EXITING_FIELD:
            return "Exiting Field";

        case STAYING_FIELD:
            return "Staying in Field";

        case EXCITER_BATT_LOW:
            return "Exciter Battery Low";

        default:
            return "Undefined LF event";
    }
}

static void lfm_tick(void)
{
    tag_sw_timer_tick(&timer_exit_field);
}

static void lfm_fsm_start(void)
{
    lfm_running = true;
}

static void lfm_fsm_stop(void)
{
    lfm_running = false;
}

uint8_t lfm_get_exciter_type(uint8_t command)
{
    switch(command) {
        case LF_CMD_NOP:
            return EXCITER_TE_PTE;

        case LF_CMD_MT_BATT_LOW:
        case LF_CMD_MT:
            return EXCITER_MOTHER_TAG;

        default:
            return EXCITER_TE_PTE;
    }
}

static void lfm_build_lf_beacon(lfm_lf_events_t event)
{
    lf_beacon_data.lf_id_upper_bits = (lfm_data.buffer_1.id & 0x700) >> 8;
    lf_beacon_data.lf_id_lower_bits = (lfm_data.buffer_1.id & 0xFF);
    lf_beacon_data.lf_exciter_type = lfm_get_exciter_type(lfm_data.buffer_1.command);

    // Check for any Battery Low LF commands and override the triggered event.
    if (lfm_data.buffer_1.command == LF_CMD_MT_BATT_LOW) {
        lf_beacon_data.lf_message_type = EXCITER_BATT_LOW;
    } else {
        // Case no Battery Low LF command is present keep the triggered event.
        lf_beacon_data.lf_message_type = event;
    }
}

/**
 * @brief This will prepare LF beacon data and send LF Event signal to Tag Beacon Machine (TBM)
 * @param event
 */
static void lfm_report_lf_event(lfm_lf_events_t event)
{
    // Prepare LF beacon data.
    lfm_build_lf_beacon(event);

    if (event == STAYING_FIELD) {
        // Send LF Field Message in sync with Tag Beacon Message (slow/fast)
        tbm_set_event(TBM_LF_EVT, false);
    } else {
        // Send LF Field Message immediately
        tbm_set_event(TBM_LF_EVT, true);
    }

    DEBUG_LOG( DBG_CAT_TAG_LF,
               "Dispatch LF Field ID: %.3X Exciter Type: 0x%.2X LF Message Type: %s",
               lfm_data.buffer_1.id,
               lfm_data.buffer_1.command,
               lfm_lf_events_to_string(event));
}

/**
 * @brief Process the LF Finite State Machine
 */
static void lfm_process_step(void)
{
    switch (lfm_fsm.state) {

//------------------------------------------------------------------------------
        case INIT:
            lfm_fsm_start();
            lfm_data.buffer_0.command = 0;
            lfm_data.buffer_0.id = 0;
            lfm_fsm.state = CHECK_EXIT_TIMEOUT;

            break;

//------------------------------------------------------------------------------
        case CHECK_EXIT_TIMEOUT:

            if (tag_sw_timer_is_expired(&timer_exit_field)) {                   // Check if exit field timer is expiring in this tick

                lfm_report_lf_event(EXITING_FIELD);                             // If true, go Report LF event Exiting Field (async msg)
                lfm_clear_status_flag(LFM_ENTERING_FIELD_FLAG | LFM_STAYING_IN_FIELD_FLAG);  // Clear LF status flags
                lfm_data.buffer_1.command = 0;
                lfm_data.buffer_1.id = 0;
                lfm_fsm.state = EXIT;                                           // And exit.

            } else {

                if (lf_decoder_is_data_available()) {                           // Check if LF decoder has new LF data available
                    lf_decoder_get_lf_data(&lfm_data.buffer_0);                 // If true, grab new LF decoded data store buffer_0
                    lfm_fsm.state = PROCESS_TAG_IN_FIELD;                       // Go process Tag in Field
                } else {
                    lfm_fsm.state = EXIT;                                       // Or exit
                }
            }

            break;

//------------------------------------------------------------------------------
        case PROCESS_TAG_IN_FIELD:

            if (lfm_data.buffer_0.id == lfm_data.buffer_1.id) {                 // Check if we are in the same LF Field ID as before
                lfm_data.buffer_1 = lfm_data.buffer_0;                          // Buffer new data
                lfm_report_lf_event(STAYING_FIELD);                             // Report LF event Staying in Field to Tag Beacon Machine (sync msg)
                lfm_clear_status_flag(LFM_ENTERING_FIELD_FLAG);                 // Clear LF status flag "Entering Field"
                lfm_set_status_flag(LFM_STAYING_IN_FIELD_FLAG);                 // Set LF status flag "Staying in Field"
            } else {
                lfm_data.buffer_1 = lfm_data.buffer_0;                          // Buffer new data
                lfm_report_lf_event(ENTERING_FIELD);                            // Report LF event Entering Field (async msg)
                lfm_set_status_flag(LFM_ENTERING_FIELD_FLAG);
            }

            tag_sw_timer_reload(&timer_exit_field, LFM_TIMER_A_PERIOD_RELOAD);  // Reload Exiting Field timeout
            lfm_fsm.state = DECODE_COMMAND;                                     // And exit.

            break;

//------------------------------------------------------------------------------
        case DECODE_COMMAND:
            switch(lfm_data.buffer_1.command) {
                case LF_CMD_NOP:
                case LF_CMD_MT:
                case LF_CMD_MT_BATT_LOW:
                    // we dont need to do anything for plain LF field..
                    lfm_clear_status_flag(LFM_WTA_DELAYED_CMD_EXEC_FLAG);
                    lfm_clear_ta_cmd();
                    break;

                case LF_CMD_EXAMPLE:
                    lfm_set_status_flag(LFM_WTA_DELAYED_CMD_EXEC_FLAG);
                    lfm_decode_command(LF_CMD_EXAMPLE);
                    break;

                default:
                    break;
            }

            lfm_fsm.state = EXIT;                                                 // And exit.
            break;

//------------------------------------------------------------------------------
        case EXIT:
            lfm_fsm_stop();
            break;

//------------------------------------------------------------------------------
        default:
            DEBUG_LOG(DBG_CAT_TAG_LF,"Error! Unknown state lfm_fsm = %u", (uint8_t)lfm_fsm.state);
            lfm_fsm_stop();
            break;
    }
}


//******************************************************************************
// Non Static functions
//******************************************************************************
lfm_lf_beacon_t* lfm_get_beacon_data(void)
{
    return &lf_beacon_data;
}

uint8_t lfm_get_lf_status(void)
{
    return lfm_data.status;
}

/**
 * @brief LF Machine (Responsible for LF high level functionalities)
 * @details Reports LF Field events to Tag Beacon Machine and decodes commands from Tag Activator.
 */
void lf_run(void)
{
    // Update all internal sw timers
    lfm_tick();

    lfm_fsm.state = INIT;

    // Run lf machine process until completion
    do {
        lfm_process_step();
    } while (lfm_running);
}

//! @brief LF Machine Init
uint32_t lfm_init(void)
{
    tag_sw_timer_reload(&timer_exit_field, 0);

    lfm_data.ta_cmd = 0;
    lfm_data.ta_cmd_counter = 0;

    lfm_fsm.state = INIT;
    lfm_fsm_stop();

    return 0;
}
