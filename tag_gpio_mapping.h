/*
 * tag_gpios.h
 *
 */

#ifndef TAG_GPIO_MAPPING_H_
#define TAG_GPIO_MAPPING_H_

#include "tag_defines.h"

#if (TAG_TYPE == TAG_UT3_ID)

// AS393x
#define TAG_GPIO_AS39_LF_DATA_PORT             (gpioPortA)
#define TAG_GPIO_AS39_DATA_PIN                 (0)
#define TAG_GPIO_AS39_LF_WAKEUP_PORT
#define TAG_GPIO_AS39_LF_WAKEUP_PIN

// UART - USART1
#define TAG_GPIO_UART_TX_PORT                  (gpioPortA)
#define TAG_GPIO_UART_TX_PIN                   (5)
#define TAG_GPIO_UART_RX_PORT                  (gpioPortA)
#define TAG_GPIO_UART_RX_PIN                   (4)

// SPI - USART0
#define TAG_GPIO_SPI_TX_PORT                   (gpioPortC)
#define TAG_GPIO_SPI_TX_PIN                    (2)
#define TAG_GPIO_SPI_RX_PORT                   (gpioPortC)
#define TAG_GPIO_SPI_RX_PIN                    (1)
#define TAG_GPIO_SPI_CLK_PORT                  (gpioPortC)
#define TAG_GPIO_SPI_CLK_PIN                   (3)
// since we only have 1 slave we keep CS controlled by the SPI driver
#define TAG_GPIO_AS39_SPI_CS_PORT              (gpioPortC)
#define TAG_GPIO_AS39_SPI_CS_PIN               (4)

#endif


#endif /* TAG_GPIO_MAPPING_H_ */
