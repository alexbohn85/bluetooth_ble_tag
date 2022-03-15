/*
 * nvm.c
 * Interface with NVM3 Driver - This is helper module to interface between
 * NVM Machine and NVM3 Driver to store persistent tag data.
 *
 * Note: Flash operations duration are not predictable so it should not be started
 * from within the Tag Main Machine ISR context to avoid blocking it for too long.
 *
 * All NVM operations are scheduled from within NVM Machine using a sleeptimer
 * callback to this helper.
 */

#include "stdio.h"
#include "stdbool.h"
#include "string.h"

#include "sl_udelay.h"
#include "em_common.h"
#include "ecode.h"
#include "app_assert.h"
#include "nvm3.h"
#include "nvm3_default_config.h"


#include "dbg_utils.h"

#define NVM_TAG_MAX_DATA_SIZE     (16)

static uint8_t nvm_data_buffer[NVM_TAG_MAX_DATA_SIZE];

#if 1
//******************************************************************************
// Defines
//******************************************************************************
#define TAG_CONFIGURATION_INIT       {.op_mode = 0x00}
#define TAG_MAC_ADDR_INIT            {.addr = {0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB}}

// Create a NVM area of 8kB (size must equal N * FLASH_PAGE_SIZE, N is integer). Create a cache of  entries.
//NVM3_DEFINE_SECTION_STATIC_DATA(nvm_tag_storage_area, (1 * FLASH_PAGE_SIZE), 10);

//******************************************************************************
// Data types
//******************************************************************************
typedef struct nvm_tag_configuration_data_t {
    uint8_t op_mode;                                                            /* NORMAL_MODE, DEEP_SLEEP_MODE, MANUFACTURING_MODE */
} nvm_tag_configuration_data_t;

typedef struct nvm_tag_mac_address_t {
    uint8_t addr[6];                                                            /* Bluetooth address in reverse byte order */
} nvm_tag_mac_address_t;

/* IDs for NVM3 data locations. Each id(key) should be used to address one object only */
typedef enum nvm_tag_keys_t {
    NVM_MAC_ADDRESS_KEY = 1,
    NVM_TAG_CONFIGURATION_KEY,
    NVM_TAG_OP_MODE_KEY,
} nvm_tag_keys_t;

//******************************************************************************
// Global variables
//******************************************************************************
static nvm_tag_configuration_data_t tag_configuration = TAG_CONFIGURATION_INIT;
static nvm_tag_mac_address_t tag_mac_addr = TAG_MAC_ADDR_INIT;


//******************************************************************************
// Static functions
//******************************************************************************
static void nvm_clear_data_buffer(void)
{
    // Init nvm_data_buffer
    //memset(nvm_data_buffer, 0, sizeof(nvm_data_buffer));
}

//******************************************************************************
// Non Static functions
//******************************************************************************
void nvm_write_mac_address(uint8_t *data, uint8_t length)
{
    Ecode_t status;
    uint32_t obj_type;
    size_t obj_size;

    nvm_tag_mac_address_t check_value;

    // Copy data into nvm_tag_mac_address_t struct
    nvm_tag_mac_address_t new_value;
    memcpy(new_value.addr, data, length);

    nvm_clear_data_buffer();

    //status = nvm3_open(nvm3_defaultHandle, nvm3_defaultInit);

    //status = nvm3_getObjectInfo(nvm3_defaultHandle, (uint32_t)NVM_MAC_ADDRESS_KEY, &obj_type, &obj_size);


    uint8_t value = 10;

    // Write new values to NVM key
    nvm3_writeData(nvm3_defaultHandle, (uint32_t)NVM_MAC_ADDRESS_KEY, &value, sizeof(value));
    // Read new values from NVM key
    nvm3_readData(nvm3_defaultHandle, (uint32_t)NVM_MAC_ADDRESS_KEY, nvm_data_buffer, sizeof(value));

    //volatile nvm_tag_mac_address_t *read_mac = (nvm_tag_mac_address_t*)nvm_data_buffer;

    // Check if values were stored successfully.
    //memcpy(&check_value, nvm_data_buffer, sizeof(check_value));
    DEBUG_LOG(DBG_CAT_GENERIC, "value = 0x%.2X", nvm_data_buffer[0]);



    // Do repacking if needed
    if (nvm3_repackNeeded(nvm3_defaultHandle)) {
        status = nvm3_repack(nvm3_defaultHandle);
        if (status != ECODE_NVM3_OK) {
            DEBUG_LOG(DBG_CAT_GENERIC, "Error on nvm3_repack status = %lu", status);
        }
    }

    //nvm3_close(nvm3_defaultHandle);
}

void nvm_init(void)
{

    // Declare a nvm3_Init_t struct of name nvm3Data1 with initialization data. This is passed to nvm3_open() below.
    //NVM3_DEFINE_SECTION_INIT_DATA(nvm_tag_storage_area, &nvm3_halFlashHandle);

    volatile Ecode_t status;
    volatile size_t numberOfObjects;

    nvm_clear_data_buffer();

    //nvm3_close(nvm3_defaultHandle);
    //status = nvm3_open(nvm3_defaultHandle, nvm3_defaultInit);

    // Get the number of valid keys already in NVM3
    numberOfObjects = nvm3_countObjects(nvm3_defaultHandle);

    // Skip if we have initial keys. If not, generate objects and store
    // persistently in NVM3 before proceeding.
    if (numberOfObjects < 2)
    {
        // Erase all objects and write initial data to NVM3
        uint8_t value = 5;

        status = nvm3_writeData(nvm3_defaultHandle, 1, &value, sizeof(value));
        if (status != ECODE_NVM3_OK) {
          DEBUG_LOG(DBG_CAT_SYSTEM, "NVM3_OPEN Error %lu", status);
        }

        status = nvm3_writeData(nvm3_defaultHandle, 1, &value, sizeof(value));
        if (status != ECODE_NVM3_OK) {
          DEBUG_LOG(DBG_CAT_SYSTEM, "NVM3_OPEN Error %lu", status);
        }
    }

    // Do repacking if needed
    if (nvm3_repackNeeded(nvm3_defaultHandle)) {
        status = nvm3_repack(nvm3_defaultHandle);
        if (status != ECODE_NVM3_OK) {
            DEBUG_LOG(DBG_CAT_GENERIC, "Error on nvm3_repack status = %lu", status);
        }
    }
    //nvm3_close(nvm3_defaultHandle);

}

//void nvm_example(nvm_tag_mac_address_t *mac_addr, nvm_tag_configuration_data_t *tag_config)
//void nvm_example(uint8_t *data1, size_t size1, uint8_t *data2, size_t size2)
void nvm_example(void)
{
    Ecode_t status;
    size_t numberOfObjects;
    uint32_t objectType;
    unsigned char dataR[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    unsigned char data2[] = { 11, 12, 13, 14, 15 };
    nvm3_ObjectKey_t data1[20] = {0};
    size_t dataLen1;
    size_t dataLen2;
    //status = nvm3_deinitDefault();
    //status = nvm3_initDefault();
    //status = nvm3_open(nvm3_defaultHandle, &nvm3Data1);
    if (status != ECODE_NVM3_OK) {
      //Handle error
    }
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    // Get the number of valid keys already in NVM3
    numberOfObjects = nvm3_countObjects(nvm3_defaultHandle);
    DEBUG_LOG(DBG_CAT_SYSTEM, "numberOfObjects = %d", numberOfObjects);
    uint8_t num_keys = nvm3_enumObjects(nvm3_defaultHandle, data1, sizeof(data1), NVM3_KEY_MIN, NVM3_KEY_MAX);
    DEBUG_LOG(DBG_CAT_SYSTEM, "enumObjects returns num_keys = %d", num_keys);
    uint8_t i;
    for (i = 0; i < 20; i++) {
        DEBUG_LOG(DBG_CAT_SYSTEM, "data1[%d] = %lu", i, data1[i]);
    }

    // Skip if we have initial keys. If not, generate objects and store
    // persistently in NVM3 before proceeding.
    if (numberOfObjects < 2) {
      // Erase all objects and write initial data to NVM3
      //nvm3_eraseAll(nvm3_defaultHandle);
      //nvm3_writeData(nvm3_defaultHandle, 1, dataR, sizeof(dataR));
      nvm3_writeData(nvm3_defaultHandle, 4, data2, sizeof(data2));
      //nvm3_writeData(nvm3_defaultHandle, 1, data1, size1);
      //nvm3_writeData(nvm3_defaultHandle, 2, data2, size2);
      //DEBUG_LOG(DBG_CAT_SYSTEM, "NVM3 Write tag_mac_addr!");
      nvm3_writeData(nvm3_defaultHandle, 5, &tag_mac_addr, sizeof(tag_mac_addr));
      //DEBUG_LOG(DBG_CAT_SYSTEM, "NVM3 Write tag_configuration!");
      //nvm3_writeData(nvm3_defaultHandle, NVM_TAG_CONFIGURATION_KEY, &tag_configuration, sizeof(tag_configuration));
    }

    // Find size of data for object with key identifier 1 and 2 and read out
    //nvm3_getObjectInfo(nvm3_defaultHandle, 1, &objectType, &dataLen1);
    nvm3_getObjectInfo(nvm3_defaultHandle, data1[0], &objectType, &dataLen1);
    if (objectType == NVM3_OBJECTTYPE_DATA) {
        nvm3_readData(nvm3_defaultHandle, data1[0], data1, dataLen1);
        DEBUG_LOG(DBG_CAT_SYSTEM, "NVM3 Read ID1 value %lu type = NVM3_OBJECTTYPE_DATA and data size = %d!", data1[0], dataLen1);
        DEBUG_LOG(DBG_CAT_SYSTEM, "NVM3 Read ID2 value %lu type = NVM3_OBJECTTYPE_DATA and data size = %d!", data1[1], dataLen1);
        DEBUG_LOG(DBG_CAT_SYSTEM, "NVM3 Read ID3 value %lu type = NVM3_OBJECTTYPE_DATA and data size = %d!", data1[2], dataLen1);
        DEBUG_LOG(DBG_CAT_SYSTEM, "NVM3 Read ID4 value %lu type = NVM3_OBJECTTYPE_DATA and data size = %d!", data1[3], dataLen1);
    } else {
        DEBUG_LOG(DBG_CAT_SYSTEM, "NVM3 Read ID %lu type = NVM3_OBJECTTYPE_COUNTER data size = %d!", data1[0], dataLen1);
    }

    nvm3_getObjectInfo(nvm3_defaultHandle, 1, &objectType, &dataLen1);
    if (objectType == NVM3_OBJECTTYPE_DATA) {
        //nvm3_readData(nvm3_defaultHandle, 1, data1, dataLen1);
        DEBUG_LOG(DBG_CAT_SYSTEM, "NVM3 Read ID 1 type = NVM3_OBJECTTYPE_DATA and data size = %d!", dataLen1);
    }

    nvm3_getObjectInfo(nvm3_defaultHandle, 2, &objectType, &dataLen1);
    if (objectType == NVM3_OBJECTTYPE_DATA) {
        DEBUG_LOG(DBG_CAT_SYSTEM, "NVM3 Read ID 2 type = NVM3_OBJECTTYPE_DATA and data size = %d!", dataLen1);
    }

    //memcpy(tag_mac_addr.addr, mac_addr->addr, sizeof(mac_addr->addr));
    status = nvm3_readData(nvm3_defaultHandle, 1, dataR, sizeof(dataR));
    status = nvm3_readData(nvm3_defaultHandle, 4, data2, sizeof(data2));
    status = nvm3_readData(nvm3_defaultHandle, 5, &tag_mac_addr, sizeof(tag_mac_addr));
    // Update values
    dataR[0]++;
    data2[0]++;
    tag_mac_addr.addr[0]++;
    status = nvm3_writeData(nvm3_defaultHandle, 1, dataR, sizeof(dataR));
    if (status == ECODE_NVM3_OK) {
        printf("\nWrite OK!");
    } else {
        printf("\nWrite Error %lu", status);
    }

    status = nvm3_writeData(nvm3_defaultHandle, 4, data2, sizeof(data2));
    if (status == ECODE_NVM3_OK) {
        printf("\nWrite OK!");
    } else {
        printf("\nWrite Error %lu", status);
    }

    status = nvm3_writeData(nvm3_defaultHandle, 5, &tag_mac_addr, sizeof(tag_mac_addr));
    if (status == ECODE_NVM3_OK) {
        printf("\nWrite OK!");
    } else {
        printf("\nWrite Error %lu", status);
    }
    //nvm3_writeData(nvm3_defaultHandle, 2, data2, dataLen2);

    //nvm3_writeData(nvm3_defaultHandle, 1, &tag_mac_addr, dataLen1);
    //nvm3_writeData(nvm3_defaultHandle, NVM_TAG_CONFIGURATION_KEY, &tag_configuration, dataLen2);

    // Do repacking if needed
    if (nvm3_repackNeeded(nvm3_defaultHandle)) {
      status = nvm3_repack(nvm3_defaultHandle);
      if (status != ECODE_NVM3_OK) {
        // Handle error
      }
    }
    CORE_EXIT_CRITICAL();
    //nvm3_close(nvm3_defaultHandle);
    printf("\ndataR[0]=%d", dataR[0]);
    printf("\ndata2[0]=%d", data2[0]);
    printf("\nMAC[0]=0x%2X", tag_mac_addr.addr[0]);
}
#endif
