/*
 * ble_manager_machine.h
 *
 */

#ifndef BLE_MANAGER_MACHINE_H_
#define BLE_MANAGER_MACHINE_H_

#include "stdbool.h"
#include "tag_main_machine.h"

//******************************************************************************
// Defines
//******************************************************************************
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
    BMM_STOP_BLE,
    BMM_START_BLE,
    BMM_PROCESS_MSG_EVENTS,
    BMM_START_ADV,
    BMM_START,
    BMM_EXIT
} bmm_states_t;

typedef struct bmm_fsm_t {
    bmm_states_t state;
    bmm_states_t return_state;
} bmm_fsm_t;

typedef enum bmm_msg_type_t {
    BLE_MSG_FW_VERSION,
    BLE_MSG_MOTION,
    BLE_MSG_LF,
    BLE_MSG_LF_COMMAND_EXEC,
    BLE_MSG_TEMP,
    BLE_MSG_BUTTON_PRESS,
    BLE_MSG_TAG_STATUS,
    BLE_MSG_TAG_BEACON,
    BLE_MSG_TAG_TAMPER
} bmm_msg_type_t;

typedef struct bmm_msg_t {
    bmm_msg_type_t type;
    uint8_t *data;
    uint8_t length;
    uint8_t priority;
} bmm_msg_t;

typedef enum bmm_ble_adv_types_t {
    LOCAL_NAME = 0x09,
    UUID_16_SERVICE_DATA = 0x16,
    MANUF_SPEC_DATA = 0xFF,
    TX_POWER_LEVEL = 0x0A
} bmm_ble_adv_types_t;

//******************************************************************************
// Interface
//******************************************************************************
void ble_manager_run(void);
uint32_t bmm_init(void);

#endif /* BLE_MANAGER_MACHINE_H_ */
