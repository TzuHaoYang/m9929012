/* Copyright (c) 2016-2017, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>


#define MAX_PWM_DEVICES 2
/* The default period and duty cycle values to be configured. */
#define DEFAULT_PERIOD_NS       10*1000 // 10ms
#define DEFAULT_DUTY_CYCLE_NS   5*1000

static ssize_t count;
static uint32_t used_pwm[MAX_PWM_DEVICES];
static uint32_t pwm_offset[MAX_PWM_DEVICES];
static uint32_t pwm_gpio[MAX_PWM_DEVICES];

static void config_div_and_duty(struct pwm_device *pwm, int period_ns, int duty_ns)
{
	extern void led_set_brightnesscfg(unsigned int gpio_num, unsigned int brightness);
	extern void led_set_active(unsigned int gpio_num, unsigned int enable);

	u32 brightness;

	brightness = (duty_ns * 128)/period_ns;
	led_set_brightnesscfg(pwm_offset[pwm->hwpwm], brightness);

	/* configure activate register */
	led_set_active(pwm_offset[pwm->hwpwm], 1);
}

static int bcm6755_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	return 0;
}

static void bcm6755_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
}

static int bcm6755_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
						int duty_ns, int period_ns)
{
	config_div_and_duty(pwm, period_ns, duty_ns);

	return 0;
}

static int bcm6755_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	extern void kerSysSetGpioDir(unsigned short bpGpio);
	extern void led_set_sw_data(unsigned int gpio_num, unsigned int enable);
	extern void led_set_polarity(unsigned int gpio_num, unsigned int polarity);

	if (!used_pwm[pwm->hwpwm])
		return -EINVAL;

	/* configure GPIO direction */
	kerSysSetGpioDir(pwm_gpio[pwm->hwpwm]);

	/* enable input register which controls actual input value */
	led_set_sw_data(pwm_offset[pwm->hwpwm], 1);

	/* set led polarity to active high mode for fan */
	if (pwm->hwpwm == 1)
		led_set_polarity(pwm_offset[pwm->hwpwm], 1);

	pwm->period = DEFAULT_PERIOD_NS;
	pwm->duty_cycle = DEFAULT_DUTY_CYCLE_NS;

	bcm6755_pwm_config(chip, pwm, pwm->duty_cycle, pwm->period);

	return 0;
}

static void bcm6755_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
}


static struct pwm_ops bcm6755_pwm_ops = {
	.request = bcm6755_pwm_request,
	.free = bcm6755_pwm_free,
	.config = bcm6755_pwm_config,
	.enable = bcm6755_pwm_enable,
	.disable = bcm6755_pwm_disable,
	.owner = THIS_MODULE,
};

static int bcm6755_pwm_probe(struct platform_device *pdev)
{
	struct pwm_chip *pwm;
	struct device *dev;
	unsigned int base_index;
	int ret;

	dev = &pdev->dev;
	pwm = devm_kzalloc(dev, sizeof(*pwm), GFP_KERNEL);
	if (!pwm) {
		dev_err(dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, pwm);

	if (of_property_read_u32(dev->of_node, "pwm-base-index", &base_index))
		base_index = 0;

	count = of_property_count_u32_elems(dev->of_node, "used-pwm-indices");

	if (of_property_read_u32_array(dev->of_node, "used-pwm-indices",
			used_pwm, count))
		return -EINVAL;

	if (of_property_read_u32_array(dev->of_node, "pwm-offset",
			pwm_offset, count))
		return -EINVAL;

	if (of_property_read_u32_array(dev->of_node, "pwm-gpio",
			pwm_gpio, count))
		return -EINVAL;

	pwm->dev = dev;
	pwm->ops = &bcm6755_pwm_ops;
	pwm->base = base_index;
	pwm->npwm = count;
	pwm->can_sleep = false;

	ret = pwmchip_add(pwm);
	if (ret < 0) {
		dev_err(dev, "pwmchip_add() failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static int bcm6755_pwm_remove(struct platform_device *pdev)
{
	struct pwm_chip *pwm = platform_get_drvdata(pdev);

	return pwmchip_remove(pwm);
}

static const struct of_device_id pwm_msm_dt_match[] = {
	{
		.compatible = "plume,bcm6755-pwm",
	},
	{}
};

static struct platform_driver bcm6755_pwm_driver = {
	.driver = {
		.name = "bcm6755-pwm",
		.owner = THIS_MODULE,
		.of_match_table = pwm_msm_dt_match,
	},
	.probe = bcm6755_pwm_probe,
	.remove = bcm6755_pwm_remove,
};

module_platform_driver(bcm6755_pwm_driver);

MODULE_LICENSE("Dual BSD/GPL");

