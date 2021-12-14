/*
 * dbg_utils.c
 *
 */

#include "stdio.h"
#include "stdbool.h"
#include "stdarg.h"
#include "sl_sleeptimer.h"
#include "sl_iostream.h"
#include "sl_iostream_init_usart_instances.h"
#include "sl_iostream_stdlib_config.h"
#include "app_properties.h"
#include "tag_main_machine.h"
#include "ble_manager_machine.h"
#include "temperature_machine.h"
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

void uptime_to_string(char *buf, size_t size)
{
#if (SL_SLEEPTIMER_WALLCLOCK_CONFIG == 1)
    sl_sleeptimer_date_t date = {0};
    sl_sleeptimer_timestamp_t timestamp = sl_sleeptimer_get_time();
    sl_sleeptimer_convert_time_to_date(timestamp, 0, &date);
    snprintf(buf, size, "%d days %.2d:%.2d:%.2d ", (date.day_of_year - 1), date.hour, date.min, date.sec);
#endif
}

void print_timestamp(void)
{
#if (SL_SLEEPTIMER_WALLCLOCK_CONFIG == 1)
    sl_sleeptimer_date_t date = {0};
    sl_sleeptimer_timestamp_t timestamp = sl_sleeptimer_get_time();
    sl_sleeptimer_convert_time_to_date(timestamp, 0, &date);
    printf("\n%.2d:%.2d:%.2d ", date.hour, date.min, date.sec);
#endif
}

void dbg_print_banner(void)
{
    char uptime[80];
    uptime_to_string(uptime, sizeof(uptime));

    printf(COLOR_WHITE "\n\n=========================================================================" COLOR_RST "\n");
    printf(COLOR_B_YELLOW  "                   BLE Tag Firmware Initialization                       " COLOR_RST "\n");
    printf(COLOR_WHITE     "=========================================================================\n");
    printf(COLOR_B_WHITE "%35s | " COLOR_CYAN "%.1d.%.1d.%.1d.%.1d \n", "Tag Firmware Version", FW_B4, FW_B3, FW_B2, FW_B1);
    printf(COLOR_B_WHITE "%35s | " COLOR_CYAN "%s\n", "Build Date", COMPILATION_TIME);
    printf(COLOR_B_WHITE "%35s | " COLOR_CYAN "%s (0x%.2X)\n", "Tag Type", tag_tag_type_to_string(), tag_get_tag_type());

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

#if defined(TAG_DEBUG_MODE_PRESENT)
    printf(COLOR_B_WHITE "%35s | %s\n", "Debug Mode", COLOR_B_GREEN "Enabled");
#else
    printf(COLOR_B_WHITE "%35s | %s\n", "Debug Mode", COLOR_B_RED "Disabled");
#endif

#if defined(TAG_CLI_PRESENT)
    printf(COLOR_B_WHITE "%35s | %s\n", "CLI", COLOR_B_GREEN "Enabled");
#else
    printf(COLOR_B_WHITE "%35s | %s\n", "CLI", COLOR_B_RED "Disabled");
#endif
    printf(COLOR_WHITE     "-------------------------------------------------------------------------\n");

    printf(COLOR_B_WHITE "%35s | %s\n", "Logger", (dbg_is_log_enabled() ? COLOR_B_GREEN "On" : COLOR_B_RED "Off"));
    printf(COLOR_B_WHITE "%35s | %s\n", "Traps", (dbg_is_trap_enabled() ? COLOR_B_GREEN "On" : COLOR_B_RED "Off"));
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN" %4d msec\n", "TMM Tick", TMM_DEFAULT_TICK_PERIOD_MS);
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN" %4d msec\n", "TTM Slow Tasks Tick", TMM_DEFAULT_SLOW_TIMER_TICK_PERIOD_MS);
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN" %4d sec\n", "Beacon Rate (Fast)", BEACON_FAST_RATE_SEC);
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN" %4d min\n", "Beacon Rate (Slow)", (int)(BEACON_SLOW_RATE_SEC/60));
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN" %4d sec\n", "Temperature Report Rate", (int)(TTM_TIMER_PERIOD_SEC));
    printf(COLOR_B_WHITE "%35s |"COLOR_CYAN"    %s\n", "Uptime", uptime);

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
#if defined(DBG_LOG)
    log_enable = value;
#else
    (void)(value);
    log_enable = false;
#endif
}

//! @brief DEBUG_TRAP (enable/disable)
void dbg_trap_enable(bool value)
{
#if defined(DBG_LOG)
    trap_enable = value;
#else
    (void)(value);
    trap_enable = false;
#endif
}

bool dbg_is_log_enabled(void)
{
#if defined(DBG_LOG)
    return log_enable;
#else
    return false;
#endif
}

bool dbg_is_trap_enabled(void)
{
#if defined(DBG_LOG)
    return trap_enable;
#else
    return false;
#endif
}

void dbg_trap(void)
{
    while(1);
}

char* dbg_log_filter_to_string(dbg_log_filters_t filter)
{
    switch(filter) {
        case DBG_CAT_TAG_LF:
            return COLOR_B_WHITE "TAG_LF" COLOR_RST;
            break;
        case DBG_CAT_TAG_MAIN:
            return COLOR_B_WHITE "TAG_MAIN" COLOR_RST;
            break;
        case DBG_CAT_TAG_BEACON:
            return COLOR_B_WHITE "TAG_BEACON" COLOR_RST;
            break;
        case DBG_CAT_BLE:
            return COLOR_B_WHITE "BLE" COLOR_RST;
            break;
        case DBG_CAT_GENERIC:
            return COLOR_B_WHITE "GENERIC" COLOR_RST;
            break;
        case DBG_CAT_WARNING:
            return COLOR_B_YELLOW "WARNING" COLOR_RST;
            break;
        case DBG_CAT_SYSTEM:
            return COLOR_B_YELLOW "SYSTEM" COLOR_RST;
            break;
        case DBG_CAT_TEMP:
            return COLOR_B_WHITE "TEMPERATURE" COLOR_RST;
            break;
        default:
            return "UNKNOWN";
            break;
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

//!-----------------------------------------------------------------------------
//!-----------------------------------------------------------------------------
//TODO placing this here temporarily...
char* tag_tag_type_to_string(void)
{
    switch(TAG_TYPE) {
        case TAG_UT3:
            return "UT3";
            break;
        default:
            return "Undefined";
            break;
    }
}

uint8_t tag_get_tag_type(void)
{
    return TAG_TYPE;
}

uint32_t tag_get_fw_revision(void)
{
    return (FW_REV);
}
//!-----------------------------------------------------------------------------
//!-----------------------------------------------------------------------------

//TODO placing this here temporarily...
//! @brief Called right after waking up
void EMU_EFPEM23PresleepHook(void)
{
    // Debug
    //GPIO_PinOutClear(BOOTSEL_B2_PORT, BOOTSEL_B2_PIN);
    //CMU_ClockDivSet(cmuClock_CORE, 16);
}

//! @brief Called right after waking up
void EMU_EFPEM23PostsleepHook(void)
{
    // Debug
    //GPIO_PinOutSet(BOOTSEL_B2_PORT, BOOTSEL_B2_PIN);
}


//TODO find proper place for this
volatile bool sleep_on_isr_exit;
void tag_sleep_on_isr_exit(bool enable)
{
    sleep_on_isr_exit = enable;
    //DEBUG_LOG(DBG_CAT_SYSTEM, "tag_sleep_on_isr_exit = %s", (sleep_on_isr_exit ? "True" : "False"));
}

//! @brief Defines app behavior after exiting an isr
//! @note This behavior needs to be dynamic during runtime to allow BLE API to run
//! and when BLE is not in use going to sleep immediately after ISR helps to save power.
#if 1
sl_power_manager_on_isr_exit_t app_sleep_on_isr_exit(void)
{
    if (sleep_on_isr_exit) {
        return SL_POWER_MANAGER_SLEEP;
    } else {
        return SL_POWER_MANAGER_WAKEUP;
    }
}
#endif



#if 0
/*!
 * @brief DBG_UTILS - Unit Tests Template
 */
void dbg_log_unit_test(void)
{
    //! Enable Debug log prints
    dbg_log_enable(true);

    //! Disable Debug log prints
    dbg_log_enable(false);

    //! Enable Debug log category filtering
    dbg_log_filter_disable(false);

    //! Test Category Filter 1
    dbg_log_set_filter(DBG_CAT_BLE);

    //! Test Category Filter 2
    dbg_log_set_filter(DBG_CAT_TAG_LF);

    //! Test Category Filter Enable/Disable
    dbg_log_filter_disable(true);

    //! Additional tests..

}
#endif
