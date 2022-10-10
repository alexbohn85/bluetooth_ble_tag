/*
 * rtcc.c
 *
 */

#include "em_core.h"

#include "lf_decoder.h"
#include "tag_main_machine.h"
#include "rtcc.h"


//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Global variables
//******************************************************************************

//******************************************************************************
// Static functions
//******************************************************************************

//******************************************************************************
// Non Static functions
//******************************************************************************
void RTCC_IRQHandler(void)
{
    CORE_DECLARE_IRQ_STATE;
    uint32_t irq_flag;

    CORE_ENTER_ATOMIC();
    irq_flag = RTCC_IntGet();

    // Clear RTCC flags
    RTCC_IntClear(irq_flag & (RTCC_IF_CC0 | RTCC_IF_CC1 | RTCC_IF_CC2));
    NVIC_ClearPendingIRQ(RTCC_IRQn);

#if 1
    // Capture/Compare 0 handler - LF Decoder
    if (irq_flag & RTCC_IF_CC0) {
        if ((RTCC->CC[0].CTRL & _RTCC_CC_CTRL_MODE_MASK) == RTCC_CC_CTRL_MODE_INPUTCAPTURE) {
            lf_decoder_capture_isr();
        } else if ((RTCC->CC[0].CTRL & _RTCC_CC_CTRL_MODE_MASK) == RTCC_CC_CTRL_MODE_OUTPUTCOMPARE) {
            lf_decoder_compare_isr();
        }
    }
#endif

#if 1
    // Capture/Compare 1 handler - Tag Main Machine SysTick
    if (irq_flag & RTCC_IF_CC1) {
        tag_main_machine_isr();
    }
#endif

#if 0
    // Capture/Compare 2 handler
    if (irq_flag &RTCC_IF_CC2) {// unused... }
#endif

    CORE_EXIT_ATOMIC();
}


// Initialize RTCC Timer
void rtcc_init(void)
{

    // First we need to clock the peripheral otherwise elon musk is gonna cry!
    // RTCC Clock Enable (Assuming LFXO was initialized during sl_system_init)
    CMU_ClockEnable(cmuClock_RTCC, true);
    CMU_ClockSelectSet(cmuClock_RTCCCLK, cmuSelect_LFXO);


    // RTCC Configuration
    RTCC_Init_TypeDef rtcc_cfg = RTCC_INIT_DEFAULT;
    rtcc_cfg.debugRun = false;
    rtcc_cfg.presc = rtccCntPresc_1;
    rtcc_cfg.prescMode = rtccCntTickPresc;
    RTCC_Init(&rtcc_cfg);

    RTCC_IntClear(RTCC_IF_CC0 | RTCC_IF_CC1 | RTCC_IF_CC2);
    RTCC_IntDisable(RTCC_IF_CC0 | RTCC_IF_CC1 | RTCC_IF_CC2);
    NVIC_ClearPendingIRQ(RTCC_IRQn);
    NVIC_EnableIRQ(RTCC_IRQn);
}
