/*
 * dbg_utils.c
 *
 */

#include "stdio.h"
#include "stdbool.h"
#include "stdarg.h"
#include "em_gpio.h"
#include "sl_sleeptimer.h"
#include "sl_iostream.h"
#include "sl_iostream_init_usart_instances.h"
#include "sl_iostream_stdlib_config.h"
#include "app_properties.h"

#include "tag_main_machine.h"
#include "tag_beacon_machine.h"
#include "ble_manager_machine.h"
#include "temperature_machine.h"
#include "boot.h"
#include "version.h"
#include "dbg_utils.h"
#include "git_log.h"


volatile bool log_enable;
volatile bool trap_enable;
volatile bool log_filter_disabled;
volatile dbg_log_filters_t log_filter_mask;


void dbg_print_git_info(void)
{
    printf("\n%s\n%s\n%s", GIT_LINE_1, GIT_LINE_2, GIT_URL);
}

void dbg_print_banner(void)
{
    char uptime[80];
    tum_get_full_uptime_string(uptime, sizeof(uptime));

    printf(COLOR_WHITE "\n\n=========================================================================" COLOR_RST "\n");
    printf(COLOR_B_YELLOW  "                   BLE Tag Firmware Initialization                       " COLOR_RST "\n");
    printf(COLOR_WHITE     "=========================================================================\n");
    printf(COLOR_B_WHITE "%35s | " COLOR_CYAN "%.1d.%.1d.%.1d.%.1d \n", "Tag Firmware Version", firmware_revision[0],
                                                                                                firmware_revision[1],
                                                                                                firmware_revision[2],
                                                                                                firmware_revision[3]);
    printf(COLOR_B_WHITE "%35s | " COLOR_CYAN "%s\n", "Build Date", COMPILATION_TIME);
    printf(COLOR_B_WHITE "%35s | " COLOR_CYAN "%s (0x%.2X)\n", "Tag Type", tag_tag_type_to_string(), tag_get_tag_type_id());

    printf(COLOR_WHITE     "\n-------------------------- Compilation Switches -------------------------\n");

#if defined(TAG_BOOT_SELECT_PRESENT)
    printf(COLOR_B_WHITE "%35s | %s\n", "Boot Select", COLOR_B_GREEN "Enabled");
#else
    printf(COLOR_B_WHITE "%35s | %s\n", "Boot Select", COLOR_B_RED "Disabled");
#endif

#if defined(TAG_LOG_PRESENT)
    printf(COLOR_B_WHITE "%35s | %s\n", "Debug Log", COLOR_B_GREEN "Enabled");
#else
    printf(COLOR_B_WHITE "%35s | %s\n", "Debug Log", COLOR_B_RED "Disabled");
#endif

#if defined(TAG_TRAP_PRESENT)
    printf(COLOR_B_WHITE "%35s | %s\n", "Debug Trap", COLOR_B_GREEN "Enabled");
#else
    printf(COLOR_B_WHITE "%35s | %s\n", "Debug Trap", COLOR_B_RED "Disabled");
#endif

#if defined(TAG_WDOG_PRESENT)
    printf(COLOR_B_WHITE "%35s | %s\n", "Watchdog", COLOR_B_GREEN "Enabled");
#else
    printf(COLOR_B_WHITE "%35s | %s\n", "Watchdog", COLOR_B_RED "Disabled");
#endif

#if defined(TAG_DEV_MODE_PRESENT)
    printf(COLOR_B_WHITE "%35s | %s\n", "Debug Mode", COLOR_B_GREEN "Development");
#else
    printf(COLOR_B_WHITE "%35s | %s\n", "Debug Mode", COLOR_B_RED "Production");
#endif

#if defined(TAG_CLI_PRESENT)
    printf(COLOR_B_WHITE "%35s | %s\n", "CLI", COLOR_B_GREEN "Enabled");
#else
    printf(COLOR_B_WHITE "%35s | %s\n", "CLI", COLOR_B_RED "Disabled");
#endif
    printf(COLOR_WHITE     "-------------------------------------------------------------------------\n");

    printf(COLOR_B_WHITE "%35s | %s\n", "Logs", (dbg_is_log_enabled() ? COLOR_B_GREEN "On" : COLOR_B_RED "Off"));
    printf(COLOR_B_WHITE "%35s | %s\n", "Traps", (dbg_is_trap_enabled() ? COLOR_B_GREEN "On" : COLOR_B_RED "Off"));
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN" %4d msec\n", "TMM Tick", TMM_RTCC_TIMER_PERIOD_MS);
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN" %4d msec\n", "TTM Slow Tasks Tick", TMM_SLOW_TASK_TIMER_PERIOD_MS);
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN" %4d sec\n", "Beacon Rate (Fast)", TBM_FAST_BEACON_RATE_SEC);
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN" %4d sec\n", "Beacon Rate (Slow)", TBM_SLOW_BEACON_RATE_SEC);
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN" %4d sec\n", "Temperature Report Rate", (int)(TTM_TIMER_PERIOD_SEC_RELOAD));
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN" %4d sec\n", "Uptime Beacon Rate", (int)(TUM_TIMER_PERIOD_SEC));
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN"    %s\n", "Current Uptime", uptime);

    printf(COLOR_WHITE "-------------------------------------------------------------------------\n\n");
    printf(COLOR_RST);

}

void dbg_log_init(void)
{
#if defined(TAG_LOG_PRESENT)
    log_enable = false;
    trap_enable = false;
    log_filter_disabled = true;
    log_filter_mask = DBG_CAT_ALL_DISABLED;
    sl_iostream_set_default(sl_iostream_uart_debug_handle);
    sl_iostream_stdlib_disable_buffering();
    sl_sleeptimer_set_time(0);
    printf("\n%s Tag Booting...", tag_tag_type_to_string());
#endif
}

void dbg_log_deinit(void)
{
    //!TODO this is not working
#if 0
    cli_stop();
    trap_enable = false;
    log_enable = false;
    log_filter_disabled = true;
    log_filter_mask = DBG_CAT_ALL_DISABLED;
    sl_iostream_uart_deinit(sl_iostream_uart_uart_debug_handle);
#endif
}

//! @brief DEBUG_LOG (enable/disable)
void dbg_log_enable(bool value)
{
#if defined(TAG_LOG_PRESENT)
    log_enable = value;
#else
    (void)(value);
    log_enable = false;
#endif
}

//! @brief DEBUG_TRAP (enable/disable)
void dbg_trap_enable(bool value)
{
#if defined(TAG_LOG_PRESENT)
    trap_enable = value;
#else
    (void)(value);
    trap_enable = false;
#endif
}

bool dbg_is_log_enabled(void)
{
#if defined(TAG_LOG_PRESENT)
    return log_enable;
#else
    return false;
#endif
}

bool dbg_is_trap_enabled(void)
{
#if defined(TAG_LOG_PRESENT)
    return trap_enable;
#else
    return false;
#endif
}

void dbg_trap(void)
{
    DEBUG_LOG(DBG_CAT_SYSTEM, "__BKPT() -> connect debugger for analysis...");
    __BKPT(0);  // Allow to connect debugger
}

char* dbg_log_filter_to_string(dbg_log_filters_t filter)
{
    switch(filter) {
        case DBG_CAT_TAG_LF:
            return COLOR_B_WHITE "TAG_LF" COLOR_RST;
        case DBG_CAT_TAG_MAIN:
            return COLOR_B_WHITE "TAG_MAIN" COLOR_RST;
        case DBG_CAT_TAG_BEACON:
            return COLOR_B_WHITE "TAG_BEACON" COLOR_RST;
        case DBG_CAT_BLE:
            return COLOR_B_WHITE "BLE" COLOR_RST;
        case DBG_CAT_GENERIC:
            return COLOR_B_WHITE "GENERIC" COLOR_RST;
        case DBG_CAT_TEMP:
            return COLOR_B_WHITE "TEMPERATURE" COLOR_RST;
        case DBG_CAT_BATTERY:
            return COLOR_B_WHITE "BATTERY" COLOR_RST;
        case DBG_CAT_CLI:
            return COLOR_B_WHITE "CLI" COLOR_RST;
        case DBG_CAT_WARNING:
            return COLOR_B_RED "WARNING" COLOR_RST;
        case DBG_CAT_SYSTEM:
            return COLOR_B_BLUE "SYSTEM" COLOR_RST;
        default:
            return "UNKNOWN";
    }
}


void dbg_log_remove_filter_rule(dbg_log_filters_t filter)
{
    log_filter_mask ^= (uint32_t)filter;
}

void dbg_log_add_filter_rule(dbg_log_filters_t filter)
{
    log_filter_mask |= (uint32_t)filter;
}

void dbg_log_filter_disable(bool value)
{
    log_filter_disabled = value;
}

void dbg_log_set_filter(dbg_log_filters_t filter)
{
    log_filter_mask = (uint32_t)filter;
}
