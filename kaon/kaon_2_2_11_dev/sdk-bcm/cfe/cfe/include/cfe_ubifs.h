/*  cfe_ubifs - simple parser for UBI + UBIFS partitions
 *
 *  Copyright (C) 2018  Plume Design Inc.
 */

#ifndef CFE_UBIFS_H_INCLUDED
#define CFE_UBIFS_H_INCLUDED


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


struct part_info
{
    ssize_t part_size;
    ssize_t part_off;
    ssize_t part_erasesize;
    ssize_t part_writesize;
};


bool ubifs_file_exists(const struct part_info *part_info, const char *filename);


#endif /* CFE_UBIFS_H_INCLUDED */
