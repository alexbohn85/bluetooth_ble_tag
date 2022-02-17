/*
 * cli.h
 *
 * This module provide UART a command line interface functionality.
 *
 */

#ifndef CLI_H_
#define CLI_H_

#include "dbg_utils.h"

void cli_start(void);
void cli_stop(void);

//! Deprecated
void cli_init(void);
void cli_deinit(void);

#endif /* CLI_H_ */
