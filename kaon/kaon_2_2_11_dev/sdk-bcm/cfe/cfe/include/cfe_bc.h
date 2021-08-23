/*  bootconfig - Boot configuration application for BCM platform with only
 *  NAND flash
 *
 *  Copyright (C) 2018  Plume Design Inc.
 */

#ifndef __CFE_BC_H__
#define __CFE_BC_H__


/*
#include <stdlib.h>
#include <stdint.h>
*/

/**
 * just because Broadcom use their own definitions of uint8_t/uint32_t/...
 */
#define __SLONGWORD_TYPE    long int
# define __SWORD_TYPE                int

#define __OFF_T_TYPE            __SLONGWORD_TYPE
#define __SSIZE_T_TYPE          __SWORD_TYPE
# define __STD_TYPE             __extension__ typedef

__STD_TYPE __SSIZE_T_TYPE __ssize_t; /* Type of a byte count, or error.  */
__STD_TYPE __OFF_T_TYPE __off_t;        /* Type of file sizes and offsets.  */


#ifndef __off_t_defined
# ifndef __USE_FILE_OFFSET64
typedef __off_t off_t;
# else
typedef __off64_t off_t;
# endif
# define __off_t_defined
#endif
#if defined __USE_LARGEFILE64 && !defined __off64_t_defined
typedef __off64_t off64_t;
# define __off64_t_defined
#endif


#ifndef __ssize_t_defined
typedef __ssize_t ssize_t;
# define __ssize_t_defined
#endif
/**
 * end
 */



#define BOOTCFG_BADCOUNT_THRESHOLD      (16)
#define BOOTCFG_MAGIC                   (0xB001CF88UL)

#define BOOTCFG_RC_OK                   (0)
#define BOOTCFG_RC_ERR                  (-1)
#define BOOTCFG_RC_ERR_SEEK             (-2)
#define BOOTCFG_RC_ERR_READ             (-3)
#define BOOTCFG_RC_ERR_WRITE            (-4)
#define BOOTCFG_RC_ERR_ERASE            (-5)
#define BOOTCFG_RC_ERR_VERIFY           (-6)
#define BOOTCFG_RC_ERR_CHKSUM           (-7)
#define BOOTCFG_RC_ERR_LOG_EMPTY        (-8)
#define BOOTCFG_RC_ERR_MAGIC            (-9)
#define BOOTCFG_RC_ERR_BADCOUNT_THRESHOLD_REACHED    (-100)


struct bc_part_layout
{
    off_t offset;
    ssize_t size;
};

struct bootcfg_nand_img
{
    uint32_t cnt_current;
    uint32_t cnt_all_success;
    uint32_t cnt_threshold;
    uint32_t skip;
    uint32_t ignore_skip;
    uint8_t _res[12]; /* arbitrary alignment to 32 bytes */
}  __attribute__ ((packed));

struct bootcfg_nand_log
{
    uint32_t magic;
    uint64_t uid;
    uint32_t cnt_all_boots;
    uint32_t cnt_all_success;
    struct bootcfg_nand_img img[2];
    uint8_t _res[40]; /* arbitrary alignment to 128 bytes */
    uint32_t crc;
}  __attribute__ ((packed));

enum bootcfg_location
{
    BOOTCFG_LOC_UNKNOWN,
    BOOTCFG_LOC_1,
    BOOTCFG_LOC_2,
};

struct bootcfg_info
{
    off_t off_next_log;
    struct bootcfg_nand_log *last_log;
    uint8_t log_buf[2048]; /* TODO: should be NAND flash write pagesize */

    ssize_t part_size;
    ssize_t part_off;
    ssize_t part_erasesize;
    ssize_t part_writesize;
};

int cfe_bc_info(void);
int cfe_bc_reset(void);
int cfe_bc_write_ignore_skip(enum bootcfg_location loc);


int bootcfg_open(void);
int bootcfg_write_log_reset(void);
int bootcfg_write_log_btldr(enum bootcfg_location location);
int bootcfg_set_skip_img(enum bootcfg_location location);
int bootcfg_set_ignore_skip_img(enum bootcfg_location location);
int bootcfg_get_last_log(struct bootcfg_nand_log *log);
void bootcfg_dump_log(struct bootcfg_nand_log *log);


#endif
