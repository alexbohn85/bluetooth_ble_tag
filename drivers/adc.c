/*
 * adc.c
 *
 */

#ifndef DRIVERS_ADC_C_
#define DRIVERS_ADC_C_

#include "em_common.h"
#include "em_cmu.h"
#include "dbg_utils.h"
#include "adc.h"


#if 0
void ADC0_IRQHandler(void)
{
    uint32_t flags;
    pir_sample_t adc_sample;
    static int32_t last_timestamp = -1;

    flags = ADC_IntGetEnabled(ADC0);
    ADC_IntClear(ADC0, flags);
    NVIC_ClearPendingIRQ(ADC0_IRQn);
}
#endif


void adc_init(void)
{
    // Initialize the ADC.
    CMU_ClockEnable(cmuClock_IADC0, true);

    IADC_Init_t iadc_init = IADC_INIT_DEFAULT;
    IADC_AllConfigs_t iadc_all_configs = IADC_ALLCONFIGS_DEFAULT;
    IADC_init(IADC0, &iadc_init, &iadc_all_configs);

    IADC_InitSingle_t iadc_init_single_param = IADC_INITSINGLE_DEFAULT;
    iadc_init_single_param.alignment = iadcAlignRight20;
    IADC_SingleInput_t iadc_single_input_param = IADC_SINGLEINPUT_DEFAULT;
    iadc_single_input_param.compare = false;
    iadc_single_input_param.negInput = iadcNegInputGnd;
    iadc_single_input_param.posInput = iadcPosInputAvdd;
    iadc_single_input_param.configId = 0;
    IADC_initSingle(IADC0, &iadc_init_single_param, &iadc_single_input_param);

    IADC_command(IADC0, iadcCmdStartSingle);

    IADC_Result_t result;
    result.data = 0;
    DEBUG_LOG(DBG_CAT_GENERIC, "ADC result = %lX", result.data);

    result = IADC_readSingleResult(IADC0);
    DEBUG_LOG(DBG_CAT_GENERIC, "ADC result = %lX", result.data);

}





#endif /* DRIVERS_ADC_C_ */
