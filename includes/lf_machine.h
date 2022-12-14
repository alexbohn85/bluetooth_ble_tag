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
#define LFM_WTA_DELAYED_CMD_EXEC_FLAG          (0x01)                           // This flag indicate we received a WTA command and we are confirming it.
#define LFM_WTA_BACKOFF_FLAG                   (0x02)                           // After receiving and confirming WTA command we want to avoid receiving another command right away.
#define LFM_STAYING_IN_FIELD_FLAG              (0x04)
#define LFM_ENTERING_FIELD_FLAG                (0x08)

//******************************************************************************
// Extern global variables
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************
typedef enum lfm_lf_events_t {
    ENTERING_FIELD = 0x01,
    EXITING_FIELD = 0x02,
    STAYING_FIELD = 0x03,
    EXCITER_BATT_LOW = 0x04
} lfm_lf_events_t;

typedef struct lfm_lf_beacon_t {
    union {
        struct {
            uint8_t lf_id_upper_bits : 3;
            uint8_t lf_message_type : 3;
            uint8_t lf_exciter_type: 2;
            uint8_t lf_id_lower_bits : 8;
        };
        uint8_t lf_data[2];
    };
} lfm_lf_beacon_t;

//******************************************************************************
// Interface
//******************************************************************************
lfm_lf_beacon_t* lfm_get_beacon_data(void);
uint8_t lfm_get_lf_status(void);
void lf_run(void);
uint32_t lfm_init(void);

#endif /* LF_MACHINE_H_ */
