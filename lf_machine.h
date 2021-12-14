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


//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Extern global variables
//******************************************************************************

//******************************************************************************
// Data types
//******************************************************************************

//******************************************************************************
// Interface
//******************************************************************************
void lf_run(void);
uint32_t lfm_init(void);

#endif /* LF_MACHINE_H_ */
