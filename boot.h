/*
 * boot.h
 *
 */

#ifndef BOOT_H_
#define BOOT_H_

#define BOOTSEL_B1_PORT                           gpioPortB
#define BOOTSEL_B1_PIN                            1

#define BOOTSEL_B2_PORT                           gpioPortB
#define BOOTSEL_B2_PIN                            2

#define BOOT_MANUFACTURING_TEST      (0b01)
#define BOOT_UNUSED                  (0b00)
#define BOOT_ESCAPE_HATCH            (0b10)
#define BOOT_NORMAL                  (0b11)

uint8_t boot_read_mode(void);
void boot_init(void);

#endif /* BOOT_H_ */
