/*
 * cli.c
 *
 * This module provide UART a command line interface functionality.
 */


#include "stdio.h"
#include "string.h"
#include "sl_iostream.h"
#include "sl_spidrv_instances.h"
#include "sl_iostream_init_usart_instances.h"
#include "sl_sleeptimer.h"

#include "dbg_utils.h"
#include "lf_decoder.h"
#include "tag_main_machine.h"
#include "tag_beacon_machine.h"
#include "tag_power_manager.h"
#include "nvm.h"
#include "boot.h"
#include "cli.h"


//******************************************************************************
// Defines
//******************************************************************************
#define CLI_SLEEPTIMER_TICK_MS       (50)
#define CLI_BUFFER_SIZE              (165)
#define ENTER_KEY                    ('\r')
#define BACKSPACE                    (0x7f)
#define CTRL_C                       ('\003')
#define NON_PRINTABLE_CHAR           (0x1b)

// For SERMAC (manufacturing mode)
#define DATA_ACK                     (0x90)
#define DATA_NAK                     (0xA0)
#define SEND_RSP(x)                  sl_iostream_putchar(sl_iostream_uart_debug_handle, x);

//******************************************************************************
// Data types
//******************************************************************************
typedef struct cli_buffer_t {
    char data[CLI_BUFFER_SIZE];
    uint8_t pos;
} cli_buffer_t;

//******************************************************************************
// Global variables
//******************************************************************************
static sl_sleeptimer_timer_handle_t cli_timer_inst;
static sl_sleeptimer_timer_handle_t *cli_timer_handle;
static cli_buffer_t input;
static cli_buffer_t cmd;

//static char input[CLI_BUFFER_SIZE];
//static char cmd[CLI_BUFFER_SIZE];

//******************************************************************************
// Static functions
//******************************************************************************
static void cli_print_mac(uint8_t* mac)
{
    uint8_t i;

    printf("\n");
    for (i = 5; i > 0; --i) {
        printf("%.2X:", (uint8_t)mac[i]);
    }
    printf("%.2X", mac[i]);
}

static void cli_clear_input_buffer(void)
{
    // clear input buffer
    memset(input.data,'\0',sizeof(input.data));

    // reset input buffer position
    input.pos = 0;
}

static uint32_t cli_decode_mac_addr(void)
{
    volatile uint8_t i;
    uint32_t status;
    unsigned int m[6];
    uint8_t mac[6];
    char *ptr;
    int ret;

    // Grab pointer to the first ':' occurrence.
    ptr = strstr(cmd.data, ":");

    if (ptr != NULL) {
        ret = sscanf((ptr - 2), "%2X:%2X:%2X:%2X:%2X:%2X%*c", &m[5], &m[4], &m[3], &m[2], &m[1], &m[0]);
        if (ret == 6) {
            for (i = 0; i < sizeof(mac); i++) {
                mac[i] = (uint8_t)m[i];
            }
            status = nvm_write_mac_address(mac);
        } else {
            status = 1;
        }
    } else {
        status = 1;
    }

    return status;
}

static void cli_process_manufacturing_command(void)
{
    memset(cmd.data,'\0',sizeof(cmd.data));
    strcpy(cmd.data, input.data);

    cli_clear_input_buffer();

    //--------------------------------------------------------------------------
    // Decode Input Commands
    //--------------------------------------------------------------------------

    // enter current draw mode -------------------------------------------------
    if (strstr(cmd.data, "current draw mode") != NULL) {

        int ret;
        uint32_t reset_delay_timeout;

        ret = sscanf(cmd.data, "%*s %*s %*s %lu", &reset_delay_timeout);

        if (ret == 1) {
            tpm_enter_current_draw_mode(reset_delay_timeout);
            SEND_RSP(DATA_ACK);
            DEBUG_LOG(DBG_CAT_CLI, "Entering current draw mode, argument = %lu msec", reset_delay_timeout);
        } else {
            SEND_RSP(DATA_NAK);
            DEBUG_LOG(DBG_CAT_WARNING, "Syntax error...");
        }

    // write/read custom mac address to NVM ------------------------------------
    } else if (strstr(cmd.data, "write mac") != NULL) {

        if(cli_decode_mac_addr() == 0) {
            SEND_RSP(DATA_ACK);
            DEBUG_LOG(DBG_CAT_CLI, "New MAC was saved into NVM... rebooting to apply new settings");
            tpm_schedule_system_reset(1000);
        } else {
            SEND_RSP(DATA_NAK);
            DEBUG_LOG(DBG_CAT_WARNING, "Syntax error... expected MAC address format -> XX:XX:XX:XX:XX:XX");
        }

    } else if (strstr(cmd.data, "read mac") != NULL) {

        uint8_t mac[6];
        nvm_read_mac_address(mac);
        cli_print_mac(mac);

    // log filter on/off -------------------------------------------------------
    } else if (strcmp(cmd.data, "log on") == 0) {

        dbg_log_enable(true);
        SEND_RSP(DATA_ACK);
        DEBUG_LOG(DBG_CAT_CLI, "Log is on");

    } else if (strcmp(cmd.data, "log off") == 0) {

        dbg_log_enable(false);
        SEND_RSP(DATA_NAK);
        DEBUG_LOG(DBG_CAT_CLI, "Log is off");

    // git info ----------------------------------------------------------------
    } else if (strcmp(cmd.data, "git info") == 0) {

        dbg_print_git_info();

    // reset -------------------------------------------------------------------
    } else if (strcmp(cmd.data, "reset") == 0) {

        NVIC_SystemReset();

    // help --------------------------------------------------------------------
    } else if (strcmp(cmd.data, "?") == 0) {

        printf("\n\n usage: command [OPTION]\n\n"                                                                         \
  COLOR_B_WHITE"   Commands                Options                  Description\n" COLOR_RST                              \
               "|   -----------------------------------------------------------------------------------------\n"          \
               "|   current draw mode       <value>                  -> Enter current draw mode\n"                        \
               "|                                                             <value> = 0 Tag stays in this mode until a manual reset\n"                                    \
               "|                                                             <value> = 1-99999 Tag will automatically reset into 'NORMAL MODE' after timeout (in ms)\n"    \
               "|   -----------------------------------------------------------------------------------------\n"          \
               "|   write mac <address>                              -> Write MAC address into NVM.\n"                    \
               "|                                                             <address> format -> XX:XX:XX:XX:XX:XX\n"    \
               "|   read mac                                         -> Read MAC address\n"                               \
               "|   -----------------------------------------------------------------------------------------\n"          \
               "|   log                     on, off                  -> Log on or off\n"                                  \
               "|   git info                                         -> Show build git commit\n"                          \
               "|   -----------------------------------------------------------------------------------------\n"          \
               "|   reset                                            -> System reset\n"                                   \
               );

    } else {
        SEND_RSP(DATA_ACK);
    } //--- END OF INPUT DECODE PROCESS

}

static void cli_process_command(void)
{
    //TODO this step does not appear to be necessary anymore, we can use input buffer directly
    memset(cmd.data,'\0',sizeof(cmd.data));
    strcpy(cmd.data, input.data);

    cli_clear_input_buffer();

    //--------------------------------------------------------------------------
    // Decode Input Commands
    //--------------------------------------------------------------------------

    // log on/off --------------------------------------------------------------
    if (strcmp(cmd.data, "log on") == 0) {
        dbg_log_enable(true);
        printf(COLOR_B_WHITE"\n\n-> Log enabled...\n");
        printf(COLOR_RST);

    } else if (strcmp(cmd.data, "log off") == 0) {
        dbg_log_enable(false);
        printf(COLOR_B_WHITE"\n\n-> Log disabled...\n");

    // log filter on/off -------------------------------------------------------
    } else if (strcmp(cmd.data, "log filter on") == 0) {
        dbg_log_filter_disable(false);
        printf(COLOR_B_WHITE"\n\n-> Log filter enabled...\n");

    } else if (strcmp(cmd.data, "log filter off") == 0) {
        dbg_log_filter_disable(true);
        printf(COLOR_B_WHITE"\n\n-> Log filter disabled...\n");

    // log filter add ----------------------------------------------------------
    } else if (strcmp(cmd.data, "log filter add DBG_CAT_TAG_MAIN") == 0) {
        dbg_log_filter_disable(false);
        dbg_log_add_filter_rule(DBG_CAT_TAG_MAIN);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_TAG_MAIN added...\n");

    } else if (strcmp(cmd.data, "log filter add DBG_CAT_BLE") == 0) {
        dbg_log_filter_disable(false);
        dbg_log_add_filter_rule(DBG_CAT_BLE);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_BLE added...\n");

    } else if (strcmp(cmd.data, "log filter add DBG_CAT_TEMP") == 0) {
        dbg_log_filter_disable(false);
        dbg_log_add_filter_rule(DBG_CAT_TEMP);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_TEMP added...\n");

    } else if (strcmp(cmd.data, "log filter add DBG_CAT_TAG_LF") == 0) {
        dbg_log_filter_disable(false);
        dbg_log_add_filter_rule(DBG_CAT_TAG_LF);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_TAG_LF added...\n");

    // log filter remove -------------------------------------------------------
    } else if (strcmp(cmd.data, "log filter add DBG_CAT_TAG_MAIN") == 0) {
        dbg_log_remove_filter_rule(DBG_CAT_TAG_MAIN);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_TAG_MAIN removed...\n");

    } else if (strcmp(cmd.data, "log filter remove DBG_CAT_BLE") == 0) {
        dbg_log_remove_filter_rule(DBG_CAT_BLE);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_BLE removed...\n");

    } else if (strcmp(cmd.data, "log filter remove DBG_CAT_TEMP") == 0) {
        dbg_log_remove_filter_rule(DBG_CAT_TEMP);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_TEMP removed...\n");

    } else if (strcmp(cmd.data, "log filter remove DBG_CAT_TAG_LF") == 0) {
        dbg_log_remove_filter_rule(DBG_CAT_TAG_LF);
        printf(COLOR_B_WHITE"\n\n-> Log filter DBG_CAT_TAG_LF removed...\n");

    // log filter list ---------------------------------------------------------
    } else if (strcmp(cmd.data, "log filter list") == 0) {
        printf(COLOR_B_WHITE"\n\n-> Available log filters:\n"
                                "   DBG_CAT_TAG_MAIN\n"
                                "   DBG_CAT_BLE\n"
                                "   DBG_CAT_TEMP\n"
                                "   DBG_CAT_TAG_LF\n");

    // write/read custom MAC address to NVM and reset to apply -----------------
    } else if (strstr(cmd.data, "write mac") != NULL) {
        if(cli_decode_mac_addr() == 0) {
            DEBUG_LOG(DBG_CAT_CLI, "New MAC was saved into NVM... rebooting to apply new settings");
            tpm_schedule_system_reset(1000);
        } else {
            DEBUG_LOG(DBG_CAT_WARNING, "Syntax error... expected MAC address format -> XX:XX:XX:XX:XX:XX");
        }

    } else if (strstr(cmd.data, "read mac") != NULL) {
        uint8_t mac[6];
        nvm_read_mac_address(mac);
        cli_print_mac(mac);

    // write beacon rate settings to NVM and apply -----------------------------
    } else if (strstr(cmd.data, "write beacon rate") != NULL) {

        int ret;
        uint32_t slow_rate;
        uint32_t fast_rate;

        ret = sscanf(cmd.data, "%*s %*s %*s %lu %lu", &slow_rate, &fast_rate);

        if (ret == 2) {
            if ( (slow_rate > 0) &&
                 (fast_rate > 0) &&
                 (slow_rate < 0xFFFF) &&
                 (fast_rate < 0xFFFF) ) {

                tbm_nvm_data_t b;

                b.is_erased = false;
                b.slow_rate = (uint16_t)slow_rate;
                b.fast_rate = (uint16_t)fast_rate;

                DEBUG_LOG(DBG_CAT_CLI, "Writing to NVM...");
                nvm_write_tbm_settings(&b);

                DEBUG_LOG(DBG_CAT_CLI, "Applying new settings to Tag Beacon Machine...");
                tbm_apply_new_settings(&b);

            } else {
                DEBUG_LOG(DBG_CAT_WARNING, "Error, value range must be 1 to 65353");
            }
        } else {
            DEBUG_LOG(DBG_CAT_WARNING, "Syntax error... unexpected command");
        }

    // stop cli ----------------------------------------------------------------
    } else if (strcmp(cmd.data, "cli stop") == 0) {
        cli_stop();

    // git info ----------------------------------------------------------------
    } else if (strcmp(cmd.data, "git info") == 0) {
        dbg_print_git_info();

    // reset -------------------------------------------------------------------
    } else if (strcmp(cmd.data, "reset") == 0) {
        printf(COLOR_B_WHITE"\n\n-> Rebooting...\n");
        NVIC_SystemReset();

    // banner ------------------------------------------------------------------
    } else if (strcmp(cmd.data, "banner") == 0) {
        dbg_print_banner();

    // usage -------------------------------------------------------------------
    } else if ((strcmp(cmd.data, "?") == 0)) {
        printf("\n\nusage: command [OPTION]...\n\n"                                                                      \
  COLOR_B_WHITE"   Commands                Options                  Description\n"COLOR_RST                              \
               "   log                     [on, off]                -> Turn log prints on/off\n"                         \
               "   -----------------------------------------------------------------------------------------\n"          \
               "   log filter              [on, off]                -> Log filter on/off\n"                              \
               "   log filter add          [dbg_log_filters_t]      -> Add log filter to list\n"                         \
               "   log filter remove       [dbg_log_filters_t]      -> Remove log filter from list\n"                    \
               "   log filter list                                  -> List available filters\n"                         \
               "   -----------------------------------------------------------------------------------------\n"          \
               "   write mac <address>                              -> Write MAC address into NVM.\n"                    \
               "                                                             <address> format -> XX:XX:XX:XX:XX:XX\n"    \
               "   read mac                                         -> Read MAC address\n"                               \
               "   -----------------------------------------------------------------------------------------\n"          \
               "   write beacon rate <slow> <fast>                  -> Write beacon rate into NVM and apply.\n"          \
               "                                                             <slow> - 1 to 65353 (x1 sec)\n"             \
               "                                                             <fast> - 1 to 65353 (x250 mS)\n"            \
               "   -----------------------------------------------------------------------------------------\n"          \
               "   cli stop                                         -> Stop cli process\n"                               \
               "   git info                                         -> Show git info\n"                                  \
               "   reset                                            -> System reset\n"                                   \
               "   banner                                           -> Show banner\n");

    } else {
        if(strcmp(cmd.data,"") != 0) {
            printf("\n... type \"?\" for commands usage\n> ");
        }
    } //--- END OF INPUT DECODE PROCESS

    printf("\n> ");

}

static void cli_refresh_echo(void)
{
    uint8_t j;
    // Refresh user console
    printf(ERASE_LINE "\r> ");
    for(j = 0; j < input.pos; ++j) {
        printf("%c", input.data[j]);
    }

    // Check for input buffer limits
    if (input.pos > (CLI_BUFFER_SIZE - 1)) {
        // Send backspace
        printf("\b");
    }
}

/**
 * @brief Process current line input (buffering)
 * @param char c
 */
static void cli_process_input(char c)
{
    size_t num_bytes;

    // Ignore some non printable keys (too keep this terminal simple we dont care about arrows, home, end keys)
    if (c == NON_PRINTABLE_CHAR) {
        do {
            sl_iostream_read(sl_iostream_uart_debug_handle, &c, sizeof(c), &num_bytes);
        } while(num_bytes > 0);
        return;
    }

    if (c == CTRL_C) {
        cli_clear_input_buffer();
        // stop logging
        dbg_log_enable(false);
        printf("\n");

    } else {
        if (c == BACKSPACE) {
            if (input.pos > 0) {
                input.data[--input.pos] = '\0';
            }
        } else {
            if (input.pos > 164) {
                printf("\b");
            } else {
                input.data[input.pos] = c;
                input.pos++;
            }
        }
    }

    // Remote echo
    if(boot_get_mode() == BOOT_DEFAULT) {
        cli_refresh_echo();
    }
}

/**
 * @brief Main CLI Process
 * @details CLI SLEEPTIMER API callback to process remote terminal inputs
 * @param sl_sleeptimer_timer_handle_t
 * @param void* data
 */
static void cli_run_fsm(sl_sleeptimer_timer_handle_t *handle, void *data)
{
    (void)(data);
    (void)(handle);

    char c;
    size_t num_bytes;

    sl_iostream_read(sl_iostream_uart_debug_handle, &c, sizeof(c), &num_bytes);

    if (num_bytes > 0) {
        if (c != ENTER_KEY) {
            // Keep processing input buffering
            cli_process_input(c);
        } else {
            // Decode input buffer
            if(boot_get_mode() == BOOT_MANUFACTURING_TEST) {
                cli_process_manufacturing_command();
            } else {
                cli_process_command();
            }
        }
    }
}


//******************************************************************************
// Non Static functions
//******************************************************************************
/**
 * @brief Start CLI Process
 */
void cli_start(void)
{
#if defined(TAG_CLI_PRESENT)

    sl_status_t status;

    cli_timer_handle = &cli_timer_inst;

    cli_clear_input_buffer();

    sl_iostream_uart_set_rx_energy_mode_restriction(sl_iostream_uart_uart_debug_handle, true);

    status = sl_sleeptimer_start_periodic_timer_ms( cli_timer_handle,
                                                    CLI_SLEEPTIMER_TICK_MS,
                                                    cli_run_fsm,
                                                    NULL,
                                                    5,
                                                    0);
    if (status == 0) {
        DEBUG_LOG(DBG_CAT_SYSTEM, "Starting CLI process...");
    }

#endif
}

/**
 * @brief Stop CLI Process
 */
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

