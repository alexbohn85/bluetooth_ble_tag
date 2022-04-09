/*!*****************************************************************************
 * @file
 * @brief Boot
 *******************************************************************************
 *
 * This module provide means to select tag operation mode @ref boot_mode based
 * on GPIO reads during reset.
 *
 * @note SERMAC uses custom JLINK scripts to control RESET and UART lines
 *       This allows SERMAC to execute the BOOT SELECT sequencing on BLE Tag.
 *       Below is the documentation for J-link custom scripts:
 *       https://wiki.segger.com/J-Link_script_files
 */

#ifndef BOOT_H_
#define BOOT_H_

#define BOOT_MANUFACTURING_TEST      (0b01)
#define BOOT_UNUSED                  (0b00)
#define BOOT_ESCAPE_HATCH            (0b10)
#define BOOT_NORMAL                  (0b11)

uint8_t boot_get_mode(void);
void boot_init(void);
uint32_t boot_get_reset_cause(void);
void boot_print_reset_cause(void);

#endif /* BOOT_H_ */
