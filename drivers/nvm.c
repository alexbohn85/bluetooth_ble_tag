/*
 * nvm.c
 */

#include "stdio.h"
#include "stdbool.h"
#include "string.h"

#include "sl_udelay.h"
#include "sl_bt_api.h"
#include "em_common.h"
#include "ecode.h"
#include "app_assert.h"
#include "nvm3_default_config.h"
#include "tag_beacon_machine.h"
#include "nvm3.h"

#include "dbg_utils.h"

//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************

/* IDs for NVM3 data locations. Each id(key) should be used to address one object only */
typedef enum nvm_tag_keys_t {
    NVM_MAC_ADDRESS_KEY = 0,
    NVM_TAG_CONFIGURATION_KEY,
    NVM_TAG_OP_MODE_KEY,
    NVM_TBM_BEACON_RATE_KEY,
} nvm_tag_keys_t;

//******************************************************************************
// Global variables
//******************************************************************************

//******************************************************************************
// Static functions
//******************************************************************************
static void nvm_repack(void)
{
    Ecode_t ret;
    // Do repacking if needed
    if (nvm3_repackNeeded(nvm3_defaultHandle)) {
        ret = nvm3_repack(nvm3_defaultHandle);
      if (ret != ECODE_NVM3_OK) {
         DEBUG_LOG(DBG_CAT_SYSTEM, "NVM Repack Error! 0x%lX", (uint32_t)ret);
      }
    }
}

//******************************************************************************
// Non Static functions
//******************************************************************************
uint32_t nvm_read_mac_address(uint8_t *mac)
{
    Ecode_t ret;
    uint32_t status;

    ret = nvm3_readData(nvm3_defaultHandle, NVM_MAC_ADDRESS_KEY, mac, 6);

    if (ret == ECODE_NVM3_OK) {
        status = 0;
    } else {
        status = 1;
    }

    nvm_repack();

    return status;
}

uint32_t nvm_write_mac_address(uint8_t *mac)
{
    Ecode_t ret;
    uint32_t status;

    // Write to NVM user area
    ret = nvm3_writeData(nvm3_defaultHandle, NVM_MAC_ADDRESS_KEY, mac, 6);

    // Write to NVM BLE Core area (tag needs to reboot to apply new mac address)
    bd_addr bd_mac;
    memcpy(bd_mac.addr, mac, 6);
    ret = sl_bt_system_set_identity_address(bd_mac, 0);

    if (ret == ECODE_NVM3_OK) {
        status = 0;
    } else {
        status = 1;
    }

    nvm_repack();

    return status;
}

uint32_t nvm_read_tbm_settings(tbm_nvm_data_t *b)
{
    Ecode_t ret;
    uint32_t status;

    ret = nvm3_readData(nvm3_defaultHandle, NVM_TBM_BEACON_RATE_KEY, b, sizeof(tbm_nvm_data_t));

    if (ret == ECODE_NVM3_OK) {
        status = 0;
    } else {
        status = 1;
    }

    nvm_repack();

    return status;
}

uint32_t nvm_write_tbm_settings(tbm_nvm_data_t *b)
{
    Ecode_t ret;
    uint32_t status;

    // Write to NVM user area
    ret = nvm3_writeData(nvm3_defaultHandle, NVM_TBM_BEACON_RATE_KEY, b, sizeof(tbm_nvm_data_t));

    if (ret == ECODE_NVM3_OK) {
        status = 0;
    } else {
        status = 1;
    }

    nvm_repack();

    return status;
}



