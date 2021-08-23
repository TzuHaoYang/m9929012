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
#include <linux/leds.h>
#include "../leds.h"


#define BRCM_LED_PWM_GPIO_OFFSET			22	/*should be the same as the pwm-offset definition for LED in 47622.dts*/
#define BRCM_LED_CONFIG0_PULSATING 			2
#define BRCM_LED_CONFIG0_REPEAT             		(1 << 14)
#define BRCM_LED_CONFIG0_DIRECT             		(1 << 15)
#define BRCM_LED_CONFIG0_INIT_DELAY_SHIFT		21
#define BRCM_LED_CONFIG0_INIT_DELAY			(3 << BRCM_LED_CONFIG0_INIT_DELAY_SHIFT)	/*3bits*/
#define BRCM_LED_CONFIG0_FINAL_DELAY_SHIFT		24
#define BRCM_LED_CONFIG0_FINAL_DELAY			(6 << BRCM_LED_CONFIG0_FINAL_DELAY_SHIFT)	/*4bits*/
#define BRCM_LED_CONFIG1_LVL1_BSTEP			3
#define BRCM_LED_CONFIG1_LVL1_TSTEP_SHIFT		4
#define BRCM_LED_CONFIG1_LVL1_TSTEP			(3 << BRCM_LED_CONFIG1_LVL1_TSTEP_SHIFT)	/*4bits for all step settings*/
#define BRCM_LED_CONFIG1_LVL1_NSTEP_SHIFT		8
#define BRCM_LED_CONFIG1_LVL1_NSTEP			(10 << BRCM_LED_CONFIG1_LVL1_NSTEP_SHIFT)
#define BRCM_LED_CONFIG1_LVL2_BSTEP_SHIFT		12
#define BRCM_LED_CONFIG1_LVL2_BSTEP			(6 << BRCM_LED_CONFIG1_LVL2_BSTEP_SHIFT)
#define BRCM_LED_CONFIG1_LVL2_TSTEP_SHIFT		16
#define BRCM_LED_CONFIG1_LVL2_TSTEP			(3 << BRCM_LED_CONFIG1_LVL2_TSTEP_SHIFT)
#define BRCM_LED_CONFIG1_LVL2_NSTEP_SHIFT		20
#define BRCM_LED_CONFIG1_LVL2_NSTEP			(11 << BRCM_LED_CONFIG1_LVL2_NSTEP_SHIFT)
#define BRCM_LED_CONFIG2_LVL3_BSTEP			3
#define BRCM_LED_CONFIG2_LVL3_TSTEP_SHIFT		4
#define BRCM_LED_CONFIG2_LVL3_TSTEP			(3 << BRCM_LED_CONFIG2_LVL3_TSTEP_SHIFT)
#define BRCM_LED_CONFIG2_LVL3_NSTEP_SHIFT		8
#define BRCM_LED_CONFIG2_LVL3_NSTEP			(10 << BRCM_LED_CONFIG2_LVL3_NSTEP_SHIFT)


/*
 * The *set_brightness_work* & *blink_timer* of *led_classdev* structure are cleaned up by led_trigger.c whenever trigger is removed.
 * We don't need to clean up the workqueue of *led_pwm_data* structure due to *can_sleep* is not set in pwm_bcm6755.c.
 */
static void breathe_trig_activate(struct led_classdev *led_cdev)
{
	extern void led_set_breathecfg(unsigned int gpio_num, unsigned int config[]);
	extern void led_set_active(unsigned int gpio_num, unsigned int enable);
	unsigned int config[3];

	config[0] = BRCM_LED_CONFIG0_PULSATING | BRCM_LED_CONFIG0_REPEAT | BRCM_LED_CONFIG0_DIRECT | BRCM_LED_CONFIG0_INIT_DELAY | BRCM_LED_CONFIG0_FINAL_DELAY;
	config[1] = BRCM_LED_CONFIG1_LVL1_BSTEP | BRCM_LED_CONFIG1_LVL1_TSTEP | BRCM_LED_CONFIG1_LVL1_NSTEP | BRCM_LED_CONFIG1_LVL2_BSTEP | BRCM_LED_CONFIG1_LVL2_TSTEP | BRCM_LED_CONFIG1_LVL2_NSTEP;
	config[2] = BRCM_LED_CONFIG2_LVL3_BSTEP | BRCM_LED_CONFIG2_LVL3_TSTEP | BRCM_LED_CONFIG2_LVL3_NSTEP;

	led_set_breathecfg(BRCM_LED_PWM_GPIO_OFFSET, config);
	led_set_active(BRCM_LED_PWM_GPIO_OFFSET, 1);
}

static void breathe_trig_deactivate(struct led_classdev *led_cdev)
{
	extern void led_set_breathecfg(unsigned int gpio_num, unsigned int config[]);
	extern void led_set_active(unsigned int gpio_num, unsigned int enable);
	unsigned int config[3] = {0, 0, 0};

	led_set_breathecfg(BRCM_LED_PWM_GPIO_OFFSET, config);
	led_set_active(BRCM_LED_PWM_GPIO_OFFSET, 1);
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

MODULE_DESCRIPTION("Breathe LED trigger");
MODULE_LICENSE("GPL");
