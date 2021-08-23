/*
* <:copyright-BRCM:2016:DUAL/GPL:standard
*
*    Copyright (c) 2016 Broadcom
*    All Rights Reserved
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed
* to you under the terms of the GNU General Public License version 2
* (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
* with the following added to such license:
*
*    As a special exception, the copyright holders of this software give
*    you permission to link this software with independent modules, and
*    to copy and distribute the resulting executable under terms of your
*    choice, provided that you also meet, for each linked independent
*    module, the terms and conditions of the license of that module.
*    An independent module is a module which is not derived from this
*    software.  The special exception does not apply to any modifications
*    of the software.
*
* Not withstanding the above, under no circumstances may you combine
* this software in any way with any other Broadcom software provided
* under a license other than the GPL, without Broadcom's express prior
* written consent.
*
* :>
*/

#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/module.h>

typedef enum
{
    kGpioInactive,
    kGpioActive
} GPIO_STATE_t;

extern void kerSysSetGpioState(unsigned short bpGpio, GPIO_STATE_t state);
extern void kerSysSetGpioDir(unsigned short bpGpio);
extern int kerSysSetGpioDirInput(unsigned short bpGpio);
extern int kerSysGetGpioValue(unsigned short bpGpio);


static int bcm6755_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	kerSysSetGpioDirInput(gpio);
	return 0;
}

static int bcm6755_gpio_direction_output(struct gpio_chip *chip, unsigned gpio, int level)
{
	kerSysSetGpioDir(gpio);
	kerSysSetGpioState(gpio, level);
	return 0;
}

static int bcm6755_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	return kerSysGetGpioValue(gpio);
}

static void bcm6755_gpio_set_value(struct gpio_chip *chip, unsigned gpio, int value)
{
	kerSysSetGpioState(gpio, value);
}

static struct gpio_chip bcm6755_gpio_chip = {
	.label            = "BCM6755_GPIO_CHIP",
	.direction_input  = bcm6755_gpio_direction_input,
	.direction_output = bcm6755_gpio_direction_output,
	.get              = bcm6755_gpio_get_value,
	.set              = bcm6755_gpio_set_value,
	.base             = 0,
	.ngpio            = 68,
};

static int __init bcm6755_gpio_init(void)
{
	return gpiochip_add(&bcm6755_gpio_chip);
}
arch_initcall(bcm6755_gpio_init);
