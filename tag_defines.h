/*
 * tag_defines.h
 *
 * BLE Tag platform definitions, compilation defines.
 *
 */

#ifndef TAG_DEFINES_H_
#define TAG_DEFINES_H_


//******************************************************************************
// Defines
//******************************************************************************
#define COMPILATION_TIME             (__DATE__" ("__TIME__")")

//TODO assign IDs based on available list.
#define TAG_UT3_ID                   (0xFF)
#define TAG_PT3_ID                   (0xFF)

// Define Tag Type compilation
#define TAG_TYPE                     (TAG_UT3_ID)

// Below GuardRFID Production FW Version 4 bytes Format (b4.b3.b2.b1)
// Internal/QA Releases increments "b1"
// External/Production Releases increments the other bytes and "b1" must be 0x00.
#define FW_B4                        (0x00)
#define FW_B3                        (0x00)
#define FW_B2                        (0x00)
#define FW_B1                        (0x01)
#define FW_REV                       ((FW_B4 << 24) | (FW_B3 << 16) | (FW_B2 << 8) | FW_B1)

// Compilation FW Features Switches --------------------------------------------

/* Watch Dog */
//#define TAG_WDOG_PRESENT

/* Boot Select */
#define TAG_BOOT_SELECT_PRESENT

/* Debug Log and Trap */
#define TAG_LOG_PRESENT
//#define TAG_TRAP_PRESENT

/* CLI */
#define TAG_CLI_PRESENT

/* Debug/Development mode */
#define TAG_DEV_MODE_PRESENT

// Compilation Switches Dependencies -------------------------------------------

/* TAG_CLI_PRESENT depends on TAG_LOG_PRESENT */
#if defined(TAG_CLI_PRESENT)
#ifndef TAG_LOG_PRESENT
#define TAG_LOG_PRESENT
#endif
#endif

// Force TAG_TRAP_PRESENT be defined only if TAG_DEV_MODE_PRESENT is defined
#ifndef TAG_DEV_MODE_PRESENT
#undef TAG_TRAP_PRESENT
#endif

//******************************************************************************
// Extern global variables
//******************************************************************************
extern const char tag_name[];

//******************************************************************************
// Interface
//******************************************************************************

#endif /* TAG_DEFINES_H_ */

