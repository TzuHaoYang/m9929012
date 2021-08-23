/***********************************************************************
 *
 * <:copyright-BRCM:2015:DUAL/GPL:standard
 *
 *    Copyright (c) 2015 Broadcom
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
 *
 ************************************************************************/

#include <rdpa_types.h>


#define IFNAMESIZ (16)
#define wc_log_err(fmt, arg...) fprintf(stderr, "wanconf %s:%d >> " fmt "\n", __FILE__, __LINE__, ##arg);
#define wc_log(fmt, arg...) printf("wanconf: " fmt "\n", ##arg);
#define insmod(d) insmod_param(d, NULL)

#ifndef CONFIG_BCM963158
#define DETECTION_OK    ( 0)
#define DETECTION_ERROR (-1)

int try_wan_type_detect_and_set(rdpa_wan_type *wan_type);
#endif
