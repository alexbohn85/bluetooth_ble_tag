/*
 * cli.c
 *
 * This module provide UART a command line interface functionality.
 */


//TODO This "custom" CLI implementation is only supposed to be temporary, this implementation should be replaced with Gecko CLI Service.

#include "cli.h"
#include "dbg_utils.h"

#include "stdio.h"
#include "string.h"
#include "sl_iostream.h"
#include "sl_spidrv_instances.h"
#include "sl_iostream_init_usart_instances.h"
#include "sl_sleeptimer.h"


//******************************************************************************
// Defines
//******************************************************************************
#define CLI_SLEEPTIMER_TICK_MS       (50)
#define CLI_BUFFER_SIZE              (165)
#define ENTER_KEY                    ('\r')

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Global variables
//******************************************************************************
static sl_sleeptimer_timer_handle_t cli_timer_inst;
static sl_sleeptimer_timer_handle_t *cli_timer_handle;
static char input[CLI_BUFFER_SIZE];
static char cmd[CLI_BUFFER_SIZE];
static volatile uint8_t i = 0;
static volatile uint8_t j;

//******************************************************************************
// Static functions
//******************************************************************************
static void cli_clear_input_buffer(void)
{
    //! clear input buffer
    memset(input,'\0',sizeof(input));

    //! reset input buffer position
    i = 0;
}

static void cli_process_command(void)
{
    //TODO this step does not appear to be necessary anymore, we can use input buffer directly
    memset(cmd,'\0',sizeof(cmd));
    strcpy(cmd, input);

    cli_clear_input_buffer();

    /*! decode commands !*/

    // log on/off --------------------------------------------------------------
    if (strcmp(cmd, "log on") == 0) {
        dbg_log_enable(true);
        printf(COLOR_B_WHITE"\n\n-> Log enabled...\n");
        printf(COLOR_RST);

    } else if (strcmp(cmd, "log off") == 0) {
        dbg_log_enable(false);
        printf(COLOR_B_WHITE"\n\n-> Log disabled...\n");

        // log filter on/off ---------------------------------------------------
    } else if (strcmp(cmd, "log filter on") == 0) {
        dbg_log_filter_disable(false);
        printf(COLOR_B_WHITE"\n\n-> Log filter enabled...\n");

    } else if (strcmp(cmd, "log filter off") == 0) {
        dbg_log_filter_disable(true);
        printf(COLOR_B_WHITE"\n\n-> Log filter disabled...\n");

        // log filter add ------------------------------------------------------
    } else if (strcmp(cmd, "log filter add DBG_CAT_TAG_MAIN") == 0) {
        dbg_log_filter_disable(false);
        dbg_log_add_filter_rule(DBG_CAT_TAG_MAIN);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_TAG_MAIN added...\n");

    } else if (strcmp(cmd, "log filter add DBG_CAT_BLE") == 0) {
        dbg_log_filter_disable(false);
        dbg_log_add_filter_rule(DBG_CAT_BLE);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_BLE added...\n");

    } else if (strcmp(cmd, "log filter add DBG_CAT_TEMP") == 0) {
        dbg_log_filter_disable(false);
        dbg_log_add_filter_rule(DBG_CAT_TEMP);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_TEMP added...\n");

    } else if (strcmp(cmd, "log filter add DBG_CAT_TAG_LF") == 0) {
        dbg_log_filter_disable(false);
        dbg_log_add_filter_rule(DBG_CAT_TAG_LF);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_TAG_LF added...\n");

        // log filter remove ---------------------------------------------------
    } else if (strcmp(cmd, "log filter add DBG_CAT_TAG_MAIN") == 0) {
        dbg_log_remove_filter_rule(DBG_CAT_TAG_MAIN);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_TAG_MAIN removed...\n");

    } else if (strcmp(cmd, "log filter remove DBG_CAT_BLE") == 0) {
        dbg_log_remove_filter_rule(DBG_CAT_BLE);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_BLE removed...\n");

    } else if (strcmp(cmd, "log filter remove DBG_CAT_TEMP") == 0) {
        dbg_log_remove_filter_rule(DBG_CAT_TEMP);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_TEMP removed...\n");

    } else if (strcmp(cmd, "log filter remove DBG_CAT_TAG_LF") == 0) {
        dbg_log_remove_filter_rule(DBG_CAT_TAG_LF);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_TAG_LF removed...\n");

        // log filter list -----------------------------------------------------
    } else if (strcmp(cmd, "log filter list") == 0) {
        printf(COLOR_B_WHITE"\n\n-> Available log filters:\n"
                                "   DBG_CAT_TAG_MAIN\n"
                                "   DBG_CAT_BLE\n"
                                "   DBG_CAT_TEMP\n"
                                "   DBG_CAT_TAG_LF\n");

        // stop cli ------------------------------------------------------------
    } else if (strcmp(cmd, "cli stop") == 0) {
        cli_stop();

        // git info ------------------------------------------------------------
    } else if (strcmp(cmd, "git info") == 0) {
        dbg_print_git_info();

        // reset ---------------------------------------------------------------
    } else if (strcmp(cmd, "reset") == 0) {
        printf(COLOR_B_WHITE"\n\n-> Rebooting...\n");
        NVIC_SystemReset();

        // banner --------------------------------------------------------------
    } else if (strcmp(cmd, "banner") == 0) {
        dbg_print_banner();

    } else if ((strcmp(cmd, "help") == 0) || (strcmp(cmd, "h") == 0)) {
        printf("\n\nusage: command [OPTION]...\n\n"                                                               \
  COLOR_B_WHITE"   Commands                Options                  Description\n"COLOR_RST                       \
               "   log                     [on, off]                -> Turn log prints on/off\n"                  \
               "   -----------------------------------------------------------------------------------------\n"   \
               "   log filter              [on, off]                -> Log filter on/off\n"                       \
               "   log filter add          [dbg_log_filters_t]      -> Add log filter to list\n"                  \
               "   log filter remove       [dbg_log_filters_t]      -> Remove log filter from list\n"             \
               "   log filter list                                  -> List available filters\n"                  \
               "   -----------------------------------------------------------------------------------------\n"   \
               "   cli stop                                         -> Stop cli process\n"                        \
               "   git info                                         -> Show git info\n"                           \
               "   reset                                            -> System reset\n"                            \
               "   banner                                           -> Show banner\n");

    } else {
        if(strcmp(cmd,"") != 0) {
            printf("\n... type \"help\" or \"h\" for commands usage\n> ");
        }
    } //!--- END OF INPUT DECODE PROCESS

    printf("\n> ");

}

static void cli_refresh_console(void)
{
    //! Refresh user console
    printf(ERASE_LINE "\r> ");
    for(j = 0; j < i; ++j) {
        printf("%c", input[j]);
    }

    //! Check for input buffer limits
    if (i > (CLI_BUFFER_SIZE - 1)) {
        //! Send backspace
        printf("\b");
    }
}

static void cli_process_input(size_t num_bytes, char buf)
{
    //! Ignore non printable keys (too keep this terminal simple we dont care about arrows, home, end keys)
    if (buf == 0x1b) {
        do {
            sl_iostream_read(sl_iostream_uart_debug_handle, &buf, sizeof(buf), &num_bytes);
        } while(num_bytes > 0);
        return;
    }

    //! Check if it is a <CTRL+C> command (kill active process)
    if (buf == '\003') {
        cli_clear_input_buffer();
        dbg_log_enable(false);
        printf("\n");

    } else {

        //! Check if this is a <BACKSPACE>
        if (buf == 0x7f) {
            if (i > 0) {
                input[--i] = '\0';
            }
        } else {
            if (i > 164) {
                printf("\b");
            } else {
                input[i] = buf;
                ++i;
            }
        }
    }

    cli_refresh_console();
}

//! @brief CLI Process
//! @details CLI Sleeptimer callback to process remote terminal inputs
static void cli_run_fsm(sl_sleeptimer_timer_handle_t *handle, void *data)
{
    (void)(data);
    (void)(handle);

    char buf;
    size_t num_bytes;

    sl_iostream_read(sl_iostream_uart_debug_handle, &buf, sizeof(buf), &num_bytes);

    if (num_bytes > 0) {
        if (buf != ENTER_KEY) {
            //! keep buffering (escape codes, validation, buffer limits)
            cli_process_input(num_bytes, buf);
        } else {
            //! go decode command from input buffer
            cli_process_command();
        }
    }
}


//******************************************************************************
// Non Static functions
//******************************************************************************
void cli_start(void)
{
#if defined(TAG_CLI_PRESENT)

    sl_status_t status;

    cli_timer_handle = &cli_timer_inst;

    sl_iostream_uart_set_rx_energy_mode_restriction(sl_iostream_uart_uart_debug_handle, true);

    status = sl_sleeptimer_start_periodic_timer_ms( cli_timer_handle,
                                                    CLI_SLEEPTIMER_TICK_MS,
                                                    cli_run_fsm,
                                                    NULL,
                                                    5,
                                                    SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
    if (status == 0) {
        DEBUG_LOG(DBG_CAT_SYSTEM, "Starting CLI process...");
        printf("\n> ");
    }

#endif
}

void cli_stop(void)
{
#if defined(TAG_CLI_PRESENT)

    bool is_running;

    sl_sleeptimer_is_timer_running(cli_timer_handle, &is_running);

    if (is_running) {

        sl_iostream_uart_set_rx_energy_mode_restriction(sl_iostream_uart_uart_debug_handle, false);

        sl_status_t status;
        status = sl_sleeptimer_stop_timer(cli_timer_handle);

        if (status == 0) {
            DEBUG_LOG(DBG_CAT_SYSTEM, "Stopping CLI process...");
        } else {
            DEBUG_TRAP();
        }
    }
#endif
}

void cli_init(void)
{
#if defined(TAG_CLI_PRESENT)
    sl_status_t status;
    cli_timer_handle = &cli_timer_inst;
    status = sl_sleeptimer_start_periodic_timer_ms( cli_timer_handle,
                                                    CLI_SLEEPTIMER_TICK_MS,
                                                    cli_run_fsm,
                                                    NULL,
                                                    5,
                                                    SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
    if (status == 0) {
        DEBUG_LOG(DBG_CAT_SYSTEM, "Starting CLI process...");
        printf("\n> ");
    }
#endif
}

void cli_deinit(void)
{
#if defined(TAG_CLI_PRESENT)
    sl_status_t status;
    status = sl_sleeptimer_stop_timer(cli_timer_handle);
    if (status == 0) {
        DEBUG_LOG(DBG_CAT_SYSTEM, "Stopping CLI process...");
    }
#endif
}
