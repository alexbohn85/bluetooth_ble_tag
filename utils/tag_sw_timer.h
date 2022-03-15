/*
 * tag_sw_timer.h
 *
 */

#ifndef TAG_SW_TIMER_H_
#define TAG_SW_TIMER_H_

#include "stdio.h"
#include "stdbool.h"

typedef struct tag_sw_timer_t {
    uint32_t value;
    uint32_t reload_value;
} tag_sw_timer_t;

void tag_sw_timer_tick(tag_sw_timer_t *timer);
bool tag_sw_timer_is_expired(tag_sw_timer_t *timer);
void tag_sw_timer_reload(tag_sw_timer_t *timer, uint32_t reload_value);
bool tag_sw_timer_is_running(tag_sw_timer_t *timer);
bool tag_sw_timer_is_stopped(tag_sw_timer_t *timer);

#endif /* TAG_SW_TIMER_H_ */
