/*
 * lf_machine.h
 *
 *  This module runs from Tag Main Machine ISR context.
 *
 *  This module track LF Decoder decoded data and provide logic for Entering, Staying
 *  and Exiting LF Field feature and report to Tag Beacon Machine.
 *
 */

#ifndef LF_MACHINE_H_
#define LF_MACHINE_H_

#include "lf_decoder.h"

//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Extern global variables
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************
typedef enum lfm_lf_events_t {
    ENTERING_FIELD,
    EXITING_FIELD,
    STAYING_FIELD,
    WTA_DELAYED_CMD_EXEC
}lfm_lf_events_t;

typedef struct lfm_lf_beacon_t {
    lfm_lf_events_t lf_event;
    lf_decoder_data_t lf_data;
} lfm_lf_beacon_t;

//******************************************************************************
// Interface
//******************************************************************************
void lf_run(void);
uint32_t lfm_init(void);

#endif /* LF_MACHINE_H_ */
