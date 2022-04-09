/***************************************************************************//**
 * @file
 * @brief Core application logic.
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

#include "em_common.h"
#include "em_cmu.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "sl_device_init_hfxo.h"
#include "gatt_db.h"
#include "stdbool.h"
#include "ble_manager_machine.h"
#include "app_assert.h"
#include "dbg_utils.h"

//! The advertising set handle allocated from Bluetooth stack.
uint8_t advertising_set_handle = 0xff;

static void ble_api_on_system_boot(sl_bt_msg_t *evt) {

    (void)evt;
    sl_status_t sc;
    bd_addr address;
    uint8_t address_type;
    uint8_t system_id[8];

    //! Extract unique ID from BT Address.
    sc = sl_bt_system_get_identity_address(&address, &address_type);
    app_assert_status(sc);

    //! Pad and reverse unique ID to get System ID.
    system_id[0] = address.addr[5];
    system_id[1] = address.addr[4];
    system_id[2] = address.addr[3];
    system_id[3] = 0xFF;
    system_id[4] = 0xFE;
    system_id[5] = address.addr[2];
    system_id[6] = address.addr[1];
    system_id[7] = address.addr[0];

    sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                                 0,
                                                 sizeof(system_id),
                                                 system_id);
    app_assert_status(sc);

    // Create an advertising set.
    sc = sl_bt_advertiser_create_set(&advertising_set_handle);
    app_assert_status(sc);
}


/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
    sl_status_t sc;
    (void)(sc);

    switch (SL_BT_MSG_ID(evt->header)) {
        default:
            break;

        case sl_bt_evt_system_boot_id:
            ble_api_on_system_boot(evt);
            break;

        case sl_bt_evt_advertiser_timeout_id:
            bmm_adv_running = false;
            DEBUG_LOG(DBG_CAT_BLE, "Stop BLE beacon advertising...");
            break;

        case sl_bt_evt_connection_opened_id:
            sl_bt_advertiser_stop(advertising_set_handle);
            DEBUG_LOG(DBG_CAT_BLE, "BLE connected...");
            break;

        case sl_bt_evt_connection_closed_id:
            DEBUG_LOG(DBG_CAT_BLE, "BLE disconnected...");
            break;

   ///////////////////////////////////////////////////////////////////////////
   // Add additional event handlers here as your application requires!      //
   ///////////////////////////////////////////////////////////////////////////

    }
}

void ble_api_fsm_run(void)
{
    sl_bt_step();
}
