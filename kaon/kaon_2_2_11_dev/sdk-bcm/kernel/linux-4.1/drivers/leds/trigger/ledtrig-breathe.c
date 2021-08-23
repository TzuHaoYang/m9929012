/*
 * LED Kernel Breathe Trigger
 *
 * Copyright 20016 Plume Design Inc.
 *
 * Author: Hong Fan <hong@wildfire.exchange>
 * Author: Mitja Horvat <mitja@plumewifi.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
#include <linux/leds.h>
#include "../leds.h"

struct breathe_trig_data
{
	struct led_classdev *led_cdev;
	unsigned long phase;
	unsigned long on_delay_us;
	unsigned long blinks;

	struct hrtimer phase_timer;
	struct hrtimer blink_timer;
};

/* Number of steps in the breathe phase */
#define BREATHE_PHASE_MAX 	50
/*
 * in msecs; interval at which the phase will increment
 * Multiplying this by BREATHE_PHASE_MAX will give you the total phase duration
 */
#define BREATHE_PHASE_INVL	50

/* in msecs; Total blink phase duration (on + off time) */
#define BLINK_PHASE_INVL	10

static enum hrtimer_restart blink_timer(struct hrtimer *timer)
{
	unsigned long brightness;
	unsigned long delay_us;

	struct breathe_trig_data *breathe_data = container_of(
			timer,
			struct breathe_trig_data,
			blink_timer);

	do
	{
		delay_us = breathe_data->on_delay_us;

		if ((breathe_data->blinks++ & 1) == 0)
		{
			brightness = LED_FULL;
		}
		else
		{
			brightness = LED_OFF;
			delay_us = BLINK_PHASE_INVL * USEC_PER_MSEC - delay_us;
		}
	}
	while (delay_us == 0);

	/* Turn on/off the LED */
	led_set_brightness(breathe_data->led_cdev, brightness);

	/* Re-arm the timer */
	hrtimer_forward_now(timer, ns_to_ktime(delay_us * NSEC_PER_USEC));

	return HRTIMER_RESTART;
}

static enum hrtimer_restart phase_timer(struct hrtimer *timer)
{
	unsigned long delay_us;

	struct breathe_trig_data *breathe_data = container_of(
			timer,
			struct breathe_trig_data,
			phase_timer);

	breathe_data->phase++;
	breathe_data->phase %= BREATHE_PHASE_MAX;

	/*
	 * The phase is cropped using %, so it basically generates a /|/|/| pattern. Convert
	 * it to a /\/\/\/\ pattern.
	 */
	if (breathe_data->phase >= (BREATHE_PHASE_MAX / 2))
	{
		delay_us = BREATHE_PHASE_MAX - breathe_data->phase;
	}
	else
	{
		delay_us = breathe_data->phase;
	}

	delay_us <<= 1;

	/*
	 *
	 * The duration of the LED on + off period is always BLINK_PHASE_INVL; The on/off time is scaled
	 * according to the brightness we want to achieve.
	 *
	 * The greater the on timer compared to the off time the brighter the LED
	 *
	 * Convert the delay from the phase number to microseconds
	 */
	delay_us = (BLINK_PHASE_INVL * USEC_PER_MSEC * delay_us + (BREATHE_PHASE_MAX >> 1)) / BREATHE_PHASE_MAX;

	breathe_data->on_delay_us = delay_us;

	/* Re-arm the timer */
	hrtimer_forward_now(timer, ms_to_ktime(BREATHE_PHASE_INVL));

	return HRTIMER_RESTART;
}

static void
breathe_trig_activate(struct led_classdev *led_cdev)
{
	struct breathe_trig_data *breathe_data;

	breathe_data = kzalloc(sizeof(*breathe_data), GFP_KERNEL);
	if (!breathe_data) return;

	breathe_data->led_cdev = led_cdev;
	breathe_data->phase = 0;
	breathe_data->on_delay_us =  0;
	breathe_data->blinks = 0;

	led_cdev->trigger_data = breathe_data;

	/* Initialize the blink timer */
	hrtimer_init(&breathe_data->blink_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	breathe_data->blink_timer.function = blink_timer;
	hrtimer_start(
			&breathe_data->blink_timer,
			ms_to_ktime(0),
			HRTIMER_MODE_REL);

	/* Initialize the phase timer */
	hrtimer_init(&breathe_data->phase_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	breathe_data->phase_timer.function = phase_timer;
	hrtimer_start(
			&breathe_data->phase_timer,
			ms_to_ktime(BREATHE_PHASE_INVL),
			HRTIMER_MODE_REL);
}

static void
breathe_trig_deactivate(struct led_classdev *led_cdev)
{
	struct breathe_trig_data *breathe_data = led_cdev->trigger_data;

	/* Stop blinking */
	led_set_brightness(led_cdev, LED_OFF);

	if (breathe_data == NULL) return;

	hrtimer_cancel(&breathe_data->blink_timer);
	hrtimer_cancel(&breathe_data->phase_timer);

	kfree(breathe_data);

	led_cdev->trigger_data = NULL;
}

static struct led_trigger breathe_led_trigger = {
	.name     = "breathe",
	.activate = breathe_trig_activate,
	.deactivate = breathe_trig_deactivate,
};

static int __init breathe_trig_init(void)
{
	return led_trigger_register(&breathe_led_trigger);
}

static void __exit breathe_trig_exit(void)
{
	led_trigger_unregister(&breathe_led_trigger);
}

module_init(breathe_trig_init);
module_exit(breathe_trig_exit);

MODULE_AUTHOR("Hong Fan <hongfan@wildfire.exchange>, Mitja Horvat <mitja@plumewifi.com>");
MODULE_DESCRIPTION("Breathe LED trigger");
MODULE_LICENSE("GPL");
