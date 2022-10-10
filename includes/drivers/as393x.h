/*
 *  as393x.h
 *
 *  AS393x device driver for Silabs ERF32 SoC
 *
 */

#ifndef AS393X_H_
#define AS393X_H_

#include "tag_gpio_mapping.h"
#include "spidrv.h"

//******************************************************************************
// Defines
//******************************************************************************
/*!
 *  @brief Select compilation AS3933 or AS3930 driver. \n\n
 *  Define here:
 *  \li \b AS39_DEVICE_AS3933 or \b AS39_DEVICE_AS3930
 */

#define AS39_DEVICE_AS3933           // Compile AS3933 part
//#define AS39_DEVICE_AS3930           // Compile AS3930 part

#define AS39_OK                      (0)
#define AS39_FAIL                    (1)
#define AS39_DRIVER_NOT_INITIATED    (2)
#define AS39_WRONG_SPI_PARAM         (3)

#define AS39_LF_DATA_PORT            TAG_GPIO_AS39_LF_DATA_PORT
#define AS39_LF_DATA_PIN             TAG_GPIO_AS39_DATA_PIN
#if defined(AS39_WAKE_UP_PRESENT)
#define AS39_WAKE_UP_PORT            TAG_GPIO_AS39_LF_WAKEUP_PORT
#define AS39_WAKE_UP_PIN             TAG_GPIO_AS39_LF_WAKEUP_PIN
#endif

#define AS39_REG_ARRAY_SIZE          (13)

#if defined(AS39_DEVICE_AS3933)
#define AS39_DEVICE_NAME             ("AS3933")
#define AS39_DEVICE_ID               (AS3933_ID)
#define AS39_DEFAULT_SETTINGS                                          \
  {                                                                    \
    {0, {0x00}},                                                       \
    {1, {0x20}},                                                       \
    {2, {0x02}},                                                       \
    {3, {0x39}},                                                       \
    {4, {0xB0}},                                                       \
    {5, {0x00}},                                                       \
    {6, {0x00}},                                                       \
    {7, {0x2B}},                                                       \
    {8, {0x00}},                                                       \
    {9, {0x00}},                           /* reserved */              \
    {10, {0x00}},                          /* read-only */             \
    {11, {0x00}},                          /* read-only */             \
    {12, {0x00}}                           /* read-only */             \
  }

#elif defined(AS39_DEVICE_AS3930)
#define AS39_DEVICE_NAME             ("AS3930")
#define AS39_DEVICE_ID               (AS3930_ID)
#define AS39_DEFAULT_SETTINGS
  {                                                                    \
    {0, {0x01}},                                                       \
    {1, {0x60}},                                                       \
    {2, {0x00}},                                                       \
    {3, {0x01}},                                                       \
    {4, {0xB0}},                                                       \
    {5, {0x00}},                                                       \
    {6, {0x00}},                                                       \
    {7, {0x2B}},                                                       \
    {8, {0x00}},                                                       \
    {9, {0x00}},                           /* reserved */              \
    {10, {0x00}},                          /* read-only */             \
    {11, {0x00}},                          /* reserved */              \
    {12, {0x00}}                           /* reserved */              \
  }

#endif

//******************************************************************************
// Data types
//******************************************************************************
/*!
 *  @brief AS393x registers addresses
 */
typedef enum as39_address_t {
    REG_0 = 0,  //!< 0x00
    REG_1,      //!< 0x01
    REG_2,      //!< 0x02
    REG_3,      //!< 0x03
    REG_4,      //!< 0x04
    REG_5,      //!< 0x05
    REG_6,      //!< 0x06
    REG_7,      //!< 0x07
    REG_8,      //!< 0x09
    REG_9,      //!< 0x0A
    REG_10,     //!< 0x0B
    REG_11,     //!< 0x0C
    REG_12      //!< 0x0D
} as39_address_t;

struct as39_r0_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
#if defined(AS39_DEVICE_AS3933)
            uint8_t : 1;
            uint8_t EN_1: 1;
            uint8_t EN_3: 1;
            uint8_t EN_2: 1;
            uint8_t MUX_123: 1;
            uint8_t ON_OFF: 1;
            uint8_t DAT_MASK: 1;
            uint8_t PATT32: 1;
#elif defined(AS39_DEVICE_AS3930)
            uint8_t PWD: 1;
            uint8_t EN_A: 1;
            uint8_t : 3;
            uint8_t ON_OFF: 1;
            uint8_t : 2;
#endif
        };
    };
};

struct as39_r1_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
#if defined(AS39_DEVICE_AS3933)
            uint8_t EN_XTAL: 1;
            uint8_t EN_WPAT: 1;
            uint8_t EN_PAT2: 1;
            uint8_t EN_MANCH: 1;
            uint8_t ATT_ON: 1;
            uint8_t AGC_UD: 1;
            uint8_t AGC_TLIM: 1;
            uint8_t ABS_HY: 1;
#elif defined(AS39_DEVICE_AS3930)
            uint8_t EN_RTC: 1;
            uint8_t EN_WPAT: 1;
            uint8_t EN_PAT2: 1;
            uint8_t : 1;
            uint8_t ATT_ON: 1;
            uint8_t AGC_UD: 1;
            uint8_t AGC_TLIM: 1;
            uint8_t ABS_HY: 1;
#endif
        };
    };
};

struct as39_r2_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
#if defined(AS39_DEVICE_AS3933)
            uint8_t S_WU1: 2;
            uint8_t DISPLAY_CLK: 2;
            uint8_t : 1;
            uint8_t G_BOOST: 1;
            uint8_t EN_EXT_CLK: 1;
            uint8_t S_ABS: 1;
#elif defined(AS39_DEVICE_AS3930)
            uint8_t S_WU1: 2;
            uint8_t : 3;
            uint8_t W_PAT_T: 2;
            uint8_t S_ABS: 1;
#endif
        };
    };
};

struct as39_r3_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
#if defined(AS39_DEVICE_AS3933)
            uint8_t FS_ENV: 3;
            uint8_t FS_SLC: 3;
            uint8_t HY_POS: 1;
            uint8_t HY_20m: 1;
#elif defined(AS39_DEVICE_AS3930)
            uint8_t FS_ENV: 3;
            uint8_t FS_SLC: 3;
            uint8_t HY_POS: 1;
            uint8_t HY_20m: 1;
#endif
        };
    };
};

struct as39_r4_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
            uint8_t GR: 4;
            uint8_t R_VAL: 2;
            uint8_t T_OFF: 2;
        };
    };
};

struct as39_r5_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
#if defined(AS39_DEVICE_AS3933)
            uint8_t PATT2B: 8;
#elif defined(AS39_DEVICE_AS3930)
            uint8_t TS2: 8;
#endif
        };
    };
};

struct as39_r6_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
#if defined(AS39_DEVICE_AS3933)
            uint8_t PATT1B: 8;
#elif defined(AS39_DEVICE_AS3930)
            uint8_t TS1: 8;
#endif
        };
    };
};

struct as39_r7_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
            uint8_t T_HBIT: 5;
            uint8_t T_OUT: 3;
        };
    };
};

struct as39_r8_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
#if defined(AS39_DEVICE_AS3933)
            uint8_t T_AUTO: 3;
            uint8_t : 2;
            uint8_t BAND_SEL: 3;
#elif defined(AS39_DEVICE_AS3930)
            uint8_t T_AUTO: 3;
            uint8_t : 5;
#endif
        };
    };
};

struct as39_r9_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
#if defined(AS39_DEVICE_AS3933)
            uint8_t : 7;
            uint8_t BLOCK_AGC: 1;
#elif defined(AS39_DEVICE_AS3930)
            uint8_t : 8;
#endif
        };
    };
};

struct as39_r10_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
            uint8_t RSSI1: 5;
            uint8_t : 3;
        };
    };
};

struct as39_r11_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
#if defined(AS39_DEVICE_AS3933)
            uint8_t RSSI2: 5;
            uint8_t : 3;
#elif defined(AS39_DEVICE_AS3930)
            uint8_t : 8;
#endif
        };
    };
};

struct as39_r12_t {
    const uint8_t addr;
    union {
        uint8_t value;
        struct {
#if defined(AS39_DEVICE_AS3933)
            uint8_t RSSI3: 5;
            uint8_t : 3;
#elif defined(AS39_DEVICE_AS3930)
            uint8_t : 8;
#endif
        };
    };
};

struct as39_data_t {
    struct as39_r0_t reg_0;
    struct as39_r1_t reg_1;
    struct as39_r2_t reg_2;
    struct as39_r3_t reg_3;
    struct as39_r4_t reg_4;
    struct as39_r5_t reg_5;
    struct as39_r6_t reg_6;
    struct as39_r7_t reg_7;
    struct as39_r8_t reg_8;
    struct as39_r9_t reg_9;
    struct as39_r10_t reg_10;
    struct as39_r11_t reg_11;
    struct as39_r12_t reg_12;
};

struct as39_iterator_t {
    uint8_t :8;           // skip address
    uint8_t value;
};

struct as39_settings_container_t {
    union {
        struct as39_data_t registers;
        struct as39_iterator_t iterator[13];
    };
};

typedef struct as39_settings_container_t * as39_settings_handle_t;

//******************************************************************************
// Interface
//******************************************************************************

/*!
 *  @brief Initialize AS393x device driver
 *  @param SPIDRV_Handle_t <as39_spi>
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_init(SPIDRV_Handle_t as39_spi);

/*!
 *  @brief Return device driver instance (AS393x register mapping)
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_get_settings_handler(as39_settings_handle_t *settings);

/*!
 *  @brief Write all registers from device driver instance to AS393x.
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_write_all_registers(void);

/*!
 *  @brief Read all registers from AS393x to device driver instance.
 *  @param as39_settings_handle_t <settings>
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_read_all_registers(as39_settings_handle_t *settings);

/*!
 *  @brief Read a register from AS393x.
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_read_reg(as39_address_t reg, uint8_t *value);

/*!
 *  @brief Write a register to AS393x.
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_write_reg(as39_address_t reg, uint8_t value);

#if defined(AS39_DEVICE_AS3933)
/*!
 *  @brief This function enables/disables AS3933 antenna receivers.\n
 *  Set to \b true if antenna will be enabled, otherwise \b false.
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_antenna_enable(bool EN_1, bool EN_2, bool EN_3);

#else
/*!
 *  @brief This function enables/disables AS3933 antenna receivers.\n
 *  Set to \b true if antenna will be enabled, otherwise \b false.
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_antenna_enable(bool EN_A);

/*!
 *  @brief This function enables/disables <b>Power-Down Mode</b> AS3930 only
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_power_down(bool PWD);
#endif

/*!
 *  @brief Sends direct command <b>"Clear Wake Up"</b>
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_cmd_clear_wake(void);

/*!
 *  @brief Sends direct command <b>"Reset RSSI"</b>
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_cmd_reset_rssi(void);

/*!
 *  @brief Sends direct command <b>"Trim Osc"</b>
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_cmd_trim_osc(void);

/*!
 *  @brief Sends direct command <b>"Clear False Wake-up"</b>
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_cmd_clear_false(void);

/*!
 *  @brief Sends direct command <b>"Preset"</b> (reset all registers to factory default).
 *  @return @ref AS39_OK on success, otherwise on failure.
 */
uint32_t as39_cmd_preset_default(void);

/*!
 *  @brief Get device part-number as string
 *  @return const char*
 */
const char* as39_get_device_name(void);

/**
 * @brief Return AS393x Gain Reduction setting
 * @details
 *     - <b>0:</b>  0 dB
 *     - <b>1:</b>  n.a
 *     - <b>2:</b> -4 dB
 *     - <b>3:</b> -8 dB
 *     - <b>4:</b> -12 dB
 *     - <b>5:</b> -16 dB
 *     - <b>6:</b> -20 dB
 *     - <b>7:</b> -24 dB
 *
 * @return uint8_t
 */
uint8_t as39_get_gain_setting(void);

#endif /* AS393X_H_ */

