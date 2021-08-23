/*  bootconfig - Boot configuration application for BCM platform with only
 *  NAND flash
 *
 *  Copyright (C) 2018  Plume Design Inc.
 */

#include "bcm63xx_util.h"
#include "bcm63xx_auth.h"
#include "flash_api.h"
#include "shared_utils.h"
#include "jffs2.h"
#include "lib_math.h"
#if (INC_NAND_FLASH_DRIVER == 1) || (INC_SPI_PROG_NAND == 1) || (INC_SPI_NAND_DRIVER == 1)
#include "bcm_ubi.h"
#endif
#if (INC_PMC_DRIVER==1)
#include "pmc_drv.h"
#include "BPCM.h"
#include "clk_rst.h"
#endif
#if defined(_BCM94908_) || defined(_BCM963158_) || defined(_BCM963178_) || defined(_BCM947622_)
#include "command.h"
#endif
#if defined (_BCM963268_) || defined (_BCM963381_)
#include "mii_shared.h"
#include "robosw_reg.h"
#elif (defined (_BCM96838_) || defined (_BCM96848_)) && (NONETWORK==0)
#include "phys_common_drv.h"
#elif defined (_BCM960333_) || defined(_BCM947189_)
#else
#if (NONETWORK == 0)
#include "bcm_ethsw.h"
#endif
#endif

#include "bcm_otp.h"
#if (INC_KERMIT==1) && (NONETWORK==1)
#include "cfe_kermit.h"
#endif
#include "bcm63xx_blparms.h"
#include "btrm_if.h"
#if (defined (_BCM96838_) || defined (_BCM96848_) || defined (_BCM96858_) || defined (_BCM96846_) || defined (_BCM96856_) || defined (_BCM96878_) ) && (NONETWORK == 0)
#include "rdp_cpu_ring.h"
#endif
#include "lib_byteorder.h"
#include "lib_malloc.h"
#include "lib_crc.h"
#include "rom_parms.h"
#include "bcm63xx_nvram.h"

#if defined (_BCM96858_) && (NONETWORK == 0)
#include "lport_drv.h"
#include "lport_stats.h"
#include "serdes_access.h"
#include "bcm6858_lport_xlmac_ag.h"
#define LPORT_MAX_PHY_ID (31)
#endif

#if (defined (_BCM96846_) || defined (_BCM96856_) || defined (_BCM96878_)) && (NONETWORK == 0)
#include "mdio_drv_impl5.h"
#include "mac_drv.h"
#endif


#include <stdbool.h>
#include "cfe_bc.h"


/**
 * CFE Bootconfig UI
 */


int cfe_bc_info(void)
{
    int rv;
    struct bootcfg_nand_log log;

    bootcfg_open();

    rv = bootcfg_get_last_log(&log);
    if (rv == BOOTCFG_RC_ERR_LOG_EMPTY)
    {
        printf("Bootconfig log is empty\n");
    }
    else if (rv != BOOTCFG_RC_OK)
    {
        return rv;
    }
    bootcfg_dump_log(&log);

    return 0;
}

int cfe_bc_reset(void)
{
    int rv;
    struct bootcfg_nand_log log;

    bootcfg_open();

    rv = bootcfg_get_last_log(&log);
    if (rv == BOOTCFG_RC_ERR_LOG_EMPTY)
    {
        printf("Bootconfig log is empty\n");
    }
    else if (rv != BOOTCFG_RC_OK)
    {
        return rv;
    }

    rv = bootcfg_write_log_reset();
    if (rv != BOOTCFG_RC_OK)
    {
        return rv;
    }

    return 0;
}

int cfe_bc_write_ignore_skip(enum bootcfg_location loc)
{
    int rv;
    struct bootcfg_nand_log log;

    bootcfg_open();

    rv = bootcfg_get_last_log(&log);
    if (rv == BOOTCFG_RC_ERR_LOG_EMPTY)
    {
        printf("Bootconfig log is empty\n");
    }
    else if (rv != BOOTCFG_RC_OK)
    {
        return rv;
    }

    rv = bootcfg_set_ignore_skip_img(loc);
    if (rv != BOOTCFG_RC_OK)
    {
        return rv;
    }

    return 0;
}


/**
 * Bootconfig
 */


#ifndef MIN
#define MIN(a,b) \
    ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })
#endif


//#define DEBUG

#ifdef DEBUG
    #define LOGD(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
    #define LOGD(fmt, ...)
#endif

#define LOGI(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)


static struct bootcfg_info bc_info;


static void get_sector_from_offset(off_t off, unsigned short *sector, int *sector_offset)
{
    off += bc_info.part_off;

    *sector = off / bc_info.part_erasesize;
    *sector_offset = off % bc_info.part_erasesize;
}

static int fd_read(off_t off, void *data, ssize_t len)
{
    int rv;
    unsigned short sector;
    int sector_offset;

    get_sector_from_offset(off, &sector, &sector_offset);

    rv = flash_read_buf(sector, sector_offset, data, len);
    if (rv != len)
    {
        LOGE("Could not read data; sector: %d; sector_off: %d; len: 0x%04x",
            sector, sector_offset, len);
        return BOOTCFG_RC_ERR_READ;
    }

    return BOOTCFG_RC_OK;
}

static int fd_write(off_t off, void *data, ssize_t len)
{
    int rv;
    unsigned short sector;
    int sector_offset;

    get_sector_from_offset(off, &sector, &sector_offset);

    rv = flash_write_buf(sector, sector_offset, data, len);
    if (rv != len)
    {
        LOGE("Could not write data; sector: %d; sector_off: %d; len: 0x%04x",
            sector, sector_offset, len);
        return BOOTCFG_RC_ERR_WRITE;
    }

    return BOOTCFG_RC_OK;
}

static int fd_erase(off_t off)
{
    int rv;
    unsigned short sector;
    int sector_offset;

    get_sector_from_offset(off, &sector, &sector_offset);

    rv = flash_sector_erase_int(sector);
    if (rv != FLASH_API_OK) {
        LOGE("Could not erase sector; sector: %d", sector);
        return BOOTCFG_RC_ERR_ERASE;
    }

    return BOOTCFG_RC_OK;
}

static bool memcmp_b(const uint8_t * buff, uint8_t value, ssize_t sz)
{
    ssize_t i = 0;
    bool cmp = true;

    while (i < sz)
    {
        if (buff[i++] != value)
        {
            cmp = false;
            break;      /* while    */
        }
    }

    return cmp;
}

static void calc_extra_part_layout(struct bc_part_layout *parts, int parts_num)
{
    int i;
    ssize_t extra = 0;

    parts[parts_num-1].offset = NVRAM.ulNandPartOfsKb[NP_DATA] * 1024;
    parts[parts_num-1].size = NVRAM.ulNandPartSizeKb[NP_DATA] * 1024;

    /* skip the /data partition */
    for (i = BCM_MAX_EXTRA_PARTITIONS - 2; i >= 0; i--)
    {
        ssize_t extra_part_size = convert_to_data_partition_entry_to_bytes(NVRAM.part_info[i].size);

        extra += extra_part_size;
        parts[i].size = extra_part_size;
        parts[i].offset = parts[parts_num-1].offset - extra;
    }
}


static void get_nand_flash_info(struct bootcfg_info *info, uint32_t part_idx)
{
    struct bc_part_layout extra_parts[BCM_MAX_EXTRA_PARTITIONS];

    info->part_erasesize = flash_get_sector_size(0);
    /* TODO: how do we get this programmatically */
    info->part_writesize = 2048;

    calc_extra_part_layout(extra_parts, BCM_MAX_EXTRA_PARTITIONS);

    info->part_size = extra_parts[part_idx].size;
    info->part_off = extra_parts[part_idx].offset;
}

static int location_to_index(enum bootcfg_location loc)
{
    if (loc == BOOTCFG_LOC_1)
    {
        return 0;
    }
    else if (loc == BOOTCFG_LOC_2)
    {
        return 1;
    }

    LOGE("Bootconfig location not recognized: %d", loc);
    return -1;
}


static uint32_t calc_crc(struct bootcfg_nand_log *log)
{
    /* crc field is located at the end of the log structure, and is of type uint32_t */
    return ~lib_get_crc32((void *)log, sizeof(*log)-sizeof(uint32_t), 0xFFFFFFFFUL);
}


static int verify_flash(off_t start_off, uint8_t *data, ssize_t datalen)
{
    uint8_t buf[256];
    off_t off = start_off;
    off_t end_off = start_off + datalen;

    LOGD("%s; 0x%08lx:0x%08x", __func__, start_off, datalen);

    while (off < end_off)
    {
        ssize_t rem = MIN(datalen, (ssize_t)sizeof(buf));
        int rv;

        rv = fd_read(off, buf, rem);
        if (rv != BOOTCFG_RC_OK)
        {
            return rv;
        }

        if (memcmp(data, buf, rem) != 0)
        {
            LOGE("Verification failed; off: 0x%08lx", off);
            return BOOTCFG_RC_ERR_VERIFY;
        }

        data += rem;
        datalen -= rem;
        off += rem;
    }

    return BOOTCFG_RC_OK;
}

static int sector_verify_erase(off_t start_off, ssize_t erasesize)
{
    uint8_t buf[256];
    off_t off = start_off;
    off_t end_off = start_off + erasesize;

    LOGD("%s; 0x%08lx:0x%04x", __func__, start_off, erasesize);

    while (off < end_off)
    {
        ssize_t rem = MIN(erasesize, (ssize_t)sizeof(buf));
        int rv;

        rv = fd_read(off, buf, rem);
        if (rv != BOOTCFG_RC_OK)
        {
            return rv;
        }

        if (memcmp_b(buf, 0xFF, rem) != true)
        {
            /*LOGE("Sector failed to erase properly; off: 0x%04x", (uint32_t)off);*/
            return BOOTCFG_RC_ERR_ERASE;
        }

        off += rem;
        erasesize -= rem;
    }

    return BOOTCFG_RC_OK;
}

static int sector_erase(off_t off, ssize_t erasesize)
{
    int rv;

    LOGD("%s; 0x%08lx", __func__, off);

    rv = fd_erase(off);
    if (rv != BOOTCFG_RC_OK)
    {
        return rv;
    }

    rv = sector_verify_erase(off, erasesize);
    if (rv != BOOTCFG_RC_OK)
    {
        return rv;
    }

    return BOOTCFG_RC_OK;
}

static int read_log(off_t off, struct bootcfg_nand_log *log)
{
    int rv;

    rv = fd_read(off, log, sizeof(*log));
    if (rv != BOOTCFG_RC_OK)
    {
        return rv;
    }

    if (memcmp_b((void *)log, 0xFF, sizeof(*log)) == true)
    {
        /*LOGD("Log: 0x%08lx empty", off);*/
        return BOOTCFG_RC_ERR_LOG_EMPTY;
    }

    if (log->magic != BOOTCFG_MAGIC)
    {
        LOGE("Log magic incorrect: 0x%08lx, 0x%08x", off, log->magic);
        return BOOTCFG_RC_ERR_MAGIC;
    }

    if (calc_crc(log) != log->crc)
    {
        LOGE("Log checksum incorrect: 0x%08lx, 0x%08x, 0x%08x", off, calc_crc(log), log->crc);
        return BOOTCFG_RC_ERR_CHKSUM;
    }

    return BOOTCFG_RC_OK;
}

static int write_log(struct bootcfg_nand_log *log, off_t off, ssize_t pagesize)
{
    int rv;

    LOGD("%s; writing to 0x%08lx", __func__, off);

    rv = fd_write(off, log, pagesize);
    if (rv != BOOTCFG_RC_OK)
    {
        return rv;
    }

    if (verify_flash(off, (void *)log, sizeof(*log)) != BOOTCFG_RC_OK)
    {
        LOGE("Read-back verification failed");
        return BOOTCFG_RC_ERR_VERIFY;
    }

    return BOOTCFG_RC_OK;
}

static int bootcfg_write_log(struct bootcfg_info *info)
{
    int rv;
    struct bootcfg_nand_log *log = info->last_log;
    off_t off = info->off_next_log;
    int retries = info->part_size / info->part_erasesize; /* number of erase sectors */


    if (info->part_size == 0) {
        LOGE("Bootconfig partition does not exist; abort bootconfig");
        return BOOTCFG_RC_ERR;
    }

    log->magic = BOOTCFG_MAGIC;
    log->uid++;
    log->crc = calc_crc(log);

restart:
    if (retries-- == 0) {
        LOGE("Could not write log; max retries reached");
        return BOOTCFG_RC_ERR_WRITE;
    }

    LOGD("Trying to write log to off: 0x%08lx", off);

    if ((off % (off_t)info->part_erasesize) == 0)
    {
        /* we are about to write to the next sector; check if it is erased */

        if (off >= (off_t)info->part_size)
        {
            /* we are at the end of partition; go back to beginning */
            LOGI("Crossing over to the start of the partition");
            off = 0;
        }

        rv = sector_verify_erase(off, info->part_erasesize);
        if (rv != BOOTCFG_RC_OK)
        {
            LOGD("Sector is not erased; erasing; 0x%08lx", off);
            /* sector is not erased; erase it */
            rv = sector_erase(off, info->part_erasesize);
            if (rv < 0)
            {
                LOGD("Sector failed to erase; restarting; 0x%08lx", off);
                /* erasing/verification of erase failed; could be a bad block */
                /* go to next erase sector */
                off += info->part_erasesize;
                goto restart;
            }
        }
    }

    /* check that the part_writesize is erased */
    rv = sector_verify_erase(off, info->part_writesize);
    if (rv != BOOTCFG_RC_OK)
    {
        /* sector is not erased; go to next erase sector */
        LOGI("Write page is not erased; restarting; off: 0x%08lx", off);
        off = off - (off % (off_t)info->part_erasesize) + info->part_erasesize;
        goto restart;
    }

    rv = write_log(log, off, info->part_writesize);
    if (rv < 0)
    {
        /* writing failed; could be a bad block; try to repeat; go to next erase sector */
        LOGE("Writing log failed; restarting; off: 0x%08lx", off);
        off = off - (off % (off_t)info->part_erasesize) + info->part_erasesize;
        goto restart;
    }

    /* write success; store pointer to the next log */
    info->off_next_log = off + info->part_writesize;

    return BOOTCFG_RC_OK;
}



/**
 * Open the interface to the bootconfig, retrieve info about flash partition
 * size and geometry.
 */
int bootcfg_open(void)
{
    LOGD("%s", __func__);

    get_nand_flash_info(&bc_info, 2/*misc3*/);

    memset(bc_info.log_buf, 0xFF, sizeof(bc_info.log_buf));
    bc_info.last_log = (struct bootcfg_nand_log *)bc_info.log_buf;

    return BOOTCFG_RC_OK;
}

/**
 * Reset the bootconfig info for both images. Typically called after
 * TFTP recovery.
 */
int bootcfg_write_log_reset(void)
{
    int rv;

    LOGD("%s", __func__);

    bc_info.last_log->img[0].skip = 0xFFFFFFFFUL;
    bc_info.last_log->img[1].skip = 0xFFFFFFFFUL;
    bc_info.last_log->img[0].ignore_skip = 0xFFFFFFFFUL;
    bc_info.last_log->img[1].ignore_skip = 0xFFFFFFFFUL;
    bc_info.last_log->img[0].cnt_current = 0;
    bc_info.last_log->img[1].cnt_current = 0;
    bc_info.last_log->img[0].cnt_all_success = 0;
    bc_info.last_log->img[1].cnt_all_success = 0;

    rv = bootcfg_write_log(&bc_info);
    if (rv < 0)
    {
        LOGE("Could not write log");
        return rv;
    }

    return BOOTCFG_RC_OK;
}

/**
 * Write a new bootconfig log with source of boot set as bootloader.
 */
int bootcfg_write_log_btldr(enum bootcfg_location location)
{
    int rv;
    int loc;

    LOGD("%s", __func__);

    loc = location_to_index(location);
    if (loc == -1)
    {
        return BOOTCFG_RC_ERR;
    }

    bc_info.last_log->cnt_all_boots++;
    bc_info.last_log->img[loc].cnt_current++;

    if (bc_info.last_log->img[loc].cnt_current >= bc_info.last_log->img[loc].cnt_threshold)
    {
        LOGI("WARNING: bootcfg bad count threshold reached");
        bc_info.last_log->img[loc].skip = 0x00000000UL;
    }

    rv = bootcfg_write_log(&bc_info);
    if (rv < 0)
    {
        LOGE("Could not write log");
        return rv;
    }

    return BOOTCFG_RC_OK;
}

/**
 * Write a new bootconfig log with 'skip image' flag set for specified image location.
 */
#if 0
int bootcfg_set_skip_img(enum bootcfg_location location)
{
    int rv;
    int loc;

    LOGD("%s", __func__);

    loc = location_to_index(location);
    if (loc == -1)
    {
        return BOOTCFG_RC_ERR;
    }

    bc_info.last_log->img[loc].skip = 0x00000000UL;

    rv = bootcfg_write_log(&bc_info);
    if (rv < 0)
    {
        LOGE("Could not write log");
        return rv;
    }

    return BOOTCFG_RC_OK;
}
#endif

/**
 * Write a new bootconfig log with 'ignore skip image' flag set for specified image location.
 */
int bootcfg_set_ignore_skip_img(enum bootcfg_location location)
{
    int rv;
    int loc;

    LOGD("%s", __func__);

    loc = location_to_index(location);
    if (loc == -1)
    {
        return BOOTCFG_RC_ERR;
    }

    bc_info.last_log->img[loc].ignore_skip = 0x00000000UL;

    rv = bootcfg_write_log(&bc_info);
    if (rv < 0)
    {
        LOGE("Could not write log");
        return rv;
    }

    return BOOTCFG_RC_OK;
}

/**
 * Find and return the latest boot log
 */
int bootcfg_get_last_log(struct bootcfg_nand_log *log)
{
    int rv;
    off_t off;
    struct bootcfg_nand_log tmp;

    LOGD("%s", __func__);

    off_t off_last_log = 0;
    uint64_t last_log_uid = 0;
    bool is_empty = true;

    if (bc_info.part_size == 0) {
        LOGE("Bootconfig partition does not exist; abort bootconfig");
        return BOOTCFG_RC_ERR;
    }

    off = 0;
    while (off < bc_info.part_size)
    {
        rv = read_log(off, &tmp);
        if (rv == BOOTCFG_RC_OK)
        {
            if (tmp.uid > last_log_uid)
            {
                /* ok, store current offset, and uid */
                last_log_uid = tmp.uid;
                off_last_log = off;
            }

            off += bc_info.part_writesize;
            is_empty = false;
        }
        else if ((rv == BOOTCFG_RC_ERR_CHKSUM) || (rv == BOOTCFG_RC_ERR_MAGIC))
        {
            /* checksum or magic is wrong */
            off += bc_info.part_writesize;
            is_empty = false;
        }
        else if (rv == BOOTCFG_RC_ERR_LOG_EMPTY)
        {
            /* skip to the beginning of the next sector */
            off = off - (off % bc_info.part_erasesize) + bc_info.part_erasesize;
        }
        else
        {
            /* unknown error; abort */
            return rv;
        }
    }

    rv = read_log(off_last_log, bc_info.last_log);
    if ((rv != BOOTCFG_RC_OK) || (is_empty == true))
    {
        LOGD("Log is empty, set defaults");

        bc_info.last_log->magic = BOOTCFG_MAGIC;
        bc_info.last_log->uid = 0;
        bc_info.last_log->cnt_all_boots = 0;
        bc_info.last_log->cnt_all_success = 0;

        bc_info.last_log->img[0].cnt_current = 0;
        bc_info.last_log->img[0].cnt_all_success = 0;
        bc_info.last_log->img[0].cnt_threshold = BOOTCFG_BADCOUNT_THRESHOLD;
        bc_info.last_log->img[0].skip = 0xFFFFFFFFUL;
        bc_info.last_log->img[0].ignore_skip = 0xFFFFFFFFUL;

        bc_info.last_log->img[1].cnt_current = 0;
        bc_info.last_log->img[1].cnt_all_success = 0;
        bc_info.last_log->img[1].cnt_threshold = BOOTCFG_BADCOUNT_THRESHOLD;
        bc_info.last_log->img[1].skip = 0xFFFFFFFFUL;
        bc_info.last_log->img[1].ignore_skip = 0xFFFFFFFFUL;

        bc_info.off_next_log = 0;

        rv = BOOTCFG_RC_ERR_LOG_EMPTY;
    }
    else {
        bc_info.off_next_log = off_last_log + bc_info.part_writesize;
    }

    if (log != NULL) {
        memcpy(log, bc_info.last_log, sizeof(*log));
    }

    return rv;
}


void bootcfg_dump_log(struct bootcfg_nand_log *log)
{
    LOGI("Bootconfig last log: uid:%llu; all:%u; all_success:%u",
        log->uid, log->cnt_all_boots, log->cnt_all_success);
    LOGI("Img1 curr:%u; all:%u; thrld:%u; skip:0x%x; ign_skip:0x%x",
        log->img[0].cnt_current, log->img[0].cnt_all_success, log->img[0].cnt_threshold,
        log->img[0].skip, log->img[0].ignore_skip);
    LOGI("Img2 curr:%u; all:%u; thrld:%u; skip:0x%x; ign_skip:0x%x",
        log->img[1].cnt_current, log->img[1].cnt_all_success, log->img[1].cnt_threshold,
        log->img[1].skip, log->img[1].ignore_skip);
}
