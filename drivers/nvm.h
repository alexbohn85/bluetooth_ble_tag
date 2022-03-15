/*
 * nvm.h
 * Interface with NVM3 Driver - This is helper module to interface between
 * NVM Machine and NVM3 Driver to store persistent tag data.
 *
 * Note: Flash operations duration are not predictable so it should not be started
 * from within the Tag Main Machine ISR context to avoid blocking it for too long.
 *
 * All NVM operations are scheduled from within NVM Machine using a sleeptimer
 * callback to this helper.
 *
 */

#ifndef DRIVERS_NVM_H_
#define DRIVERS_NVM_H_

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
void nvm_write_mac_address(uint8_t *data, uint8_t length);
void nvm_init(void);
void nvm_example(void);
//void nvm_example(uint8_t *data1, size_t size1, uint8_t *data2, size_t size2);
#endif /* DRIVERS_NVM_H_ */
