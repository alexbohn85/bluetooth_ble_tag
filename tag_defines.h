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

/** Tag Types IDs **/
#define UT3_ID                       (0x05)

/** Define Tag Type for the compilation **/
#define TAG_ID                       (UT3_ID)

//******************************************************************************
// Compilation FW Features Switches
//******************************************************************************

/** Watch Dog **/
//#define TAG_WDOG_PRESENT

/** Boot Select **/
#define TAG_BOOT_SELECT_PRESENT

/** Log **/
#define TAG_LOG_PRESENT

/** Trap **/
//#define TAG_TRAP_PRESENT

/** CLI **/
#define TAG_CLI_PRESENT

/** Development Mode **/
#define TAG_DEV_MODE_PRESENT

// Compilation Switches Dependencies -------------------------------------------
#if defined(TAG_CLI_PRESENT)
#ifndef TAG_LOG_PRESENT
#define TAG_LOG_PRESENT
#endif
#endif

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
uint8_t tag_get_tag_type_id(void);
char* tag_tag_type_to_string(void);

#endif /* TAG_DEFINES_H_ */

