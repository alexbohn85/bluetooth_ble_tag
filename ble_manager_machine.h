/*!*****************************************************************************
 * @file
 * @brief BLE Manager Machine
 *******************************************************************************
 *
 *  This module runs from Tag Main Machine ISR context.
 *
 *  This module is the only interface between Tag Application and BLE. Which
 *  means it should encapsulate BLE concerns. Tag Application should not make
 *  BLE API calls directly.
 *
 *  It manages BLE current state using BLE API calls and interact
 *  with (ble_api.c running on main context).
 *
 *  It provides an interface for Tag Beacon Machine to queue new messages to
 *  advertise.
 *
 *  It provides a configuration interface between Tag Application and BLE Stack.
 *  For example: start and stop BLE Stack with proper callback so Tag Application
 *  can be notified.
 *
 */

#ifndef BLE_MANAGER_MACHINE_H_
#define BLE_MANAGER_MACHINE_H_

#include "stdbool.h"
#include "tag_main_machine.h"

//******************************************************************************
// Defines
//******************************************************************************
#define BLE_MSG_MAX_DATASIZE         (8)  // Maximum number of data bytes in a beacon message

//TODO Beacon Rate is here temporarily for test purposes. BLE_Machine is in charge of packet aggregation and BLE API app level control
#define BEACON_FAST_RATE_SEC         (12)
#define BEACON_SLOW_RATE_SEC         (600)
#define BEACON_FAST_RATE_RELOAD      ((BEACON_FAST_RATE_SEC * 1000) / TMM_DEFAULT_TICK_PERIOD_MS)
#define BEACON_SLOW_RATE_RELOAD      ((BEACON_SLOW_RATE_SEC * 1000) / TMM_DEFAULT_TICK_PERIOD_MS)

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

enum bmm_msg_type_enum {
    BLE_MSG_SIMPLE_BEACON = 1,
    BLE_MSG_MOTION = 2,
    BLE_MSG_LF = 3,
    BLE_MSG_LF_COMMAND_DELAYED_EXEC = 4,
    BLE_MSG_COMMAND_ACK = 5,
    BLE_MSG_TEMPERATURE = 6,
    BLE_MSG_BUTTON_PRESS = 7,
    BLE_MSG_TAG_STATUS = 8,
    BLE_MSG_TAG_TAMPER = 9,
    BLE_MSG_BATTERY = 10
};

typedef struct bmm_msg_t {
    uint8_t msg_type;
    uint8_t msg_data[BLE_MSG_MAX_DATASIZE];
    /* Note: length = (msg_type + msg_data) */
    uint8_t length;
} bmm_msg_t;


typedef enum bmm_ble_adv_types_t {
    ADV_TYPE_FLAGS = 0x01,
    ADV_TYPE_COMPLETE_LOCAL_NAME = 0x09,
    ADV_TYPE_UUID_16_SERVICE_DATA = 0x16,
    ADV_TYPE_MANUF_SPEC_DATA = 0xFF,
    ADV_TYPE_TX_POWER_LEVEL = 0x0A
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
