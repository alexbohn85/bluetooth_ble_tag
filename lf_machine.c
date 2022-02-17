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
#include "stdio.h"
#include "stdbool.h"
#include "dbg_utils.h"
#include "sl_spidrv_instances.h"
#include "as393x.h"
#include "lf_decoder.h"
#include "lf_machine.h"
#include "tag_main_machine.h"


//******************************************************************************
// Defines
//******************************************************************************
#define LFM_TIMER_A_PERIOD_MS        2000
//TODO Check this division
#define LFM_TIMER_A_PERIOD_TICK      (LFM_TIMER_A_PERIOD_MS / TMM_DEFAULT_TICK_PERIOD_MS)

#define LFM_TIMER_B_PERIOD_MS        12000
//TODO Check this division
#define LFM_TIMER_B_PERIOD_TICK      (LFM_TIMER_A_PERIOD_MS / TMM_DEFAULT_TICK_PERIOD_MS)

//******************************************************************************
// Data types
//******************************************************************************
static lf_decoder_data_t buffer[2];

//******************************************************************************
// Global variables
//******************************************************************************
static volatile tag_sw_timer_t lfm_timer_A;
static volatile tag_sw_timer_t lfm_timer_B;

//******************************************************************************
// Static functions
//******************************************************************************
//! SW Timer A - Used for Exiting Field Message Report
static void lfm_reload_timer_A(uint32_t value)
{
    lfm_timer_A.value = value;
}

//! SW Timer B - Used for Staying in Field Message Report
static void lfm_reload_timer_B(uint32_t value)
{
    lfm_timer_B.value = value;
}

static void lfm_tick_timeout_timer_A(void)
{
    if (lfm_timer_A.value > 0) {
        --lfm_timer_A.value;
    }
}

static void lfm_tick_timeout_timer_B(void)
{
    if (lfm_timer_B.value > 0) {
        --lfm_timer_B.value;
    }
}

//******************************************************************************
// Non Static functions
//******************************************************************************
void lf_run(void)
{
    // Update SW Timers
    lfm_tick_timeout_timer_A();
    lfm_tick_timeout_timer_B();

    if (lf_decoder_is_data_available()) {
        lf_decoder_get_lf_data(&buffer[0]);
        lf_decoder_clear_lf_data();

        if (lfm_timer_A.value == 0) {
            DEBUG_LOG(DBG_CAT_TAG_LF, "LF Field ID: 0x%04X CMD: 0x%02X", buffer[0].id, buffer[0].command);
            // 1 second
            lfm_reload_timer_A(2000/TMM_DEFAULT_TICK_PERIOD_MS);
        }
    }
}

uint32_t lfm_init(void)
{
    lfm_reload_timer_A(0);
    lfm_reload_timer_B(0);
    return 0;
}






// old code
#if 0
        if (validating_cmd) {
            if ((buffer[0].command == buffer[1].command) && (buffer[0].id == buffer[1].id)) {
                cmd_counter++;                    // increment counter
                if (cmd_counter == 3) {           // check counter value
                    validating_cmd = false;       // reset value
                    cmd_counter = 0;              // reset value
                    lfm_lf_command_exec();        // go execute command
                }
            } else {
                cmd_counter = 0;
                validating_cmd = false;
            }

        } else {
            if (buffer[0].command != 0x00) {      // We want to receive this command "n" times to make sure this
                validating_cmd = true;            // is a real command and not some garbage value from a random CRC false pass.
                cmd_counter = 0;                  // This is a simpler solution than implementing a complex handshake.
            }
        }


        if(buffer[0].id == buffer[1].id) {
            // Set LF_STATE = STAYING IN FIELD

        } else {
            DEBUG_LOG(DBG_CAT_TAG_LF, "Enter LF Field ID: 0x%04X CMD: 0x%02X", buffer[0].id, buffer[0].command);
            // Set LF_STATE = ENTERING FIELD
            buffer[1] = buffer[0];
        }

        if (lfm_timer_A.value == 1) {
            DEBUG_LOG(DBG_CAT_TAG_LF, "Exiting LF Field ID: 0x%04X CMD: 0x%02X", buffer[1].id, buffer[1].command);
            // Set LF_STATE = EXITING FIELD
        }

        lfm_reload_timer_A(TMM_DEFAULT_TICK_PERIOD_MS * 8); // 2 seconds timeout
    }

#endif
