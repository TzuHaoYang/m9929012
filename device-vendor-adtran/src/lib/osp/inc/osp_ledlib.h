/*
 * The library sets and clears states independently in a stack like
 * model. The same state may be set twice. When states are cleared and
 * there are more than one on the stack, the most recent one is cleared
 * first. When states are set, the most recent state that is set has
 * precedence over all others of the same priority and the LED will be
 * reflective of that states behavior. If a state has a higher priority,
 * it will hold the LED's behavior even if states with a lower priority
 * are set after it. If a state with the same higher priority is set after
 * it, that state will dictate the LED's behavior.
 *
 * The stack is currently 10 states deep. If a state is set and the stack
 * is full, the oldest state is forgotten and the new state is pushed onto
 * the stack as usual. This will protect the system from misbehaving clients
 * who set and never clear states or if the library is spammed with several
 * states all at once.
 *
 * This library also implements Plume's expected LED API found in
 * "osp_led.h". Internally, their API is translated into the one described
 * in this header file.
 */

#ifndef OSP_LEDLIB_H
#define OSP_LEDLIB_H

typedef enum
{
    LED_STATE_INVALID = 0,
    LED_STATE_FIRST = 1,
    LED_STATE_ERROR = 1,
    LED_STATE_PLUME_CONNECTED,
    LED_STATE_PLUME_CONNECTING,
    LED_STATE_PLUME_CONNECTFAIL,
    LED_STATE_WPS,
    LED_STATE_OPTIMIZE,
    LED_STATE_LOCATE,
    LED_STATE_PLUME_ERROR,
    LED_STATE_HWERROR,
    LED_STATE_THERMAL,
    LED_STATE_BTCONNECTABLE,
    LED_STATE_BTCONNECTING,
    LED_STATE_BTCONNECTED,
    LED_STATE_BTCONNECTFAIL,
    LED_STATE_UPGRADING,
    LED_STATE_UPGRADED,
    LED_STATE_UPGRADEFAIL,
    LED_STATE_HWTEST,
    LED_STATE_MESMERIZE,
    LED_STATE_DEFAULT_RESET,
    LED_STATE_FACTORY_RESET,
    LED_STATE_LAST,
    LED_STATE_OFF      // For internal use only
} led_state_t;

typedef enum
{
    LED_RESULT_OK,
    LED_RESULT_ERROR
} led_result_t;

typedef enum
{
    LED_PRIORITY_IGNORE,
    LED_PRIORITY_DEFAULT,
    LED_PRIORITY_HIGH
} led_priority_t;


led_result_t led_set(led_state_t state);
led_result_t led_clear(led_state_t state);

const char* led_state_to_str(led_state_t state);
led_state_t led_str_to_state(const char* str);

led_priority_t led_state_priority(led_state_t state);

typedef void (*state_stack_iter_func_t)(led_state_t state, void* param);
void led_state_stack_iter(state_stack_iter_func_t func, void* param);

led_state_t led_current_state();
void _state_stack_reset();

#endif
