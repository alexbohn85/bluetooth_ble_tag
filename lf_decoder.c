/*
 *  lf_decoder.c
 *
 *    LF Field Decoder:
 *
 *    Decodes DATA signal coming LF Receiver (AS393x). LF receiver is a simple
 *    demodulator. The baseband signal is encoded using differential
 *    manchester at 4Kbps.
 *
 *    This algorithm will detect and decode Preamble, Start bit and Data + CRC
 *    It will extract DATA and perform CRC calculation to detect errors
 *    This module is also responsible for mitigating LF Noise scenarios by
 *    using some event counters and thresholds.
 *
 *    Nominal timing parameters from transmitter (tolerances will be applied)
 *   |     PREAMBLE     |    GAP    |    START BIT    |     DATA and CRC    |
 *   |  1.75 to 5.75mS  |  1.25 mS  |     0.25 mS     |   0.25 or 0.50 mS   |
 */

#include "em_cmu.h"
#include "em_prs.h"
#include "em_ramfunc.h"
#include "em_gpio.h"
#include "sl_sleeptimer.h"
#include "stdbool.h"
#include "string.h"

#include "dbg_utils.h"
#include "as393x.h"
#include "lf_decoder.h"
#include "rtcc.h"
#include "app_properties.h"

//TODO Delete this (only for a test)
#include "boot.h"


//******************************************************************************
// Defines
//******************************************************************************
#define LF_DECODER_DEBUG


#define LF_DATA_PIN                  AS39_LF_DATA_PIN
#define LF_DATA_PORT                 AS39_LF_DATA_PORT

#define PRS_LF_CH                    (0)

#define LF_RTCC_CC0                  (0)            //!  LF decoder uses RTCC Capture/Compare channel 0
#define LF_NUMBER_OF_BITS            (24)           //!  # of bits in the LF dataframe
#define LF_FALSE_WAKEUP_TIMEOUT      (492)          //!  ~15 mS
#define LF_CRC_OK_TIMEOUT            (32768)        //!  ~1000 mS


//#define LF_TOL_TIGHT
#define LF_TOL_RELAXED

#if defined(LF_TOL_TIGHT)
// LF decoder pulse tolerances in ticks @32.768Hz
// Tight
#define LF_PREAMBLE_L                (62)           //!  ~1.900 mS
#define LF_PREAMBLE_H                (197)          //!  ~6.000 mS
#define LF_START_BIT_GAP_MIN         (41)           //!  ~1.250 mS
#define LF_START_BIT_GAP_MAX         (45)           //!  ~1.350 mS
#define LF_ME_BIT0_L                 (7)            //!  ~0.218 mS
#define LF_ME_BIT0_H                 (10)           //!  ~0.280 mS
#define LF_ME_BIT1_L                 (15)           //!  ~0.457 mS
#define LF_ME_BIT1_H                 (18)           //!  ~0.545 mS
#endif

#if defined(LF_TOL_RELAXED)
#define LF_PREAMBLE_L                (49)           //!  ~1.500 mS
#define LF_PREAMBLE_H                (200)          //!  ~6.100 mS
#define LF_START_BIT_GAP_MIN         (41)           //!  ~1.250 mS
#define LF_START_BIT_GAP_MAX         (45)           //!  ~1.350 mS
#define LF_ME_BIT0_L                 (5)            //!  ~0.195 mS
#define LF_ME_BIT0_H                 (11)           //!  ~0.290 mS
#define LF_ME_BIT1_L                 (13)           //!  ~0.440 mS
#define LF_ME_BIT1_H                 (22)           //!  ~0.590 mS
#endif


//******************************************************************************
// Data types
//******************************************************************************
typedef enum lf_decoder_rtcc_modes_t {
    LF_RTCC_CAPTURE,//!< LF_RTCC_CAPTURE
    LF_RTCC_COMPARE //!< LF_RTCC_COMPARE
} lf_decoder_rtcc_modes_t;

typedef enum lf_decoder_states_t {
    PREAMBLE = 0,
    START_BIT_GAP,
    START_BIT,
    DATA,
    CRC,
    SKIP_NEXT_PULSE
} lf_decoder_states_t;

typedef struct lf_decoder_t {
    lf_decoder_states_t state;
    uint32_t curr_edge;
    uint32_t prev_edge;
    uint32_t buffer;
    uint8_t bit_counter;
    uint8_t crc;
    uint16_t errors;
} lf_decoder_t;


//******************************************************************************
// Global variables
//******************************************************************************
static volatile lf_decoder_data_t lf_data;
static lf_decoder_t decoder;

//******************************************************************************
// Static functions
//******************************************************************************

//******************************************************************************
// Non-Static functions
//******************************************************************************

//!-----------------------------------------------------------------------------
//!-----------------------------------------------------------------------------
//!-----------------------------------------------------------------------------
bool lf_decoder_is_data_available(void)
{
    return lf_data.is_available;
}

void lf_decoder_get_lf_data(lf_decoder_data_t *dest)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    dest->is_available = lf_data.is_available;
    dest->command = lf_data.command;
    dest->id = lf_data.id;

    CORE_EXIT_CRITICAL();
}

void lf_decoder_set_lf_data(void)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    lf_data.is_available = true;
    lf_data.id = (uint16_t)((decoder.buffer >> 13) & 0x7FF);
    lf_data.command = (uint8_t)((decoder.buffer >> 7) & 0x3F);

    CORE_EXIT_CRITICAL();
}

void lf_decoder_clear_lf_data(void)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    lf_data.is_available = false;
    lf_data.id = 0;
    lf_data.command = 0;

    CORE_EXIT_CRITICAL();
}

//!-----------------------------------------------------------------------------
//!-----------------------------------------------------------------------------
//!-----------------------------------------------------------------------------

void lf_decoder_rtcc_cc0_config(lf_decoder_rtcc_modes_t mode, uint32_t timeout)
{
    if (mode == LF_RTCC_CAPTURE) {
        RTCC_CCChConf_TypeDef cc0_cfg = RTCC_CH_INIT_CAPTURE_DEFAULT;
        cc0_cfg.inputEdgeSel = rtccInEdgeBoth;
        cc0_cfg.prsSel = PRS_LF_CH;
        RTCC_ChannelInit(LF_RTCC_CC0, &cc0_cfg);

    } else if (mode == LF_RTCC_COMPARE) {
        RTCC_CCChConf_TypeDef cc0_cfg = RTCC_CH_INIT_COMPARE_DEFAULT;
        RTCC_ChannelInit(LF_RTCC_CC0, &cc0_cfg);

        //! Setup a timeout on RTCC CC0 channel
        uint32_t timer_offset = timeout + RTCC_CounterGet();
        RTCC_ChannelCompareValueSet(LF_RTCC_CC0, timer_offset);
    }
}


void lf_decoder_crc(uint8_t bit)
{
    if ((decoder.crc ^ (bit << 1)) & 0x02) {
        decoder.crc = (((decoder.crc ^ 0x20) >> 1) | 0x80);
    } else {
        decoder.crc = decoder.crc >> 1;
    }
}

void lf_decoder_compare_start(uint32_t timeout)
{
    lf_decoder_rtcc_cc0_config(LF_RTCC_COMPARE, timeout);
    RTCC_IntClear(RTCC_IEN_CC0);
    NVIC_ClearPendingIRQ(RTCC_IRQn);
    //RTCC_IntEnable(RTCC_IEN_CC0);
    //NVIC_EnableIRQ(RTCC_IRQn);
}

void lf_decoder_capture_start(void)
{
    lf_decoder_rtcc_cc0_config(LF_RTCC_CAPTURE, 0);
    //NVIC_ClearPendingIRQ(RTCC_IRQn);
    //RTCC_IntEnable(RTCC_IEN_CC0);
    //RTCC_IntClear(RTCC_IEN_CC0);
    //NVIC_EnableIRQ(RTCC_IRQn);
}

void lf_decoder_reset_and_backoff(uint32_t timeout)
{
    as39_antenna_enable(false, false, false);
    lf_decoder_compare_start(timeout);
}

#if 0
void lf_decoder_capture_stop(void)
{
    RTCC_IntDisable(RTCC_IEN_CC0);
    RTCC_IntClear(RTCC_IEN_CC0);
    NVIC_ClearPendingIRQ(RTCC_IRQn);
    NVIC_DisableIRQ(RTCC_IRQn);
    //RTCC_Stop();
}
#endif

void lf_decoder_compare_isr(void)
{
    decoder.state = PREAMBLE;
    lf_decoder_capture_start();
    decoder.prev_edge = RTCC_ChannelCaptureValueGet(LF_RTCC_CC0);
    as39_antenna_enable(true, false, false);
    decoder.errors = 0;
}

void lf_decoder_capture_isr(void)
{

    decoder.curr_edge = RTCC_ChannelCaptureValueGet(LF_RTCC_CC0);

    volatile int pulse_width = (decoder.curr_edge - decoder.prev_edge);

    //TODO Implement glitch filter approach to improve decoding in low SNR.
    //volatile uint32_t stored_prev_edge = decoder.prev_edge;

    decoder.prev_edge = decoder.curr_edge;

    //! Handle negative values
    if (pulse_width < 0) {
        pulse_width = (~pulse_width + 1);
    }

    //! TODO Testing using error counter in the PREAMBLE and start bit to try to distinguish noise from weak field.
    //! TODO Compare performance not using on/off and making PREAMBLE tolerance nominal.
    if (decoder.errors > 500) {
        decoder.errors = 0;
        lf_decoder_reset_and_backoff(LF_FALSE_WAKEUP_TIMEOUT * 10);
    }

    switch(decoder.state) {

        //! bit "0" was detected on previous pulse, ignore this transition and go back to DATA state.
        case SKIP_NEXT_PULSE:
            decoder.state = DATA;
            break;

        case PREAMBLE:
            if ((pulse_width > LF_PREAMBLE_L) && (pulse_width < LF_PREAMBLE_H)) {
                decoder.state = START_BIT_GAP;
                decoder.buffer = 0;
                decoder.bit_counter = LF_NUMBER_OF_BITS;
                decoder.crc = 0;
            } else {
                decoder.state = PREAMBLE;
                //! Increment error counter (testing counter approach)
                decoder.errors++;
            }
            break;

        case START_BIT_GAP:
            if ((pulse_width > LF_START_BIT_GAP_MIN) && (pulse_width < LF_START_BIT_GAP_MAX)) {
                decoder.state = START_BIT;
            } else {
                decoder.state = PREAMBLE;
                //! Increment error counter (testing counter approach)
                decoder.errors++;
            }
            break;

        case START_BIT:
            //TODO Check transmitter asymmetry effect during this pulse, PREAMBLE + START_GAP are the critical signals.
            //! ignore start bit pulse
            decoder.state = DATA;
            break;

            //! Decode LF DATA Stream
        case DATA:
            if ((pulse_width > LF_ME_BIT0_L) && (pulse_width < LF_ME_BIT0_H)) {
                decoder.buffer = decoder.buffer  << 1;
                decoder.state = SKIP_NEXT_PULSE;
            } else if ((pulse_width > LF_ME_BIT1_L) && (pulse_width < LF_ME_BIT1_H)) {
                decoder.buffer  = ((decoder.buffer  << 1) | 1);
            } else {
                //decoder.prev_edge = stored_prev_edge;
                //! Increment error counter (testing counter approach)
                decoder.errors++;
                decoder.state = PREAMBLE;
                break;
            }

            decoder.bit_counter--;

            //! On the fly CRC
            if (decoder.bit_counter > 6) {
                //! Send current bit received to CRC on the fly calculation
                lf_decoder_crc(decoder.buffer & 0x01);
                break;
            }

            //! All bits received, check CRC.
            if (decoder.bit_counter == 0) {
                uint8_t rx_crc = (decoder.buffer & 0x7F);
                if (((decoder.crc >> 1) ^ rx_crc) == 0) {
                    //! CRC OK!
                    lf_decoder_set_lf_data();
                    lf_decoder_reset_and_backoff(LF_CRC_OK_TIMEOUT);
                    decoder.errors = 0;
                }
                decoder.state = PREAMBLE;
            }
            break;

        default:
            decoder.state = PREAMBLE;
            break;
    }
}

void lf_decoder_prs_init(void)
{
    CMU_ClockEnable(cmuClock_PRS, true);
    PRS_ConnectSignal(PRS_LF_CH, prsTypeAsync, prsSignalGPIO_PIN0);
    GPIO_IntConfig(LF_DATA_PORT, LF_DATA_PIN, false, false, false);
    PRS_ConnectConsumer(PRS_LF_CH, prsTypeAsync, prsConsumerRTCC_CC0);
}

#if 0  // moved to rtcc.c
void lf_decoder_rtcc_init(void)
{
    //! RTCC Clock Enable (Assuming LFXO was initialized during sl_system_init)
    CMU_ClockSelectSet(cmuClock_RTCCCLK, cmuSelect_LFXO);
    CMU_ClockEnable(cmuClock_RTCC, true);

    //! RTCC Configuration
    RTCC_Init_TypeDef rtcc_cfg = RTCC_INIT_DEFAULT;
    rtcc_cfg.debugRun = false;
    rtcc_cfg.presc = rtccCntPresc_1;
    rtcc_cfg.prescMode = rtccCntTickPresc;
    RTCC_Init(&rtcc_cfg);
}
#endif

void lf_decoder_init(void)
{
    //! Here we initialize everything required to start the LF Decoder
    memset(&decoder, 0, sizeof(decoder));
    decoder.state = PREAMBLE;

    //! Check if AS393x device driver is initialized
    if(as39_get_handle(NULL) == AS39_DRIVER_NOT_INITIATED) {
        DEBUG_LOG(DBG_CAT_WARNING, "ERROR! AS393x device driver was not initiated...");
        DEBUG_TRAP();
    } else {
        //! Used for debug
        //GPIO_PinModeSet(BOOTSEL_B2_PORT, BOOTSEL_B2_PIN, gpioModePushPull, 0);

        //! We are connecting the LF DATA pin to the RTCC Capture using PRS
        lf_decoder_prs_init();
        lf_decoder_capture_start();

        //! Enable RTCC CC1 Interrupt
        RTCC_IntClear(RTCC_IEN_CC0);
        RTCC_IntEnable(RTCC_IEN_CC0);

#if (TAG_TYPE == TAG_UT3)
        as39_antenna_enable(true, false, false);
#endif
        DEBUG_LOG(DBG_CAT_SYSTEM, "LF Decoder started...");
    }

}


