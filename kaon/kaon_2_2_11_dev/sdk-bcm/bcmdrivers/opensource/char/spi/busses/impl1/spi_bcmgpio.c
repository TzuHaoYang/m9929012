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

/*
 *******************************************************************************
 * File Name  : spi_bcmgpio.c
 *
 * Description: This file contains Broadcom spi gpio device driver using Linux
 *              spi Bit-Bang driver.
 *
 *******************************************************************************
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/gpio.h>

#include <bcm_gpio.h>
#include <board.h>
#include <linux/platform_device.h>

/****************************************************************************
 * If CONFIG_SPI_GPIO is defined, the kernel SPI gpio driver,
 * kernel/linux/drivers/spi/busses/spi-gpio.c, is compiled into the image.
 * Register a device with it.
 *
 ****************************************************************************/

static struct spi_board_info spi_board_info[] __initdata = {
    {
        .modalias = "spidev",
        .max_speed_hz = 1000000,     /* max spi clock (SCK) speed in HZ */
        .bus_num = 0,
        .chip_select = 0,
        .mode = SPI_MODE_0,
    },
};

static struct spi_gpio_platform_data bcm_spi_gpio_platform_data;


static struct platform_device bcm_spi_gpio = {
    .name      = "spi_gpio",
    .id        = 0,
    .dev       = {
        .platform_data= &bcm_spi_gpio_platform_data
    },
};

static struct platform_device *bcm_spi_devices[] __initdata = {
    &bcm_spi_gpio,
};


static int __init spi_bcm_gpio_init(void)
{
    unsigned short bpGpio_miso, bpGpio_mosi, bpGpio_sclk, bpGpio_cs;
    int cs;
    int ret;

    BpGetSpiClkGpio( &bpGpio_sclk );
    BpGetSpiCsGpio( &bpGpio_cs );
    BpGetSpiMisoGpio( &bpGpio_miso );
    BpGetSpiMosiGpio( &bpGpio_mosi );

    if (bpGpio_sclk != BP_NOT_DEFINED &&
        bpGpio_cs != BP_NOT_DEFINED &&
        bpGpio_miso != BP_NOT_DEFINED &&
        bpGpio_mosi != BP_NOT_DEFINED) 
    {
        bcm_spi_gpio_platform_data.sck = bpGpio_sclk;
        bcm_spi_gpio_platform_data.mosi = bpGpio_mosi;
        bcm_spi_gpio_platform_data.miso = bpGpio_miso;
        bcm_spi_gpio_platform_data.num_chipselect = 1;
 
        printk("spi_bcm_gpio_init: SCK=%u and MOSI=%lu MISO=%lu\n",
                  bcm_spi_gpio_platform_data.sck, 
                  bcm_spi_gpio_platform_data.mosi,
                  bcm_spi_gpio_platform_data.miso);

        cs = bpGpio_cs;
        spi_board_info[0].controller_data = (void *)cs;
        
        ret = spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
        printk("spi_bcm_gpio_init register spidev status=%d\n", ret);
        
        if (ret != 0)
        {
            return ret;
        }

        ret = platform_add_devices(bcm_spi_devices, ARRAY_SIZE(bcm_spi_devices));
        printk("spi_bcm_gpio_init registering SPI driver status=%d\n", ret);

    }
    else
    {
        printk("spi_bcm_gpio_init: GPIO pins undefined\n");
        ret = 1;
    }

    return ret; /* successful */
}
arch_initcall(spi_bcm_gpio_init);

static void __exit spi_bcm_gpio_exit(void)
{
}
module_exit(spi_bcm_gpio_exit);

MODULE_DESCRIPTION("Broadcom SPI GPIO driver");
MODULE_LICENSE("GPL");
