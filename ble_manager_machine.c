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

#include "em_common.h"
#include "em_cmu.h"
#include "stdio.h"
#include "stdbool.h"
#include "string.h"
#include "sl_bluetooth.h"
#include "sl_device_init_hfxo.h"
#include "sl_slist.h"
#include "app_assert.h"
#include "sl_status.h"

#include "dbg_utils.h"
#include "tag_sw_timer.h"
#include "tag_defines.h"
#include "ble_api.h"
#include "tag_main_machine.h"
#include "temperature_machine.h"
#include "ble_manager_machine.h"


//******************************************************************************
// Defines
//******************************************************************************
#define BLE_CIRCULAR_BUFFER_SIZE     (10)

#define ADV_PAYLOAD_MAX_LEN          (31)
//#define ADV_RETRANSMISSIONS          (3)
#define ADV_RETRANSMISSIONS          (30)
#define ADV_MIN_INTERVAL             (20)
#define ADV_MAX_INTERVAL             (100)

// Manufacturer Specific Data UUID
#define UUID_GUARD_L                 0x16
#define UUID_GUARD_H                 0x0B
#define UUID_TEST_L                  0xFF
#define UUID_TEST_H                  0xFF

//******************************************************************************
// Data types
//******************************************************************************
typedef struct bmm_circular_queue_t {
    int8_t front;
    int8_t rear;
    bmm_msg_t item[BLE_CIRCULAR_BUFFER_SIZE];
} bmm_circular_queue_t;

typedef struct bmm_adv_pdu_t {
    uint8_t data[ADV_PAYLOAD_MAX_LEN];
    size_t length;
}bmm_adv_payload_t;

//******************************************************************************
// Global variables
//******************************************************************************
static sl_sleeptimer_timer_handle_t bmm_ble_api_sched_timer;
static volatile bmm_fsm_t bmm_fsm;
volatile bool bmm_adv_running;
volatile bool bmm_stack_running;
static bool bmm_running;
static bmm_circular_queue_t queue;
static bmm_adv_payload_t adv_payload;


//******************************************************************************
// Functions
//******************************************************************************
static void ble_api_start_adv(uint8_t retransmissions, uint32_t min_interval, uint32_t max_interval)
{
    sl_status_t sc;

    sc = sl_bt_advertiser_set_timing( advertising_set_handle,
                                      min_interval*1.6,                         // min. adv. interval (milliseconds * 1.6)
                                      max_interval*1.6,                         // max. adv. interval (milliseconds * 1.6)
                                      0,                                        // adv. duration
                                      retransmissions);                         // max. num. adv. events
    app_assert_status(sc);

    sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                     sl_bt_advertiser_advertising_data_packet,
                                     adv_payload.length,
                                     adv_payload.data);
    app_assert_status(sc);

    /* Set all adv channels
    *     - <b>1:</b> Advertise on CH37
    *     - <b>2:</b> Advertise on CH38
    *     - <b>3:</b> Advertise on CH37 and CH38
    *     - <b>4:</b> Advertise on CH39
    *     - <b>5:</b> Advertise on CH37 and CH39
    *     - <b>6:</b> Advertise on CH38 and CH39
    *     - <b>7:</b> Advertise on all channels
    */
    sl_bt_advertiser_set_channel_map(advertising_set_handle, 7);

#if 1
    // Start BLE advertising (advertiser_non_connectable)
    sc = sl_bt_advertiser_start( advertising_set_handle,
                                 advertiser_user_data,
                                 advertiser_non_connectable);

#else
    //TODO to receive commands we need reader to connect.
    //TODO we need custom GATT
    sc = sl_bt_advertiser_start( advertising_set_handle,
                                 advertiser_user_data,
                                 advertiser_connectable_non_scannable);
#endif
    app_assert_status(sc);

    DEBUG_LOG(DBG_CAT_BLE, "Start BLE beacon advertising...");
}

static void bmm_ble_api_start_adv_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
    // This SLEEPTIMER ISR context is running on HFXO, so we can call BLE API.

    (void)(data);
    (void)(handle);

    // Start a BLE Advertiser
    ble_api_start_adv(ADV_RETRANSMISSIONS, ADV_MIN_INTERVAL, ADV_MAX_INTERVAL);

    // While BLE is being used we change the global setting to avoid going to sleep after an ISR.
    // This means, after the ISR we go to main context to let call ble_api_fsm_run() until adv is over.
    // This way HFXO is used.
    bmm_adv_running = true;
    tag_sleep_on_isr_exit(false);

}

//----------------------------------------------------------------------------
// START OF CIRCULAR QUEUE IMPLEMENTATION (FIFO)
//----------------------------------------------------------------------------
void bmm_queue_init(void)
{
    queue.front = -1;
    queue.rear = -1;
    memset(queue.item, 0, (sizeof(queue.item) / sizeof(queue.item[0])));
}

bool bmm_queue_is_empty(void)
{
    return (queue.front == -1);
}

bool bmm_queue_is_full(void)
{
    return (((queue.rear + 1) % BLE_CIRCULAR_BUFFER_SIZE) == queue.front);
}

//! @brief Add a beacon message to the queue.
uint32_t bmm_enqueue_msg(bmm_msg_t *msg)
{
    uint32_t status;

    if( bmm_queue_is_full()) {
        DEBUG_LOG(DBG_CAT_BLE, "Queue is full!");
        status = -1;
    } else {
        if (bmm_queue_is_empty()) {
            queue.front++;
        }
        queue.rear = (queue.rear + 1) % BLE_CIRCULAR_BUFFER_SIZE;
        memcpy(&queue.item[queue.rear], msg, sizeof(bmm_msg_t));
        status = 0;
    }

    return status;
}

//! @brief Peek next beacon message from the queue (no dequeuing)
static uint32_t bmm_peek_queue(bmm_msg_t *msg)
{
    uint32_t status;
    if (bmm_queue_is_empty()) {
        status = 1;
    } else {
        memcpy(msg, &queue.item[queue.front], sizeof(bmm_msg_t));
        status = 0;
    }

    return status;
}

//! @brief Dequeue beacon message from the queue
static uint32_t bmm_dequeue_msg(bmm_msg_t *msg)
{
    uint32_t status;

    if (bmm_queue_is_empty()) {
        status = 1;
    } else {
        memcpy(msg, &queue.item[queue.front], sizeof(bmm_msg_t));
        if (queue.front == queue.rear) {
            queue.front = -1;
            queue.rear = -1;
        } else {
            queue.front = ((queue.front + 1) % BLE_CIRCULAR_BUFFER_SIZE);
        }
        status = 0;
    }

    return status;
}
//----------------------------------------------------------------------------
// END OF QUEUE IMPLEMENTATION
//----------------------------------------------------------------------------

/**
 * @brief Schedule a BLE API call to start BLE Advertiser
 * @note Wrap BLE API call in a SLEEPTIMER context where HFXO will be restored.
 */
static void bmm_start_adv(void)
{
    sl_sleeptimer_start_timer_ms( &bmm_ble_api_sched_timer,
                                  1,
                                  bmm_ble_api_start_adv_callback,
                                  (void*)NULL,
                                  0,
                                  0);
}

static void bmm_append_adv_flags(uint8_t *pdu_len, uint8_t flags)
{

    adv_payload.data[(*pdu_len)++] = 0x02;
    adv_payload.data[(*pdu_len)++] = ADV_TYPE_FLAGS;
    adv_payload.data[(*pdu_len)++] = flags;
}

static void bmm_append_adv_complete_local_name(uint8_t *pdu_len)
{
    uint8_t i;

    // Keep index where name length should be placed.
    uint8_t temp = (*pdu_len);

    // Add length (to be calculated below)
    adv_payload.data[(*pdu_len)++] = 0;

    // Add adv_type
    adv_payload.data[(*pdu_len)++] = ADV_TYPE_COMPLETE_LOCAL_NAME;

    // Add tag_name
    for (i = 0; i < strlen(tag_name); i++) {
        adv_payload.data[(*pdu_len)++] = tag_name[i];
    }

    // Add the length
    adv_payload.data[temp] = i + 1;

}

static void bmm_append_user_data(uint8_t *pdu_len, bmm_msg_t *msg)
{
    uint8_t i;

    if (*pdu_len > ADV_PAYLOAD_MAX_LEN) {
        DEBUG_LOG(DBG_CAT_BLE, "Error ADV PAYLOAD size exceeded...");
    } else {

        // Append message type
        adv_payload.data[(*pdu_len)++] = msg->msg_type;

        // Append message data
        for (i = 0; i < (msg->length - 1); i++) {
            adv_payload.data[(*pdu_len)++] = msg->msg_data[i];
        }
    }
}

static void bmm_process_packet_aggregation(uint8_t *pdu_len, uint8_t *userdata_len)
{
    bool finished = false;
    bmm_msg_t msg;

    do {
        if(bmm_peek_queue(&msg) != 1) {                                         // Peek a message from the queue (does not remove from the queue)
            if (((*pdu_len) + msg.length) < ADV_PAYLOAD_MAX_LEN) {              // If message fits into the ADV_PDU buffer then proceed.
                (*userdata_len) += msg.length;                                  // Update manuf. spec. data length
                bmm_dequeue_msg(&msg);                                          // Grab message from the queue (this time it will remove from the queue)
                bmm_append_user_data(pdu_len, &msg);                            // And append message to ADV_PDU buffer
            } else {                                                            // If message does not fit then leave it in the queue and exit.
                finished = true;                                                // If it does not fit, just exit.
            }
        } else {
            finished = true;                                                    // In case there is no more messages in the queue, just exit.
        }
    } while (!finished);

}
/**
 * @brief Append Manufacturer Spec. Data into the ADV_PDU
 * @details This function will try to fit "aggregate" as many beacon messages as possible in the ADV_PAYLOAD.
 * @param pdu_len variable holding PDU length counter
 */
static void bmm_append_man_spec_data(uint8_t *pdu_len)
{
    // Save current index position, we will use this later when packet aggregation is done.
    uint8_t man_spec_data_len_index = (*pdu_len);
    uint8_t man_spec_data_len = 0;

    // Fill in all Manufacturer Specific Data fields
    adv_payload.data[(*pdu_len)++] = 0;                                         // placeholder for the length once we finish packet assembly.
    adv_payload.data[(*pdu_len)++] = ADV_TYPE_MANUF_SPEC_DATA;                  // ADV type id
    man_spec_data_len++;
    adv_payload.data[(*pdu_len)++] = UUID_GUARD_L;                              // Manufacturer 16-bits UUID (little-endian)
    man_spec_data_len++;
    adv_payload.data[(*pdu_len)++] = UUID_GUARD_H;
    man_spec_data_len++;
    adv_payload.data[(*pdu_len)++] = TAG_TYPE;                                  // BLE Tag Type ID
    man_spec_data_len++;

    // Now go thru the msg queue and aggregate messages until PDU buffer is full.
    bmm_process_packet_aggregation(pdu_len, &man_spec_data_len);

    // Add manufacturer specific data length to the appropriate index in the buffer.
    adv_payload.data[man_spec_data_len_index] = man_spec_data_len;

    // And finally, add total PDU length.
    adv_payload.length = (size_t)(*pdu_len);

}

static uint32_t bmm_process_msg_events(void)
{
    uint32_t status;

    if (!bmm_queue_is_empty()) {
        uint8_t pdu_len = 0;

        // Clear ADV_PDU buffer
        memset(&adv_payload, 0, sizeof(adv_payload));

#if 0
        // Check if this BLE Manager is currently set to advertise as "advertiser_connectable_non_scannable".
        // TODO Need to decide how this policy will be managed by BMM.
        if(connectable) {
            //! If so, then we must add the advertise flags according to BLE specs
            bmm_append_adv_flags(&pdu_len, flag_value);
        }
#endif

        // Append Complete Local Name
        bmm_append_adv_complete_local_name(&pdu_len);

        // Append Manufacturer Specific Data
        bmm_append_man_spec_data(&pdu_len);

        status = 0;

    } else {
        status = 1;
    }

    return status;
}

static void bmm_process_stop(void)
{
    bmm_running = false;
}

static void bmm_process_start(void)
{
    bmm_running = true;
}

/**
 * @brief BLE Manager State Machine Processing
 * @details Process states based on current events and signals (TBD in more details)
 */
static void bmm_process_step(void)
{
    switch (bmm_fsm.state) {
        case BMM_START:
            bmm_process_start();
            // Make decision based on signals on what to do next...
            // Check Beacon Messages Queue is the only thing implemented for now..
            bmm_fsm.state = BMM_PROCESS_MSG_EVENTS;
            break;

        case BMM_PROCESS_CFG_REQ:
            //set mac?
            //change phy config?
            //change ad channels, timing, burst, etc?
            break;

        case BMM_PROCESS_MSG_EVENTS:
            if (!bmm_adv_running) {
                if (bmm_process_msg_events() == 0) {
                    bmm_fsm.state = BMM_START_ADV;
                } else {
                    bmm_fsm.state = BMM_EXIT;
                }
            }
            break;

        case BMM_START_ADV:
            bmm_start_adv();
            bmm_fsm.state = BMM_EXIT;
            break;

        case BMM_EXIT:
            bmm_process_stop();
            break;

        default:
            break;
    }
}

/**
 * @brief BLE Manager Machine
 * @details Implements interfaces between Tag and BLE API. It is also responsible for
 *     advertising packet formatting, assembly and dispatch to BLE Stack for transmissions.
 *     It also provides interface for BLE Stack configuration.
 */
void ble_manager_run(void)
{
    // reset bmm process state machine
    bmm_fsm.state = BMM_START;
    bmm_fsm.return_state = BMM_START;

    // Run this machine until its completion.
    do {
        bmm_process_step();
    } while (bmm_running);
}

//! @brief BLE Manager Init
uint32_t bmm_init(void)
{
    bmm_stack_running = false;
    bmm_adv_running = false;
    bmm_queue_init();
    return 0;
}


