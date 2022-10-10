/*
 * tag_gpios_mapping.h
 *
 */

#ifndef TAG_GPIO_MAPPING_H_
#define TAG_GPIO_MAPPING_H_

#include "tag_defines.h"

#if TAG_ID == MODEL_A_ID

//TODO This pins will later be mapped to UART TX/RX
//#define BOOTSEL_B1_PORT                        (gpioPortB)
//#define BOOTSEL_B1_PIN                         (1)

//#define BOOTSEL_B2_PORT                        (gpioPortB)
//#define BOOTSEL_B2_PIN                         (2)

#define BOOTSEL_B1_PORT                        TAG_GPIO_UART_TX_PORT
#define BOOTSEL_B1_PIN                         TAG_GPIO_UART_TX_PIN

#define BOOTSEL_B2_PORT                        TAG_GPIO_UART_RX_PORT
#define BOOTSEL_B2_PIN                         TAG_GPIO_UART_RX_PIN

/* AS393x */
#define TAG_GPIO_AS39_LF_DATA_PORT             (gpioPortA)
#define TAG_GPIO_AS39_DATA_PIN                 (0)
#define TAG_GPIO_AS39_LF_WAKEUP_PORT
#define TAG_GPIO_AS39_LF_WAKEUP_PIN

/* UART - USART1 */
#define TAG_GPIO_UART_TX_PORT                  (gpioPortA)
#define TAG_GPIO_UART_TX_PIN                   (5)
#define TAG_GPIO_UART_RX_PORT                  (gpioPortA)
#define TAG_GPIO_UART_RX_PIN                   (4)

/* SPI - USART0 */
#define TAG_GPIO_SPI_TX_PORT                   (gpioPortC)
#define TAG_GPIO_SPI_TX_PIN                    (2)
#define TAG_GPIO_SPI_RX_PORT                   (gpioPortC)
#define TAG_GPIO_SPI_RX_PIN                    (1)
#define TAG_GPIO_SPI_CLK_PORT                  (gpioPortC)
#define TAG_GPIO_SPI_CLK_PIN                   (3)
#define TAG_GPIO_AS39_SPI_CS_PORT              (gpioPortC)
#define TAG_GPIO_AS39_SPI_CS_PIN               (4)

#endif


#endif /* TAG_GPIO_MAPPING_H_ */
