/*
 * nvm.h
 */

#ifndef DRIVERS_NVM_H_
#define DRIVERS_NVM_H_

#include "tag_beacon_machine.h"

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
uint32_t nvm_read_mac_address(uint8_t *mac);
uint32_t nvm_write_mac_address(uint8_t *mac);

/**
 * @brief Read/Write functions for Tag Beacon Machine settings (beacon rates)
 * @param tbm_nvm_data_t
 * @return uint32_t
 */
uint32_t nvm_read_tbm_settings(tbm_nvm_data_t *b);
uint32_t nvm_write_tbm_settings(tbm_nvm_data_t *b);

#endif /* DRIVERS_NVM_H_ */
