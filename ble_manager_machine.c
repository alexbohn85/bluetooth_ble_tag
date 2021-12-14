/*
 * ble_manager_machine.c
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
 *  For example: start and stop BLE Stack with proper callbacks so Tag Application
 *  can be notified.
 *
 *  To be continued...
 *
 */

#include "em_common.h"
#include "em_cmu.h"
#include "stdio.h"
#include "stdbool.h"
#include "sl_bluetooth.h"
#include "sl_device_init_hfxo.h"
#include "sl_slist.h"
#include "app_assert.h"
#include "sl_status.h"
#include "ble_api.h"
#include "dbg_utils.h"
#include "ble_manager_machine.h"
#include "tag_main_machine.h"
#include "temperature_machine.h"


//******************************************************************************
// Defines
//******************************************************************************


//! FOR TESTS (START) ----------------------------------------------------------
#define UUID_GUARD_L                 0x16
#define UUID_GUARD_H                 0x0B
#define UUID_TEST_L                  0xFF
#define UUID_TEST_H                  0xFF

#define ADV_HEADER_3B                                                                                        \
        2,                          /* length: */                                                            \
        1,                          /* AD type: Flags  */                                                    \
        0x06                        /* Flags */                                                              \

#define ADV_COMPLETE_NAME_5B                                                                                 \
        4,                         /* length: */                                                             \
        9,                         /* AD type: Complete Local Name (0x09)  */                                \
        'U',                                                                                                 \
        'T',                                                                                                 \
        '3'                        /* value: "UT3"  */                                                       \

#define DUMMY_ADV_PACKET_31B {                                                                               \
        ADV_HEADER_3B,                                                                                       \
        ADV_COMPLETE_NAME_5B,                                                                                \
        22,                        /* length: AD_type + Manuf_ID + Tag Beacon Message Data */                \
        0xFF,                      /* AD type: 0xFF (Manufacture Specific Data) */                           \
        UUID_GUARD_L,              /* Manufacturer ID (LSB byte) */                                          \
        UUID_GUARD_H,              /* Manufacturer ID (MSB byte) */                                          \
        0x04,0x03,0x02,0x01,        /* Tag Beacon Message Data <type + data> */                              \
        0x04,0x03,0x02,0x01,        /* Tag Beacon Message Data <type + data> */                              \
        0x04,0x03,0x02,0x01,        /* Tag Beacon Message Data <type + data> */                              \
        0x04,0x03,0x02,0x01,        /* Tag Beacon Message Data <type + data> */                              \
        0x04,0x03,0x02             /* Tag Beacon Message Data <type + data> */                               \
        }

#define DUMMY_ADV_PACKET_12B {                                                                               \
        ADV_HEADER_3B,                                                                                       \
        ADV_COMPLETE_NAME_5B,                                                                                \
        3,                         /* length: AD_type + Manuf_ID + Tag Beacon Message Data */                \
        0xFF,                      /* AD type: 0xFF (Manufacture Specific Data) */                           \
        UUID_GUARD_L,              /* Manufacturer ID (LSB byte) */                                          \
        UUID_GUARD_H               /* Manufacturer ID (MSB byte) */                                          \
        }
//! FOR TESTS (END) ------------------------------------------------------------


//******************************************************************************
// Data types
//******************************************************************************


//******************************************************************************
// Global variables
//******************************************************************************
static sl_sleeptimer_timer_handle_t bmm_timer_1;
static volatile tag_sw_timer_t bmm_debug_timer;
static volatile bmm_fsm_t bmm_fsm;
volatile bool bmm_adv_running;
volatile bool bmm_stack_running;
static bool bmm_running;

//******************************************************************************
// Static functions
//******************************************************************************

static void ble_tx_dummy_msg(uint8_t num_events, uint32_t interval_ms)
{
    sl_status_t sc;

    // Set 3 ADV Transmissions with 100 mS interval.
    sc = sl_bt_advertiser_set_timing( advertising_set_handle,
                                      interval_ms*1.6,         // min. adv. interval (milliseconds * 1.6)
                                      interval_ms*1.6,         // max. adv. interval (milliseconds * 1.6)
                                      0,                       // adv. duration
                                      num_events);             // max. num. adv. events
    app_assert_status(sc);


    uint8_t dummy_msg[] = DUMMY_ADV_PACKET_12B;

    sc = sl_bt_advertiser_set_data(0, 0, sizeof(dummy_msg), dummy_msg);
    app_assert_status(sc);

    sl_bt_advertiser_set_channel_map(advertising_set_handle, 7);

    // Start BLE advertising
    sc = sl_bt_advertiser_start( advertising_set_handle,
                                 advertiser_user_data,
                                 advertiser_non_connectable);

    DEBUG_LOG(DBG_CAT_BLE,"Start BLE beacon advertising...");

    app_assert_status(sc);
    bmm_adv_running = true;
}


static void bmm_timer_reload(uint32_t value)
{
    bmm_debug_timer.value = value;
}

static void bmm_timer_tick(void)
{
    if (bmm_debug_timer.value > 0) {
        --bmm_debug_timer.value;
    }
}


//******************************************************************************
// Non Static functions
//******************************************************************************
uint32_t bmm_queue_msg(uint32_t *msg)
{
    //! Implement add to queue
    return 0;
}

static uint8_t bmm_get_queue_lenght(void)
{
    //! Return the current number of messages in the queue
    return 0;
}

static void bmm_dequeue_priority_msg(void)
{
    //! Get the highest priority message from the queue.
}



//TODO implement some better FSM state transition handling
static void bmm_start_adv(void)
{
    if (!bmm_stack_running) {
        bmm_fsm.state = BMM_START_BLE;
        bmm_fsm.return_state = BMM_START_ADV;
    } else {
        //Handle msg -> packets
        bmm_fsm.state = BMM_START_ADV;
    }
}

static void bmm_process_msg_events(void)
{
    if (bmm_get_queue_lenght() != 0) {
        //Handle msg -> packets

        // Process FSM state transition
        bmm_fsm.state = BMM_START_ADV;

    }
}

static void bmm_start_ble_stack(void)
{
    // Init HFXO
    // Select SYSCLK = HFXO
    // Start BLE on demand
    sl_bt_system_start_bluetooth();

    // Wait for sl_bt_evt_system_boot. Probably pooling here...

    if (bmm_fsm.return_state != BMM_EXIT) {
        bmm_fsm.state = bmm_fsm.return_state;
    }
}

static void bmm_process_stop(void)
{
    bmm_running = false;
    bmm_fsm.state = BMM_START;
}

static void bmm_process_start(void)
{
    bmm_running = true;
}

//! Work in progres...
//!
//! Make sure all possible transitions are not blocking Tag Main Machine ISR
//! for too long.
//!
//! Since we will probably run BLE API calls in this machine the first state
//! could be to enable HFXO restore and let the next tick come where HFXO is restored
//! or we may need to switch from FSRCO to HFXO during this ISR (so far I have been
//! getting unexpected behavior, using "app_sleep_on_isr_exit" to change HFXO restore
//! seems to work better. But it might be just a matter of debugging a bit more and
//! make this clock switch smoothly without the need to use Power Manager.
//!
static void bmm_process_step(void)
{
    switch (bmm_fsm.state) {
        case BMM_START:
            bmm_process_start(); // this will lock this machine into a while loop
            // make decision on what to do next based on BMM main flags (signals)
            // another state to go thru different types of flags may be required
            // also, we may want to handle each flag per tick to avoid blocking for too long
            // it all depends on the final implementation and measuring performance

            //For example FLAG = "SEND MSG" so -> bmm_fsm.state = BMM_PROCESS_MSG_EVENTS;

            break;

        case BMM_PROCESS_CFG_REQ:
            //set mac?
            //change phy config?
            //change ad channels, timing, burst, etc.?
            //change whatever api needs to be exposed to other machines?
            //or just dont change anything because this is likely not necessary and I am just writing because it is 3am and I cant fall sleep...
            break;

        case BMM_PROCESS_MSG_EVENTS:
            bmm_process_msg_events();
            break;

        case BMM_START_BLE:
            bmm_start_ble_stack();
            break;

        case BMM_STOP_BLE:
            break;

        case BMM_START_ADV:
            bmm_start_adv();
            break;

        case BMM_EXIT:
            bmm_process_stop(); // this will unlock this machine from the while loop
            break;

        default:
            break;
    }
}


static void bmm_tx_run(sl_sleeptimer_timer_handle_t *handle, void *data)
{
    //! This ISR is running on HFXO, so we can call BLE API.

    //! TODO Remove BLE stuff from main context, wrap it in a periodic timer that can started by BLE Manager, and add BLE on demand with proper start/stop mechanism.

    (void)(data);
    (void)(handle);


    //! Start a dummy BLE Advertisement (Tests)
    ble_tx_dummy_msg(3, 40);

    //! While BLE is being used we change the global setting to avoid going to sleep after an ISR.
    //! This means, after the ISR we go to main context

#if 1
    if (bmm_adv_running) {
        tag_sleep_on_isr_exit(false);
    } else {
        tag_sleep_on_isr_exit(true);
    }
#endif

}

//! @brief BLE Manager
void ble_manager_run(void)
{
    //! We dont have Tag Beacon Machine fully implemented yet so just simulate
    //! a periodic BLE event here to run power consumption eval.
    bmm_timer_tick();

    if (bmm_debug_timer.value == 0) {

        //! Schedule an API call (we need HFXO restored so use option_flags = 0)
        //! This is a temporary workaround to force HFXO restore on the wake-up.
        sl_sleeptimer_start_timer_ms( &bmm_timer_1,
                                      1, //TODO this delay is just for debug on energy tracer, we can set to a smaller value.
                                      bmm_tx_run,
                                      (void*)NULL,
                                      0, //-> priority 0 is highest
                                      0); //-> 0 means it will restore HFXO

        //! Reload timer (12 seconds)
        bmm_timer_reload(BEACON_FAST_RATE_RELOAD);

#if 0
        //! just concept ideas here

        //! Implement BLE Manager State Machine here
        //! NOTES:
        //! Check if queue is empty, if not, start BLE Stack. Else Stop BLE Stack.
        //! Dequeue BLE messages
        //! And process by priority
        //! Assemble packets in accord to the selected Beacon Format and perform packet aggregation (if enabled).
        //! Send BLE advertisement

        //! reset bmm process state machine
        bmm_fsm.state = BMM_START;
        bmm_fsm.return_state = BMM_START;

        //! Run this machine until its completion (bmm_running = false);
        do {
            bmm_process_step();
        } while (bmm_running);


#endif


    }
}

//! @brief BLE Manager Init
uint32_t bmm_init(void)
{
    bmm_timer_reload(12000/TMM_DEFAULT_TICK_PERIOD_MS);
    bmm_stack_running = false;
    bmm_adv_running = false;
    bmm_fsm.state = BMM_START;
    return 0;
}


