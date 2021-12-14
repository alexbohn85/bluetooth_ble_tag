/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include "em_common.h"
#include "sl_app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "em_gpio.h"
#include "custom_adv.h"
#include "sl_app_log.h"
#include "lfDecoding.h"

CustomAdv_t sData; // Our custom advertising data stored here

uint8_t upperLFData = 0xFF;
uint8_t lowerLFData = 0xFF;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static void handle_button_press () {

  uint16_t tagID = getLfData();

  upperLFData = (tagID >> 8 & 0x00ff);
  lowerLFData = (tagID & 0x00ff);

  // Update the advertising data on-the-fly
  update_adv_data(&sData, advertising_set_handle, upperLFData, lowerLFData);
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init (void) {
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action (void) {
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event (sl_bt_msg_t *evt) {
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
  // -------------------------------
  // This event indicates the device has started and the radio is ready.
  // Do not call any stack command before receiving this boot event!
  case sl_bt_evt_system_boot_id:

    sl_app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Failed to write attribute\n", (int )sc);

    // Create an advertising set.
    sc = sl_bt_advertiser_create_set(&advertising_set_handle);
    sl_app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Failed to create advertising set\n", (int )sc);

    // Set advertising interval to 100ms.
    sc = sl_bt_advertiser_set_timing(advertising_set_handle,
        16000, // min. adv. interval (milliseconds * 1.6)
        16000, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
    sl_bt_advertiser_set_channel_map(advertising_set_handle, 1);

    fill_adv_packet(&sData, 0x06, 0x02FF, upperLFData, lowerLFData, "BLE");
    start_adv(&sData, advertising_set_handle);
    sl_app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Failed to set advertising timing\n", (int )sc);
    sl_app_log("Start advertising ...\n");
    break;

    // -------------------------------
    // This event indicates that a new connection was opened.
  case sl_bt_evt_connection_opened_id:
    break;

    // -------------------------------
    // This event indicates that a connection was closed.
  case sl_bt_evt_connection_closed_id:
    // Restart advertising after client has disconnected.
    sc = sl_bt_advertiser_start(advertising_set_handle, advertiser_user_data,
        advertiser_non_connectable);
    sl_app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Failed to start advertising\n", (int )sc);
    break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

  case sl_bt_evt_system_external_signal_id:
    if (evt->data.evt_system_external_signal.extsignals & 0x1) {
      handle_button_press();
    }
    break;

    // -------------------------------
    // Default event handler.
  default:
    break;
  }
}
