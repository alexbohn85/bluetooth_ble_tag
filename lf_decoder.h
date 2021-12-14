/*
 *  lf_decoder.h
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
 *   |  1.75 to 5.75 mS |  1.25 mS  |     0.25 mS     |   0.25 or 0.50 mS   |
 *
 *
 */

#ifndef LF_DECODER_H_
#define LF_DECODER_H_

#include "as393x.h"
#include "stdbool.h"

//******************************************************************************
// Defines
//******************************************************************************


//******************************************************************************
// Data types
//******************************************************************************
typedef struct lf_decoder_data_t {
    bool is_available;
    uint16_t id;
    uint8_t command;
} lf_decoder_data_t;


//******************************************************************************
// Global variables
//******************************************************************************


//******************************************************************************
// Interface
//******************************************************************************
void lf_decoder_clear_lf_data(void);
void lf_decoder_get_lf_data(lf_decoder_data_t *dest);
bool lf_decoder_is_data_available(void);
void lf_decoder_compare_isr(void);
void lf_decoder_capture_isr(void);
void lf_decoder_capture_stop(void);
void lf_decoder_capture_start(void);
void lf_decoder_init(void);

#endif /* LF_DECODER_H_ */
