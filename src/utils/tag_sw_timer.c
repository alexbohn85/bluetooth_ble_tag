/*
 * tag_sw_timer.c
 *
 */

#include "stdio.h"
#include "tag_sw_timer.h"

//! @brief Tick sw timer
void tag_sw_timer_tick(tag_sw_timer_t *timer)
{
    if (timer->value > 0) {
        --timer->value;
    }
}

//! @brief Check if sw timer value expired (equals 0)
bool tag_sw_timer_is_expired(tag_sw_timer_t *timer)
{
    return (timer->value == 1? true : false);
}

//! @brief Reload sw timer value
void tag_sw_timer_reload(tag_sw_timer_t *timer, uint32_t reload_value)
{
    timer->value = reload_value + 1;
}

bool tag_sw_timer_is_running(tag_sw_timer_t *timer)
{
    return (timer->value > 1 ? true : false);
}

bool tag_sw_timer_is_stopped(tag_sw_timer_t *timer)
{
    return (timer->value == 0 ? true : false);
}
