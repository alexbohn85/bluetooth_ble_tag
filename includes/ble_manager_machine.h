/*
 * ble_manager_machine.c
 *
 *  This module is the only interface between Tag Application and BLE. Which
 *  means it should encapsulate BLE concerns. Tag Application should not make
 *  BLE API calls directly.
 */

#ifndef BLE_MANAGER_MACHINE_H_
#define BLE_MANAGER_MACHINE_H_

#include "stdbool.h"
#include "tag_main_machine.h"

//******************************************************************************
// Defines
//******************************************************************************
#define BLE_MSG_MAX_DATASIZE         (23)

//******************************************************************************
// Extern global variables
//******************************************************************************
extern volatile bool bmm_adv_running;
extern volatile bool bmm_stack_running;

//******************************************************************************
// Data types
//******************************************************************************
typedef enum bmm_states_t {
    BMM_PROCESS_CFG_REQ,
    BMM_PROCESS_MSG_EVENTS,
    BMM_START_ADV,
    BMM_START,
    BMM_EXIT
} bmm_states_t;

typedef struct bmm_fsm_t {
    bmm_states_t state;
    bmm_states_t return_state;
} bmm_fsm_t;

/** BLE Tag Beacon Message Types **/
enum bmm_msg_type_enum {
    BLE_MSG_TAG_STATUS       = 0x01,  /* Tag Status: Motion, Tamper, Ambient Light, etc */
    BLE_MSG_LF_FIELD         = 0x02,  /* Entering, Exiting, Staying in TE or MT Field */
                                      /* 0x03 and 0x04 not used on UT3 to allow compatibility with 433MHz */
    BLE_MSG_BUTTON_PRESS     = 0x05,  /* Button Press message */
    BLE_MSG_FALL_DETECTION   = 0x06,  /* Fall Detection message */
    BLE_MSG_TEMPERATURE      = 0x07,  /* Temperature message */
    BLE_MSG_TAG_EXT_STATUS   = 0x08,  /* Tag Extended Status message */
    BLE_MSG_UPTIME           = 0x09,  /* Tag Uptime message */
                                      /* Range 0x0A to 0xFD for future use */
    BLE_MSG_COMMAND_ACK      = 0xFE,  /* ACK for received commands */
    BLE_MSG_FIRMWARE_REV     = 0xFF,  /* BLE Tag FW Revision message */
};

/** BLE Tag Beacon Message Container **/
typedef struct bmm_msg_t {
    uint8_t type;
    uint8_t data[BLE_MSG_MAX_DATASIZE];
    /* Note: length = (msg_type + msg_data) */
    uint8_t length;
} bmm_msg_t;

/** BLE Advertising Types (Generic Access Profile) **/
typedef enum bmm_ble_adv_types_t {
    ADV_FLAGS = 0x01,
    ADV_SHORTENED_LOCAL_NAME = 0x08,
    ADV_COMPLETE_LOCAL_NAME = 0x09,
    ADV_SERVICE_DATA = 0x16,
    ADV_TX_POWER_LEVEL = 0x0A,
    ADV_MANU_SPEC_DATA = 0xFF
} bmm_ble_adv_types_t;

//******************************************************************************
// Interface
//******************************************************************************
void ble_manager_run(void);
uint32_t bmm_init(void);
bool bmm_queue_is_empty(void);
bool bmm_queue_is_full(void);
uint32_t bmm_enqueue_msg(bmm_msg_t *msg);

#endif /* BLE_MANAGER_MACHINE_H_ */
