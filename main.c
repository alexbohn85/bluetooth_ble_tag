/***************************************************************************//**
 * @file
 * @brief main() function.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

// Build generated git log information
#include "git_log.h"

#include "sl_component_catalog.h"
#include "sl_system_init.h"
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
#include "sl_power_manager.h"
#endif // SL_CATALOG_POWER_MANAGER_PRESENT
#if defined(SL_CATALOG_KERNEL_PRESENT)
#include "sl_system_kernel.h"
#else // SL_CATALOG_KERNEL_PRESENT
#include "sl_system_process_action.h"
#endif // SL_CATALOG_KERNEL_PRESENT
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

//! keep tag specific headers here
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
    //! Initialize UART debug logs (print FW banner)
    dbg_log_init();

#if defined(TAG_DEBUG_MODE_PRESENT)
    dbg_log_enable(false);
    dbg_print_banner();
#endif

    //! Set what is the lowest power mode for the Power Manager Service can enter.
    //! For instance: LF timer requires EM2 to run with 32KHz crystal and it needs to run all the time.
    sl_power_manager_add_em_requirement(TAG_LOWEST_POWER_MODE);
    DEBUG_LOG(DBG_CAT_SYSTEM, "Power Manager lowest power mode enabled is EM%d", TAG_LOWEST_POWER_MODE);

    //! Tell Power Manager if we want to stay awake in main context or go to sleep immediately after an ISR.
    tag_sleep_on_isr_exit(true);

    //EMU->CTRL_SET = EMU_CTRL_FLASHPWRUPONDEMAND;
    CMU_ClockDivSet(cmuClock_HCLK, 1);

    //! Initialize AS3933 device driver
    as39_init(sl_spidrv_as39_spi_handle, NULL);

    //! Keep antenna disabled for now
    as39_antenna_enable(false, false, false);
    as39_handle_t as39_hld;
    as39_get_handle(&as39_hld);
    as39_hld->registers.reg_0.ON_OFF = 0;
    as39_hld->registers.reg_0.MUX_123 = 0;
    as39_write_register(REG_0);

    //! Initialize RTCC (used by Tag Main Machine and LF Decoder)
    rtcc_init();

    //! Init LF Decoder
    lf_decoder_init();

    //! TODO Debug BLE on demand
    sl_bt_system_start_bluetooth();
    //sl_bt_system_stop_bluetooth();

#if defined(TAG_WDOG_PRESENT)
    watchdog_init();
    DEBUG_LOG(DBG_CAT_SYSTEM, "Initializing WDOG process...");
#endif


    //! Initialize Tag Main Machine "Main System Tick"
    switch(boot_read_mode()) {

        case BOOT_NORMAL:
            DEBUG_LOG(DBG_CAT_SYSTEM, "Boot Select (boot_mode = 0x%.2X) -> Normal mode", boot_read_mode());
            tmm_start_normal_mode();
#if defined(TAG_DEBUG_MODE_PRESENT)
            //cli_start();
#endif
            break;

        case BOOT_MANUFACTURING_TEST:
            DEBUG_LOG(DBG_CAT_SYSTEM, "Boot Select (boot_mode = 0x%.2X) -> Manufacturing mode", boot_read_mode());
            tmm_start_manufacturing_mode();
            cli_start();
            break;

        default:
            DEBUG_LOG(DBG_CAT_SYSTEM, "ERROR Boot Select (boot_mode = 0x%.2X) -> Not recognized...\n", boot_read_mode());
            DEBUG_LOG(DBG_CAT_SYSTEM, "\n.... Booting in NORMAL MODE!\n");
            tmm_start_normal_mode();
            break;
    }

    while (1)
    {
        //! TODO Compare how BLE API would perform inside a periodic sleeptimer that is started/stopped by BLE Manager
        //! BLE API events
        ble_api_fsm_run();
        sl_power_manager_sleep();
    }
}


//------------------------------------------------------------------------------
// Main System Initialization
//------------------------------------------------------------------------------
int main(void)
{
    //!*************************************************************************
    //! Custom Boot Select
    //!*************************************************************************
    boot_init();

    //!*************************************************************************
    //! Initialize Silicon Labs device, system, service(s) and protocol stack(s).
    //!*************************************************************************
    sl_system_init();

    //!*************************************************************************
    //! Tag System Initialization
    //!*************************************************************************
    tag_init();

}




