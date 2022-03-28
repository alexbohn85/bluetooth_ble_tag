/***************************************************************************//**
 * @file
 * @brief main() Function
 *******************************************************************************
 * Main function and Initializations
 *
 */

#include "sl_component_catalog.h"
#include "sl_system_init.h"
#include "sl_power_manager.h"
#include "sl_system_process_action.h"
#include "em_common.h"
#include "em_cmu.h"
#include "em_rmu.h"
#include "em_gpio.h"
#include "app_assert.h"
#include "app_properties.h"
#include "gatt_db.h"
#include "sl_bluetooth.h"
#include "sl_spidrv_instances.h"
#include "sl_iostream_init_usart_instances.h"
#include "sl_sleeptimer.h"
#include "sl_udelay.h"
#include "stdio.h"

/* Build generated git log information */
#include "git_log.h"

/* Keep tag specific headers here */
#include "tag_defines.h"
#include "tag_main_machine.h"
#include "ble_api.h"
#include "ble_manager_machine.h"
#include "as393x.h"
#include "wdog.h"
#include "rtcc.h"
#include "dbg_utils.h"
#include "boot.h"
#include "cli.h"
#include "nvm.h"
#include "adc.h"
#include "lf_decoder.h"

#if !defined(SL_CATALOG_POWER_MANAGER_PRESENT)
#error "This platform requires Power Manager Service, install it using configurator!"
#endif

#if !defined(SL_SPIDRV_INSTANCES_H)
#error "This platform requires SPIDRV for AS393x, install it using configurator!"
#endif

#if !defined(SL_CATALOG_BLUETOOTH_FEATURE_ADVERTISER_PRESENT)
#error "This platform requires Bluetooth Advertiser, install it using configurator!"
#endif

#if !defined(SL_CATALOG_SLEEPTIMER_PRESENT)
#error "This platform requires Sleeptimer Service, install it using configurator!"
#endif

#define TAG_LOWEST_POWER_MODE    (SL_POWER_MANAGER_EM2)


//------------------------------------------------------------------------------
// Tag Main Initialization
//------------------------------------------------------------------------------
void tag_init(void)
{
    // Initialize UART debug logs (print FW banner)
    dbg_log_init();

#if defined(TAG_DEV_MODE_PRESENT)
    dbg_log_enable(true);
    dbg_trap_enable(false);
    dbg_print_banner();
#endif

    //TODO this cause a crash on reset
    //adc_init();

    // Print reset cause
    boot_print_reset_cause();

    // Set what is the lowest power mode for the Power Manager Service can enter.
    // For instance: LF Decoder requires EM2 to run with 32KHz LFXO crystal and it needs to run all the time.
    sl_power_manager_add_em_requirement(TAG_LOWEST_POWER_MODE);
    DEBUG_LOG(DBG_CAT_SYSTEM, "Power Manager lowest power mode enabled is EM%d", TAG_LOWEST_POWER_MODE);

    // Tell Power Manager if we want to stay awake in main context or go to sleep immediately after an ISR.
    tag_sleep_on_isr_exit(true);

    // Initialize AS3933 device driver
    as39_init(sl_spidrv_as39_spi_handle);

    // Keep antenna disabled for now
    as39_antenna_enable(false, false, false);

    // Initialize RTCC (used by Tag Main Machine and LF Decoder)
    rtcc_init();

    // Init LF Decoder
    lf_decoder_init();

#if defined(TAG_WDOG_PRESENT)
    watchdog_init();
    DEBUG_LOG(DBG_CAT_SYSTEM, "Initializing WDOG process...");
#endif

    // Initialize Tag Main Machine "Main System Tick"
    switch(boot_get_mode()) {

        case BOOT_NORMAL:
            DEBUG_LOG(DBG_CAT_SYSTEM, "Boot Select (boot_mode = 0x%.2X) -> Normal mode", boot_get_mode());
            tmm_start_normal_mode();
#if defined(TAG_DEV_MODE_PRESENT)
            cli_start();
#endif
            break;

        case BOOT_MANUFACTURING_TEST:
            DEBUG_LOG(DBG_CAT_SYSTEM, "Boot Select (boot_mode = 0x%.2X) -> Manufacturing mode", boot_get_mode());
            tmm_start_manufacturing_mode();
            cli_start();
            break;

        default:
            DEBUG_LOG(DBG_CAT_SYSTEM, "ERROR Boot Select (boot_mode = 0x%.2X) -> Not recognized...\n", boot_get_mode());
            DEBUG_LOG(DBG_CAT_SYSTEM, "\n.... Booting in NORMAL MODE!\n");
            tmm_start_normal_mode();
            break;
    }

    // At this point Tag Main Context stays in this loop forever. In this loop
    // there is a call to BLE Stack to process scheduled tasks events.
    while (1)
    {
        // BLE API events
        ble_api_fsm_run();
        sl_power_manager_sleep();
    }
}


//------------------------------------------------------------------------------
// Main System Initialization
//------------------------------------------------------------------------------
int main(void)
{
    //*************************************************************************
    // Custom Boot Select
    //*************************************************************************
    boot_init();

    //*************************************************************************
    // Initialize Silicon Labs device, system, service(s) and protocol stack(s).
    //*************************************************************************
    sl_system_init();

    //*************************************************************************
    // Tag System Initialization
    //*************************************************************************
    tag_init();

}




