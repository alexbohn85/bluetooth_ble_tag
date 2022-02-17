/*
 * tag_defines.h
 *
 * BLE Tag platform definitions, compilation defines.
 *
 */

#ifndef TAG_DEFINES_H_
#define TAG_DEFINES_H_

#define COMPILATION_TIME             (__DATE__" ("__TIME__")")

//! Just a dummy value (check list of available IDs)
#define TAG_UT3                      (0xFF)

//! Define Tag Type
#define TAG_TYPE                     (TAG_UT3)

//! Below GuardRFID Production FW Version 4 bytes Format (b4.b3.b2.b1)
//! Internal/QA Releases increment "b1"
//! External/Production Releases "b1" must be 0x00

#define FW_B4                        (0x00)
#define FW_B3                        (0x00)
#define FW_B2                        (0x00)
#define FW_B1                        (0x01)
#define FW_REV                       ((FW_B4 << 24) | (FW_B3 << 16) | (FW_B2 << 8) | FW_B1)

//! Compilation Switches -------------------------------------------------------
//#define TAG_WDOG_PRESENT
#define TAG_BOOT_SELECT_PRESENT
#define TAG_LOG_PRESENT
#define TAG_CLI_PRESENT
#define TAG_DEBUG_MODE_PRESENT
//#define TAG_TRAP_PRESENT

//! Compilation Switches Dependencies ------------------------------------------
#if defined(TAG_DEBUG_MODE_PRESENT)
#undef TAG_WDOG_PRESENT
#endif

#if defined(TAG_CLI_PRESENT)
#ifndef TAG_LOG_PRESENT
#define TAG_LOG_PRESENT
#endif
#endif


#endif /* TAG_DEFINES_H_ */

