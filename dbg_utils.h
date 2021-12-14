/*
 * dbg_utils.h
 *
 */

#ifndef DBG_UTILS_H_
#define DBG_UTILS_H_
#include "em_core.h"
#include "em_usart.h"
#include "stdio.h"
#include "tag_defines.h"


//******************************************************************************
// Defines
//******************************************************************************

#if defined(TAG_LOG_PRESENT)
#define DBG_LOG
#if defined(TAG_TRAP_PRESENT)
#define DBG_TRAP
#endif
#endif

#define SCREEN_CLEAR                 "[2J"         // Clear screen, reposition cursor to top left
#define ERASE_LINE                   "\33[2K"
#define TERMINAL_RESET               "[c"          // Reset terminal
#define COLOR_BOLD                   "[1m"
#define COLOR_UNDERLINE              "[4m"
#define COLOR_REVERSED               "[7m"
#define COLOR_RST                    "[0m"         // Reset to default colors

#define COLOR_BLACK                  "[30m"
#define COLOR_RED                    "[31m"
#define COLOR_GREEN                  "[32m"
#define COLOR_YELLOW                 "[33m"
#define COLOR_BLUE                   "[34m"
#define COLOR_MAGENTA                "[35m"
#define COLOR_CYAN                   "[36m"
#define COLOR_WHITE                  "[37m"

#define COLOR_B_BLACK                "[30;1m"
#define COLOR_B_RED                  "[31;1m"
#define COLOR_B_GREEN                "[32;1m"
#define COLOR_B_YELLOW               "[33;1m"
#define COLOR_B_BLUE                 "[34;1m"
#define COLOR_B_MAGENTA              "[35;1m"
#define COLOR_B_CYAN                 "[36;1m"
#define COLOR_B_WHITE                "[37;1m"

#define COLOR_BG_BLACK               "[24;40m"
#define COLOR_BG_RED                 "[24;41m"
#define COLOR_BG_GREEN               "[24;42m"
#define COLOR_BG_YELLOW              "[24;43m"
#define COLOR_BG_BLUE                "[24;44m"
#define COLOR_BG_MAGENTA             "[24;45m"
#define COLOR_BG_CYAN                "[24;46m"
#define COLOR_BG_WHITE               "[24;47m"


//******************************************************************************
// Data types
//******************************************************************************
typedef enum dbg_log_filters_t {
    DBG_CAT_TAG_LF =        (1 << 0),
    DBG_CAT_TAG_MAIN =      (1 << 1),
    DBG_CAT_TAG_BEACON =    (1 << 2),
    DBG_CAT_BLE =           (1 << 3),
    DBG_CAT_GENERIC =       (1 << 4),
    DBG_CAT_WARNING =       (1 << 5),
    DBG_CAT_TEMP =          (1 << 6),
    DBG_CAT_SYSTEM =        (1 << 7),
    DBG_CAT_ALL_DISABLED = 0,
    DBG_CAT_ALL_ENABLED = -1
} dbg_log_filters_t;

//******************************************************************************
// Extern global variables
//******************************************************************************
extern volatile bool log_enable;
extern volatile bool trap_enable;
extern volatile bool log_filter_disabled;
extern volatile dbg_log_filters_t log_filter_mask;
extern volatile bool sleep_on_isr_exit;

//******************************************************************************
// Macros
//******************************************************************************

#if defined(DBG_LOG)

char* dbg_log_filter_to_string(dbg_log_filters_t filter);
void print_timestamp(void);

#define DEBUG_LOG(log_filter, format, args...)                                                                    \
   do {                                                                                                           \
       if(log_enable) {                                                                                           \
           if (((log_filter & log_filter_mask) | log_filter_disabled)) {                                          \
               print_timestamp();                                                                                 \
               printf("[%s] (%s:%u) " format, dbg_log_filter_to_string(log_filter), __FILE__, __LINE__, ##args);  \
           }                                                                                                      \
       }                                                                                                          \
   } while(0)                                                                                                     \

#else
#define DEBUG_LOG(log_filter, format, args...)
#endif


#if defined(DBG_TRAP)
void dbg_trap(void);
#define DEBUG_TRAP(format, args...)                                                                            \
   do {                                                                                                        \
       if (trap_enable) {                                                                                      \
           printf(COLOR_RED "[DEBUG_TRAP] (%s:%u) " format "\n", __FILE__, __LINE__, ##args);                  \
           dbg_trap();                                                                                         \
       }                                                                                                       \
   } while(0)                                                                                                  \

#else
#define DEBUG_TRAP(format, args...)
#endif


//******************************************************************************
// Interface
//******************************************************************************
void dbg_log_enable(bool value);
bool dbg_is_log_enabled(void);

void dbg_trap_enable(bool value);
bool dbg_is_trap_enabled(void);

void dbg_log_remove_filter_rule(dbg_log_filters_t filter);
void dbg_log_add_filter_rule(dbg_log_filters_t filter);
void dbg_log_filter_disable(bool value);
void dbg_log_set_filter(dbg_log_filters_t filter);

void dbg_log_init(void);
void dbg_log_deinit(void);

void dbg_print_banner(void);
void dbg_print_git_info(void);

//!TODO Temporarily here
char* tag_tag_type_to_string(void);
uint8_t tag_get_tag_type(void);
uint32_t tag_get_fw_revision(void);

//!TODO Temporarily here
void EMU_EFPEM23PresleepHook(void);
void EMU_EFPEM23PostsleepHook(void);
void tag_sleep_on_isr_exit(bool enable);

#endif /* DBG_UTILS_H_ */


// Backups
/*
#define DEBUG_LOGC(color, log_filter, format, args...)                                                         \
    do {                                                                                                       \
        CORE_DECLARE_IRQ_STATE;                                                                                \
        if(!dbg_log_enable) {                                                                                  \
            break;                                                                                             \
        }                                                                                                      \
        if (((log_filter & log_filter_mask) | log_filter_enable)) {                                            \
            CORE_ENTER_ATOMIC();                                                                               \
            printf(color "\n[DEBUG_LOG] (%s:%u) " format COLOR_RST, __FILE__, __LINE__, ##args);               \
            CORE_EXIT_ATOMIC();                                                                                \
        }                                                                                                      \
    } while(0)                                                                                                 \
*/
