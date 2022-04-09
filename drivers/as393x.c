/*
 *  as393x.c
 *
 *  AS393x device driver for ERF32BG22 SoC *
 *  Nov. 9, 2021
 *
 */

#include "stdio.h"
#include "string.h"
#include "em_gpio.h"
#include "em_ldma.h"
#include "spidrv.h"
#include "dbg_utils.h"
#include "as393x.h"


//******************************************************************************
// Defines
//******************************************************************************

// AS393x SPI configuration
#define AS39_SPI_CLKMODE             (spidrvClockMode1)
#define AS39_SPI_MODE                (spidrvMaster)
#define AS39_SPI_MAX_CLOCK           (2000000)
#define AS39_SPI_FRAME_LENGTH        (8)
#define AS39_SPI_BIT_ORDER           (spidrvBitOrderMsbFirst)
#define AS39_SPI_BUFFER_SIZE         (8)

//******************************************************************************
// Data types
//******************************************************************************
// AS393x command modes
typedef enum as39_command_modes_t {
    WRITE = 0x00,                    //!< 0x00
    READ = 0x40,                     //!< 0x40
    NOT_ALLOWED = 0x80,              //!< 0x80
    DIRECT_COMMAND = 0xC0            //!< 0xC0
} as39_command_modes_t;


// AS393x direct commands
enum as393x_direct_commands_t {
    CLEAR_WAKE = 0,                  //!< 0x00
    RESET_RSSI,                      //!< 0x01
    TRIM_OSC,                        //!< 0x02
    CLEAR_FALSE,                     //!< 0x03
    PRESET_DEFAULT,                  //!< 0x04
#if defined(AS39_DEVICE_AS3933)
    CALIB_RCO_LC                     //!< 0x05
#endif
};

// AS393x device driver internal data
struct as39_dev_handle_t {
    const char *device_name;
    union {
        struct as39_data_t registers;
        struct as39_iterator_t iter[13];
    };
    SPIDRV_Handle_t spi;
};

//******************************************************************************
// Global variables
//******************************************************************************
static struct as39_data_t as39_settings = AS39_DEFAULT_SETTINGS;
static struct as39_dev_handle_t _as39_dev_data;
static struct as39_dev_handle_t *_as39_dev_handle = NULL;

//******************************************************************************
// Static functions
//******************************************************************************
static bool _as39_is_initiated(void)
{
    bool status = false;
    if (_as39_dev_handle != NULL) {
        status = true;
    }
    return status;

}

static uint32_t _as39_init_spi(void)
{
    uint32_t status = AS39_OK;

    if (_as39_is_initiated()) {
        // AS393X CS is active high (but SPIDRV is active low by default)
        _as39_dev_handle->spi->peripheral.usartPort->CTRL |= USART_CTRL_CSINV_ENABLE;

        if (_as39_dev_handle->spi->initData.clockMode != AS39_SPI_CLKMODE) {
            status = AS39_WRONG_SPI_PARAM;
        } else if (_as39_dev_handle->spi->initData.bitRate > AS39_SPI_MAX_CLOCK) {
            status = AS39_WRONG_SPI_PARAM;
        } else if (_as39_dev_handle->spi->initData.frameLength != AS39_SPI_FRAME_LENGTH) {
            status = AS39_WRONG_SPI_PARAM;
        } else if (_as39_dev_handle->spi->initData.bitOrder != AS39_SPI_BIT_ORDER) {
            status = AS39_WRONG_SPI_PARAM;
        } else if (_as39_dev_handle->spi->initData.type != AS39_SPI_MODE) {
            status = AS39_WRONG_SPI_PARAM;
        }
    } else {
        status = AS39_DRIVER_NOT_INITIATED;
    }

    return status;
}

static void _as39_init_gpio(void)
{
    CMU_ClockEnable(cmuClock_GPIO, true);
    GPIO_PinModeSet(AS39_LF_DATA_PORT, AS39_LF_DATA_PIN, gpioModeInput, 0);
    GPIO_IntConfig(AS39_LF_DATA_PORT, AS39_LF_DATA_PIN, false, false, false);
#if defined(AS39_WAKE_UP_PRESENT)
    GPIO_PinModeSet(AS39_WAKE_UP_PORT, AS39_WAKE_UP_PIN, gpioModeInput, 0);
#endif
}

static uint32_t _as39_direct_command(enum as393x_direct_commands_t cmd)
{
    Ecode_t status;
    uint8_t command = (DIRECT_COMMAND | cmd);
    status = SPIDRV_MTransmitB(_as39_dev_handle->spi, &command, 1);
    return status;
}

static uint32_t _as39_write_byte(as39_address_t reg_addr, uint8_t value)
{
    Ecode_t status;
    uint8_t command = (WRITE | reg_addr);
    uint8_t txBuffer[2] = { command, value };

    status = SPIDRV_MTransmitB(_as39_dev_handle->spi, &txBuffer, 2);

    if (status != ECODE_EMDRV_SPIDRV_OK) {
        return (uint32_t)status;
    }

    return status;
}

static uint32_t _as39_read_byte(as39_address_t reg_addr, uint8_t *data)
{
    Ecode_t status;

    uint8_t command = (READ | reg_addr);
    uint8_t txBuffer[2] = { command, 0x00 };
    uint8_t rxBuffer[2] = { 0 };

    status = SPIDRV_MTransferB(_as39_dev_handle->spi, &txBuffer, &rxBuffer, 2);
    if (status == ECODE_EMDRV_SPIDRV_OK) {
        *(data) = rxBuffer[1];
    }

    return (uint32_t)status;
}

static uint32_t _as39_write_burst(as39_address_t start_reg, uint8_t *buf, uint8_t size)
{
    Ecode_t status;

    uint8_t command = (WRITE | start_reg);

    uint8_t txBuffer[AS39_REG_ARRAY_SIZE+1];

    memset(txBuffer, 0, sizeof(txBuffer));

    txBuffer[0] = command;

    memcpy(&txBuffer[1], buf, size);

    status = SPIDRV_MTransmitB(_as39_dev_handle->spi, txBuffer, sizeof(txBuffer));
    if (status != ECODE_EMDRV_SPIDRV_OK) {
        return (uint32_t)status;
    }

    return status;
}

static uint32_t _as39_read_burst(as39_address_t start_reg, uint8_t *buf, uint8_t size)
{
    Ecode_t status;

    uint8_t command = (READ | start_reg);

    uint8_t txBuffer[AS39_REG_ARRAY_SIZE + 1];
    uint8_t rxBuffer[AS39_REG_ARRAY_SIZE + 1];

    memset(txBuffer, 0, sizeof(txBuffer));

    txBuffer[0] = command;

    status = SPIDRV_MTransferB(_as39_dev_handle->spi, txBuffer, rxBuffer, sizeof(txBuffer));
    if (status == ECODE_EMDRV_SPIDRV_OK) {
        memcpy(buf, &rxBuffer[1], size);
    }

    return status;
}

//******************************************************************************
// Non Static functions
//******************************************************************************

/*!
 *  @brief Initialize AS393x device driver
 *  @param SPIDRV_Handle_t as39_spi
 *  @brief User must provide the SPIDRV handler (instance) so AS393x driver
 *      can communicate with the device.
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_init(SPIDRV_Handle_t as39_spi)
{
    uint32_t status;

    // Check if AS39 device driver was already initialized
    if(_as39_dev_handle != NULL) {
        return AS39_OK;
    }

    if (as39_spi == NULL) {
        return AS39_FAIL;
    }

    _as39_dev_handle = &_as39_dev_data;
    _as39_init_gpio();
    _as39_dev_data.spi = as39_spi;
    status = _as39_init_spi();

    if (status != 0) {
        return status;
    }

    // Send preset command to AS393x (this will AS393x settings in reset values)
    status = as39_cmd_preset_default();

    // Read all registers from AS393x to initialize device driver instance.
    status = as39_read_all_registers(NULL);

    if (status == AS39_OK) {
        // Copy AS39 device driver default settings into device driver instance.
        memcpy(&_as39_dev_handle->registers, &as39_settings, sizeof(as39_settings));

        // Send all settings to AS393x
        as39_write_all_registers();
        //*user_settings = (as39_settings_handle_t)(&_as39_dev_handle->registers);
    }

    _as39_dev_handle->device_name = AS39_DEVICE_NAME;
    DEBUG_LOG(DBG_CAT_TAG_LF, "%s device driver initialized...", _as39_dev_handle->device_name);

    return status;
}

uint32_t as39_get_settings_handler(as39_settings_handle_t *as39_handle)
{
    uint32_t status = 0;
    if (_as39_is_initiated()) {
        *as39_handle = (as39_settings_handle_t)(&_as39_dev_handle->registers);
    } else {
        *as39_handle = 0;
        status = AS39_DRIVER_NOT_INITIATED;
    }
    return status;
}

#if 0
static uint32_t as39_write_register(as39_address_t reg)
{
    uint32_t status;
    if(_as39_is_initiated()) {
        status = _as39_write_byte(reg, _as39_dev_handle->iter[reg].value);
    } else {
        status = AS39_DRIVER_NOT_INITIATED;
    }
    return status;
}

static uint32_t as39_read_register(as39_address_t reg)
{
    uint32_t status;
    uint8_t temp;

    if (_as39_is_initiated()) {
        status = _as39_read_byte(reg, &temp);
        if (status == ECODE_EMDRV_SPIDRV_OK)
        {
            _as39_dev_handle->iter[reg].value = temp;
        }
    } else {
        status = AS39_DRIVER_NOT_INITIATED;
    }

    return status;
}
#endif

uint32_t as39_write_reg(as39_address_t reg, uint8_t value)
{
    uint32_t status;
    if (_as39_is_initiated()) {

        status = _as39_write_byte(reg, value);

        if (status == ECODE_EMDRV_SPIDRV_OK) {
            _as39_dev_handle->iter[reg].value = value;
        }
    } else {
        status = AS39_DRIVER_NOT_INITIATED;
    }

    return status;
}

uint32_t as39_read_reg(as39_address_t reg, uint8_t *value)
{
    uint32_t status;
    if (_as39_is_initiated()) {

        status = _as39_read_byte(reg, value);

        if (status == ECODE_EMDRV_SPIDRV_OK) {
            *value = _as39_dev_handle->iter[reg].value;
        }
    } else {
        status = AS39_DRIVER_NOT_INITIATED;
    }

    return status;
}

uint32_t as39_write_all_registers(void)
{
    uint8_t i;
    uint32_t status;


    if (_as39_dev_handle == NULL) {
        status = AS39_DRIVER_NOT_INITIATED;
    } else {

        // RW registers from 0 to 7 (others we do not care)
        uint8_t buffer[8];
        for (i = 0; i < sizeof(buffer); i++) {
            buffer[i] = _as39_dev_handle->iter[i].value;
        }

        status = _as39_write_burst(REG_0, buffer, sizeof(buffer));
    }

    return status;
}

uint32_t as39_read_all_registers(as39_settings_handle_t *data)
{
    uint8_t i;
    uint32_t status;

    if (_as39_dev_handle == NULL) {
        status = AS39_DRIVER_NOT_INITIATED;
    } else {

        uint8_t rxBuffer[AS39_REG_ARRAY_SIZE] = {0};

        status = _as39_read_burst(REG_0, rxBuffer, sizeof(rxBuffer));

        if (status == ECODE_EMDRV_SPIDRV_OK) {
            for (i = 0; i < sizeof(rxBuffer); i++) {
                _as39_dev_handle->iter[i].value = rxBuffer[i];
            }
            *data = (as39_settings_handle_t) (&_as39_dev_handle->registers);
        }
    }

    return status;
}

#if defined(AS39_DEVICE_AS3933)
uint32_t as39_antenna_enable(bool EN_1, bool EN_2, bool EN_3)
#else
/*!
 *  @brief This function enables/disables AS3930 antenna receiver. \n\n
 *  @param[in] bool
 *  @return AS39_OK on success, otherwise on failure.
 */
uint32_t as39_antenna_enable(bool EN_A)
#endif
{
    Ecode_t status;
    uint8_t temp = _as39_dev_handle->registers.reg_0.value;

#if defined(AS39_DEVICE_AS3933)
    _as39_dev_handle->registers.reg_0.EN_1 = EN_1;
    _as39_dev_handle->registers.reg_0.EN_2 = EN_2;
    _as39_dev_handle->registers.reg_0.EN_3 = EN_3;
#else
    _as39_dev_handle->registers.reg_0.EN_A = EN_A;
#endif

    status = _as39_write_byte(REG_0, _as39_dev_handle->registers.reg_0.value);

    // If error, restore configuration
    if (status != ECODE_EMDRV_SPIDRV_OK) {
        _as39_dev_handle->registers.reg_0.value = temp;
    }

    return status;
}

#if defined(AS39_DEVICE_AS3930)
uint32_t as39_power_down(bool PWD)
{
    Ecode_t status;
    uint8_t temp = _as39_dev_handle->registers.reg_0.value;
    _as39_dev_handle->registers.reg_0.PWD = PWD;
    status = _as39_write_byte(REG_0, _as39_dev_handle->registers.reg_0.value);

    // If error, restore previous value
    if (status != ECODE_EMDRV_SPIDRV_OK) {
        _as39_dev_handle->registers.reg_0.value = temp;
    }

    return status;
}
#endif

uint32_t as39_cmd_clear_wake(void)
{
    return _as39_direct_command(CLEAR_WAKE);
}

uint32_t as39_cmd_reset_rssi(void)
{
    return _as39_direct_command(RESET_RSSI);
}

uint32_t as39_cmd_clear_false(void)
{
    return _as39_direct_command(CLEAR_FALSE);
}

uint32_t as39_cmd_preset_default(void)
{
    return _as39_direct_command(PRESET_DEFAULT);
}

const char* as39_get_device_name(void)
{
    return _as39_dev_handle->device_name;
}

uint8_t as39_get_attenuation_set(void)
{
    return (_as39_dev_handle->registers.reg_4.GR & 0x0E) >> 1;
}
