/*
 * LED Kernel Blink Pattern Trigger
 *
 * Copyright 2016 Plume Design Inc.
 *
 * Author: Yongseok Joo (yongseok@plumewifi.com)
 * This module allows a user to configure blinking pattern on leds
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/leds.h>
#include "../leds.h"

#define MAX_PATTERN_NUM     8
#define MAX_PATTERN_STR_LEN 128

/* pattern_trig_data : pattern, # of pattern, total time(peirod), and timer */
struct pattern_trig_data {
	unsigned int curr_idx;
	unsigned int pattern[MAX_PATTERN_NUM];
	struct timer_list timer;
};

/* default pattern */
static int default_pattern[] = { 400000, 250000, 500000, 1000000, 400000, 250000, 500000, 1000000 };

static int validate_pattern(char *str) {
	int ret = 0, len = 0;

	len = strlen(str);

	if(len == 0) {
		printk(KERN_ERR "%s: zero length pattern.\n", __func__);
		goto error;
	}

	if(len > MAX_PATTERN_STR_LEN) {
		printk(KERN_ERR "%s: too long pattern(%d).\n", __func__, len);
		goto error;
	}

	while(*(++str)) {
		if(*str == ' ' || *str == '\n')
			continue;

		if((*str > '9') || (*str < '0')) {
			printk(KERN_ERR "%s: invalid character(\'%c\') in pattern\n", __func__, *str);
			goto error;
		}
	}

	ret = 1;

error:
	return ret;
}

/* creating pattern and schedule timer */
static void led_blink_pattern(unsigned long data)
{

	struct led_classdev *led_cdev = (struct led_classdev *) data;
	struct pattern_trig_data *t_data = led_cdev->trigger_data;

	/* avoiding overflow of array index */
	if(t_data->curr_idx >= MAX_PATTERN_NUM)
		t_data->curr_idx = 0;

	/* if curr_idx is even, then on / if it is odd, then off */
	if(t_data->curr_idx % 2) {
		led_set_brightness(led_cdev, LED_FULL);
	} else {
		led_set_brightness(led_cdev, LED_OFF);
	}

	mod_timer(&t_data->timer, jiffies + usecs_to_jiffies(t_data->pattern[t_data->curr_idx]));
	t_data->curr_idx++;
}

static ssize_t led_blink_pattern_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	int *p_data = ((struct pattern_trig_data *)led_cdev->trigger_data)->pattern;

	return sprintf(buf, "%u %u %u %u %u %u %u %u\n",
			p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5], p_data[6], p_data[7]);
}

static ssize_t led_blink_pattern_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	char pattern_str[MAX_PATTERN_STR_LEN + 1];

	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct pattern_trig_data *t_data = led_cdev->trigger_data;
	int *p_data = ((struct pattern_trig_data *)led_cdev->trigger_data)->pattern;;

	memset(pattern_str, 0, sizeof(pattern_str));
	strncpy(pattern_str, buf, MAX_PATTERN_STR_LEN);

	if(!validate_pattern(pattern_str))
		return -EINVAL;

	/* Remove previous value in the structure */
	t_data->curr_idx = 0;
	memset(p_data, 0, (sizeof(int) * MAX_PATTERN_NUM));
	ret = sscanf(pattern_str, "%u %u %u %u %u %u %u %u",
			&p_data[0], &p_data[1], &p_data[2], &p_data[3], &p_data[4], &p_data[5], &p_data[6], &p_data[7]);

	if(!ret) {
		printk(KERN_ERR "%s: invalid pattern(\"%s\")\n", __func__, pattern_str);
		return -EINVAL;
	}

	printk(KERN_INFO "%s: %u %u %u %u %u %u %u %u\n", __func__,
			p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5], p_data[6], p_data[7]);

	/* reset the LEDs */
	led_blink_pattern((unsigned long)led_cdev);

	return size;
}

/* pattern configuration file */
static DEVICE_ATTR(pattern, 0644, led_blink_pattern_show, led_blink_pattern_store);

static void
pattern_trig_activate(struct led_classdev *led_cdev)
{
	int rc;
	struct pattern_trig_data *t_data;
	int *p_data;

	/* creating pattern file for user input and review */
	rc = device_create_file(led_cdev->dev, &dev_attr_pattern);
	if(rc)
		return;

	t_data = kzalloc(sizeof(struct pattern_trig_data), GFP_KERNEL);
	if (!t_data) {
		/* fail-exit case. need to destroy device file */
		device_remove_file(led_cdev->dev, &dev_attr_pattern);
		return;
	}

	/* initialize the pattern data */
	t_data->curr_idx = 0;
	memcpy(t_data->pattern, default_pattern, sizeof(t_data->pattern));
	p_data = t_data->pattern;

	printk(KERN_INFO "%s: initializing....\n", __func__);
	printk(KERN_INFO "%s: %u %u %u %u %u %u %u %u\n", __func__,
			p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5], p_data[6], p_data[7]);

	led_cdev->trigger_data = t_data;

	setup_timer(&t_data->timer,
			led_blink_pattern, (unsigned long)led_cdev);

	led_blink_pattern(t_data->timer.data);
}

static void
pattern_trig_deactivate(struct led_classdev *led_cdev)
{
	struct pattern_trig_data *t_data= led_cdev->trigger_data;

	if (t_data) {
		del_timer_sync(&t_data->timer);
		device_remove_file(led_cdev->dev, &dev_attr_pattern);
		kfree(t_data);
	}
}

static struct led_trigger pattern_led_trigger = {
	.name     = "pattern",
	.activate = pattern_trig_activate,
	.deactivate = pattern_trig_deactivate,
};

static int __init pattern_trig_init(void)
{
	return led_trigger_register(&pattern_led_trigger);
}

static void __exit pattern_trig_exit(void)
{
	led_trigger_unregister(&pattern_led_trigger);
}

module_init(pattern_trig_init);
module_exit(pattern_trig_exit);

MODULE_AUTHOR("Yongseok Joo <yongseok@plumewifi.com>");
MODULE_DESCRIPTION("Pattern LED Trigger");
MODULE_LICENSE("GPLv2");
