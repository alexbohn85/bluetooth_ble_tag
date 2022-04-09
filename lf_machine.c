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

#define LFM_WTA_DELAYED_CMD_EXEC_COUNTS        (3)                              //! # of WTA commands events in a row before executing command.

#define LF_CMD_NOP                             (0x00)

#define LFM_TIMER_A_PERIOD_MS                  3000
#define LFM_TIMER_A_PERIOD_RELOAD              (LFM_TIMER_A_PERIOD_MS / TMM_DEFAULT_TIMER_PERIOD_MS)

#define LFM_TIMER_B_PERIOD_MS                  12000
#define LFM_TIMER_B_PERIOD_RELOAD              (LFM_TIMER_B_PERIOD_MS / TMM_DEFAULT_TIMER_PERIOD_MS)

#define LFM_TIMER_C_PERIOD_MS                  10000
#define LFM_TIMER_C_PERIOD_RELOAD              (LFM_TIMER_C_PERIOD_MS / TMM_DEFAULT_TIMER_PERIOD_MS)

//******************************************************************************
// Data types
//******************************************************************************
typedef enum lfm_states_t {
    INIT,                      //!< INIT
    CHECK_EXIT_TIMEOUT,        //!< CHECK_EXIT_TIMEOUT
    CHECK_WTA_DELAYED_CMD_EXEC,//!< CHECK_WTA_DELAYED_CMD_EXEC
    CHECK_NEW_LF_DATA,         //!< CHECK_NEW_LF_DATA
    CHECK_WTA_BACKOFF,         //!< CHECK_WTA_BACKOFF
    START_WTA_DELAYED_CMD_EXEC,//!< START_WTA_DELAYED_CMD_EXEC
    PROCESS_TAG_IN_FIELD,      //!< PROCESS_TAG_IN_FIELD
    EXIT                       //!< EXIT
} lfm_states_t;

typedef struct lfm_data_t {
    uint8_t status;
    uint8_t wta_cmd_counter;
    lf_decoder_data_t lf_data_0;  //! we use this to save new lf data
    lf_decoder_data_t lf_data_1;  //! we use this to store previous lf data
} lfm_data_t;

typedef struct lfm_fsm_t {
    lfm_states_t state;
    lfm_states_t return_state;
} lfm_fsm_t;


//******************************************************************************
// Global variables
//******************************************************************************
//! Internal sw timers
static tag_sw_timer_t timer_exit_field;
static tag_sw_timer_t timer_staying_field;
static tag_sw_timer_t timer_wta_backoff;
//! Internal data
static lfm_data_t lfm_data;
//! FSM state variable
static volatile lfm_fsm_t lfm_fsm;
//! FSM running flag
static volatile bool lfm_running;
//! LF Beacon data
static lfm_lf_beacon_t lf_beacon_data;

//******************************************************************************
// Static functions
//******************************************************************************
static char* lfm_lf_events_to_string(lfm_lf_events_t event)
{
    switch (event) {
        case ENTERING_FIELD:
            return "Entering Field";
            break;
        case EXITING_FIELD:
            return "Exiting Field";
            break;
        case STAYING_FIELD:
            return "Staying in Field";
            break;
        case WTA_DELAYED_CMD_EXEC:
            return "WTA Delayed Command Execution";
            break;
        default:
            return "Undefined LF event";
            break;
    }
}

static void lfm_tick(void)
{
    tag_sw_timer_tick(&timer_exit_field);
    tag_sw_timer_tick(&timer_staying_field);
    tag_sw_timer_tick(&timer_wta_backoff);
}

//! @brief Decode and Execute WTA Command
static void lfm_decode_wta_command(void)
{
    lfm_data.wta_cmd_counter = 0;                                               //! Clear WTA delayed cmd exec. counter.
    lfm_data.status &= ~(LFM_WTA_DELAYED_CMD_EXEC_FLAG);                        //! Clear WTA delayed cmd exec. flag.
    lfm_data.status |= LFM_WTA_BACKOFF_FLAG;                                    //! Set WTA backoff flag (this will prevent executing same command over and over
                                                                                //! as WTA keeps sending it for 20 seconds. We could potentially disable LF decoder.

    tag_sw_timer_reload(&timer_wta_backoff, LFM_TIMER_C_PERIOD_MS);             //! Load WTA Backoff timer.
    tag_sw_timer_reload(&timer_exit_field, 0);                                  //! Stop Exit Field timer.

    DEBUG_LOG(DBG_CAT_TAG_LF,"WTA Delayed Command Exec. Confirmed! Dispatching Command = 0x%.2X", lfm_data.lf_data_1.command);
}

//! @brief Perform WTA delayed command execution
static void lfm_process_delayed_cmd(void)
{
    if (lfm_data.lf_data_0.command == lfm_data.lf_data_1.command) {
        lfm_data.wta_cmd_counter++;
        if (lfm_data.wta_cmd_counter == LFM_WTA_DELAYED_CMD_EXEC_COUNTS) {
            lfm_decode_wta_command();
            lfm_fsm.state = EXIT;                                               //! And exit.
        } else {
            tag_sw_timer_reload(&timer_exit_field, LFM_TIMER_A_PERIOD_RELOAD);  //! Reload Exit Field timer.
            lfm_fsm.state = EXIT;                                               //! And exit.
        }
    } else {
        lfm_fsm.state = CHECK_NEW_LF_DATA;
        lfm_data.status &= ~(LFM_WTA_DELAYED_CMD_EXEC_FLAG);
    }
}

//! @brief Start LF Finite State Machine
static void lfm_fsm_start(void)
{
    lfm_running = true;
}

//! @brief Stop LF Finite State Machine
static void lfm_fsm_stop(void)
{
    lfm_running = false;
}

//! @brief This will prepare LF beacon data and send LF Event signal to Tag Beacon Machine (TBM)
static void lfm_report_lf_event(lfm_lf_events_t event)
{
    //! Prepare LF beacon data.
    lf_beacon_data.lf_data = lfm_data.lf_data_1;
    lf_beacon_data.lf_event = event;

    if (event == STAYING_FIELD) {
        tbm_set_event(TBM_LF_EVT, false);
    } else {
        tbm_set_event(TBM_LF_EVT, true);
    }

    //! Signal LF Event to Tag Beacon Machine.
    DEBUG_LOG( DBG_CAT_TAG_LF,
               "Dispatch LF Event to Tag Beacon Machine, LF ID: 0x%.4X LF CMD: 0x%.2X LF EVENT: %s",
               lfm_data.lf_data_1.id,
               lfm_data.lf_data_1.command,
               lfm_lf_events_to_string(event));
}

/**
 * @brief Process the LF Finite State Machine
 */
static void lfm_process_step(void)
{
    switch (lfm_fsm.state) {

        //!=====================================================================
        case INIT:
            lfm_fsm_start();
            lfm_data.lf_data_0.command = 0;
            lfm_data.lf_data_0.id = 0;
            lfm_fsm.state = CHECK_EXIT_TIMEOUT;
            break;

        //!=====================================================================
        case CHECK_EXIT_TIMEOUT:
            //! Check for exiting field timeout
            if (tag_sw_timer_is_expired(&timer_exit_field)) {
                lfm_report_lf_event(EXITING_FIELD);                             //! Go process Exiting Field event
                lfm_data.status &= ~(LFM_STAYING_IN_FIELD_FLAG);                //! Clear Staying In Field flag
                lfm_data.lf_data_1.command = 0;
                lfm_data.lf_data_1.id = 0;
                lfm_fsm.state = EXIT;                                           //! And exit.
            } else {
                if (lf_decoder_is_data_available()) {                           //! Check if LF decoder has decoded data available
                    lf_decoder_get_lf_data(&lfm_data.lf_data_0);                //! If true, go grab new LF decoded data
                    lf_decoder_clear_lf_data();
                    lfm_fsm.state = CHECK_WTA_DELAYED_CMD_EXEC;                 //! And go check if there is a WTA delayed command exec. process
                } else {
                    lfm_fsm.state = EXIT;
                }
            }
            break;

        //!=====================================================================
        case CHECK_WTA_DELAYED_CMD_EXEC:
            if (lfm_data.status & LFM_WTA_DELAYED_CMD_EXEC_FLAG) {              //! Check if we are currently in WTA delayed command execution
                lfm_process_delayed_cmd();                                      //! If true, go process the delayed command execution.
            } else {
                lfm_fsm.state = CHECK_NEW_LF_DATA;                              //! If false, go process the new LF data.
            }
            break;

        //!=====================================================================
        case CHECK_NEW_LF_DATA:
            if (lfm_data.lf_data_0.command == LF_CMD_NOP) {                     //! Check if this is just a Tag Exciter field (CMD = NOP(0x00))
                lfm_fsm.state = PROCESS_TAG_IN_FIELD;                           //! If true, this is just a regular Tag Exciter field, go process that.
            } else {
                lfm_fsm.state = CHECK_WTA_BACKOFF;                              //! If false, this is a WTA field. Go check if WTA is in backoff timer.
            }
            break;

        //!=====================================================================
        case CHECK_WTA_BACKOFF:
            if (lfm_data.status & LFM_WTA_BACKOFF_FLAG) {                       //! Check if we are waiting for WTA backoff timer to expire.
                if (tag_sw_timer_is_expired(&timer_wta_backoff)) {              //! If true, check if timer expired.
                    lfm_data.status &= ~(LFM_WTA_BACKOFF_FLAG);                 //! If so, clear WTA backoff flag
                    lfm_fsm.state = START_WTA_DELAYED_CMD_EXEC;                 //! And go start WTA delayed command exec.
                } else {
                    DEBUG_LOG(DBG_CAT_TAG_LF, "WTA Back-off timer is still running..");   //! If timer is still running just ignore this command.
                    lfm_fsm.state = EXIT;                                                 //! And exit.
                }
            } else {
                lfm_data.status |= LFM_WTA_BACKOFF_FLAG;
                tag_sw_timer_reload(&timer_wta_backoff, LFM_TIMER_C_PERIOD_MS);
            }
            break;

        //!=====================================================================
        case START_WTA_DELAYED_CMD_EXEC:

             /*!----------------------------------------------------------------
                We are being requested to execute a LF command from WTA.
                But since this could have been a corrupted transmission due to a
                false CRC OK, we need to confirm it before proceeding to execute
                it. We will need to receive the same LF ID and LF CMD for "n"
                times in a row to confirm it.
              * --------------------------------------------------------------*/
            lfm_data.status |= LFM_WTA_DELAYED_CMD_EXEC_FLAG;                   //! Set WTA delayed command execution flag.
            lfm_data.wta_cmd_counter = 0;                                       //! Reset WTA delayed command execution counter
            lfm_data.lf_data_1 = lfm_data.lf_data_0;                            //! Buffer the new LF data.
            lfm_report_lf_event(WTA_DELAYED_CMD_EXEC);                          //! Report LF event Delayed LF Command.
            tag_sw_timer_reload(&timer_exit_field, LFM_TIMER_A_PERIOD_RELOAD);    //! Load Exit Field Timeout timer.
            lfm_fsm.state = EXIT;                                               //! And exit.

            break;

        //!=====================================================================
        case PROCESS_TAG_IN_FIELD:
            if (lfm_data.lf_data_0.id == lfm_data.lf_data_1.id) {                          //! Check if new LF data is from same LF ID
               lfm_data.status |= LFM_STAYING_IN_FIELD_FLAG;                               //! Set Staying in Field flag
                if (tag_sw_timer_is_stopped(&timer_staying_field)) {                       //! Check if Staying in Field timer is stopped.
                    tag_sw_timer_reload(&timer_staying_field, LFM_TIMER_B_PERIOD_RELOAD);    //! Reload Exit Field timer.
                }
            } else {
                lfm_data.lf_data_1 = lfm_data.lf_data_0;                                   //! If different, buffer the new LF data.
                tag_sw_timer_reload(&timer_staying_field, LFM_TIMER_B_PERIOD_RELOAD);        //! Reload Staying in Field timer.
                lfm_report_lf_event(ENTERING_FIELD);                                       //! Report LF event Entering Field.
            }

            tag_sw_timer_reload(&timer_exit_field, LFM_TIMER_A_PERIOD_RELOAD);               //! If true, load it.
            lfm_fsm.state = EXIT;                                                          //! And exit.

            break;

        //!=====================================================================
        case EXIT:
            lfm_fsm_stop();
            break;

        //!=====================================================================
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

//! @brief LF Machine
//! @details Reports LF Field events to Tag Beacon Machine
void lfm_run(void)
{
    //! Update internal sw timers
    lfm_tick();
    //return;

    //! reset lfm process state machine
    lfm_fsm.state = INIT;
    lfm_running = false;

    //! Run this machine until its completion.
    do {
        lfm_process_step();
    } while (lfm_running);

    if (tag_sw_timer_is_expired(&timer_staying_field)) {
        if (lfm_data.status & LFM_STAYING_IN_FIELD_FLAG) {
            lfm_report_lf_event(STAYING_FIELD);
        }
    }
}

//! @brief LF Machine Init
uint32_t lfm_init(void)
{
    tag_sw_timer_reload(&timer_exit_field, 0);
    tag_sw_timer_reload(&timer_staying_field, 0);
    tag_sw_timer_reload(&timer_wta_backoff, 0);
    lfm_fsm.state = INIT;
    return 0;
}
