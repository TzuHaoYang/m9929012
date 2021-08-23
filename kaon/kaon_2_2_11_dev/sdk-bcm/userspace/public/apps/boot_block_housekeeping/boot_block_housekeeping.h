/***********************************************************************
 * <:copyright-BRCM:2017:DUAL/GPL:standard
 *
 *    Copyright (c) 2017 Broadcom
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
 ************************************************************************/

#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <arpa/inet.h>

#include "bcm_hwdefs.h"
#include "board.h"
#include "bcm_flashutil_nand.h"

#define DATA_DIR "/data"

#if !defined HOUSEKEEPING_BOOTBLOCK_TIME_UNTIL_NEXT_CHECK
#define HOUSEKEEPING_BOOTBLOCK_TIME_UNTIL_NEXT_CHECK 100*24*60*60 /// 100 days before next check
#endif

#if !defined HOUSEKEEPING_BOOTBLOCK_CHECK_FILE_SYNC_TIME
#define HOUSEKEEPING_BOOTBLOCK_CHECK_FILE_SYNC_TIME 60*60
#endif

#if !defined HOUSEKEEPING_BOOTBLOCK_CHECK_BOOT_COUNTS
#define HOUSEKEEPING_BOOTBLOCK_CHECK_BOOT_COUNTS 100
#endif
#define BOOTBLOCK_CHECK_TIME 1
#define BOOT_BLOCK_CHECK_INFO_FILE DATA_DIR"/bootblock_check.info"

#if !defined HOUSEKEEPING_SLEEP_DURATION
#define HOUSEKEEPING_SLEEP_DURATION 3600
#endif

typedef struct
{
	int boot_count;
	int last_check_time;
}bootblock_check_info;

#define VERBOSE_FILE DATA_DIR"/verbose"
