#ifndef LED_PRIVATE_H
#define LED_PRIVATE_H

#include "osp_ledlib.h"

typedef enum
{
    LED_COLOR_INVALID,
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
    LED_COLOR_WHITE,  // The colors that follow are all composite colors
    LED_COLOR_ALL,
    LED_COLOR_MESMERIZE,
} led_colors_t;

led_state_t led_current_state();

#endif
