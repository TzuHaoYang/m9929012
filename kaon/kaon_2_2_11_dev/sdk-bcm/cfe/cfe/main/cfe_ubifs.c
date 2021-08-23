/*  cfe_ubifs - simple parser for UBI + UBIFS partitions
 *
 *  Copyright (C) 2018  Plume Design Inc.
 */

#include "bcm63xx_util.h"
#include "bcm63xx_auth.h"
#include "flash_api.h"
#include "shared_utils.h"
#include "lib_math.h"
#include "lib_byteorder.h"
#include "lib_malloc.h"
#include "lib_crc.h"
#include "rom_parms.h"

#include <stdbool.h>
#include <endian.h>
#include "cfe_ubifs.h"




/* The version of UBI images supported by this implementation */
#define UBI_VERSION 1

/* The highest erase counter value supported by this implementation */
#define UBI_MAX_ERASECOUNTER 0x7FFFFFFF

/* The initial CRC32 value used when calculating CRC checksums */
#define UBI_CRC32_INIT 0xFFFFFFFFU

/* Erase counter header magic number (ASCII "UBI#") */
#define UBI_EC_HDR_MAGIC  0x55424923
/* Volume identifier header magic number (ASCII "UBI!") */
#define UBI_VID_HDR_MAGIC 0x55424921


/* Sizes of UBI headers */
#define UBI_EC_HDR_SIZE  sizeof(struct ubi_ec_hdr)
#define UBI_VID_HDR_SIZE sizeof(struct ubi_vid_hdr)

/* Sizes of UBI headers without the ending CRC */
#define UBI_EC_HDR_SIZE_CRC  (UBI_EC_HDR_SIZE  - sizeof(uint32_t))
#define UBI_VID_HDR_SIZE_CRC (UBI_VID_HDR_SIZE - sizeof(uint32_t))


/**
 * struct ubi_ec_hdr - UBI erase counter header.
 */
struct ubi_ec_hdr {
    uint32_t magic;
    uint8_t  version;
    uint8_t  padding1[3];
    uint64_t ec; /* Warning: the current limit is 31-bit anyway! */
    uint32_t vid_hdr_offset;
    uint32_t data_offset;
    uint32_t image_seq;
    uint8_t  padding2[32];
    uint32_t hdr_crc;
} __attribute__((packed));

/**
 * struct ubi_vid_hdr - on-flash UBI volume identifier header.
 */
struct ubi_vid_hdr {
    uint32_t magic;
    uint8_t  version;
    uint8_t  vol_type;
    uint8_t  copy_flag;
    uint8_t  compat;
    uint32_t vol_id;
    uint32_t lnum;
    uint8_t  padding1[4];
    uint32_t data_size;
    uint32_t used_ebs;
    uint32_t data_pad;
    uint32_t data_crc;
    uint8_t  padding2[4];
    uint64_t sqnum;
    uint8_t  padding3[12];
    uint32_t hdr_crc;
} __attribute__((packed));



#define UBIFS_NODE_MAGIC  0x06101831

/* Initial CRC32 value used when calculating CRC checksums */
#define UBIFS_CRC32_INIT 0xFFFFFFFFU

#define UBIFS_FORMAT_VERSION 4

/* Maximum possible key length */
#define UBIFS_MAX_KEY_LEN 16
/*
 * Maximum file name and extended attribute length (must be a multiple of 8,
 * minus 1).
 */
#define UBIFS_MAX_NLEN 255

#define UBIFS_CH_SZ        sizeof(struct ubifs_ch)
#define UBIFS_DENT_NODE_SZ sizeof(struct ubifs_dent_node)
#define UBIFS_MAX_DENT_NODE_SZ  (UBIFS_DENT_NODE_SZ + UBIFS_MAX_NLEN + 1)

enum {
    UBIFS_INO_NODE,
    UBIFS_DATA_NODE,
    UBIFS_DENT_NODE,
    UBIFS_XENT_NODE,
    UBIFS_TRUN_NODE,
    UBIFS_PAD_NODE,
    UBIFS_SB_NODE,
    UBIFS_MST_NODE,
    UBIFS_REF_NODE,
    UBIFS_IDX_NODE,
    UBIFS_CS_NODE,
    UBIFS_ORPH_NODE,
    UBIFS_NODE_TYPES_CNT,
};

/**
 * struct ubifs_ch - common header node.
 */
struct ubifs_ch {
    uint32_t magic;
    uint32_t crc;
    uint64_t sqnum;
    uint32_t len;
    uint8_t  node_type;
    uint8_t  group_type;
    uint8_t  padding[2];
} __attribute__((packed));

/**
 * struct ubifs_dent_node - directory entry node.
 */
struct ubifs_dent_node {
    struct ubifs_ch ch;
    uint8_t key[UBIFS_MAX_KEY_LEN];
    uint64_t inum;
    uint8_t padding1;
    uint8_t type;
    uint16_t nlen;
    uint8_t padding2[4];
    uint8_t name[];
} __attribute__((packed));





static void get_sector_from_offset(const struct part_info *info,
        off_t off, unsigned short *sector, int *sector_offset)
{
    off += info->part_off;

    *sector = off / info->part_erasesize;
    *sector_offset = off % info->part_erasesize;
}

static int fd_read(const struct part_info *info, off_t off, void *data, ssize_t len)
{
    int rv;
    unsigned short sector;
    int sector_offset;

    get_sector_from_offset(info, off, &sector, &sector_offset);

    rv = flash_read_buf(sector, sector_offset, data, len);
    if (rv != len)
    {
        return -1;
    }

    return len;
}

static uint32_t calc_crc(uint32_t init_crc, void *data, uint32_t len)
{
    return lib_get_crc32(data, len, init_crc);
}


static void conv_ubi_ec(struct ubi_ec_hdr *hdr)
{
    hdr->magic = be32toh(hdr->magic);
    /*hdr->version = hdr->version;*/
    hdr->ec = be64toh(hdr->ec);
    hdr->vid_hdr_offset = be32toh(hdr->vid_hdr_offset);
    hdr->data_offset = be32toh(hdr->data_offset);
    hdr->image_seq = be32toh(hdr->image_seq);
    hdr->hdr_crc = be32toh(hdr->hdr_crc);
}

static void conv_ubi_vid(struct ubi_vid_hdr *hdr)
{
     hdr->magic = be32toh(hdr->magic);
     /*hdr->version = hdr->version;*/
     /*hdr->vol_type = hdr->vol_type;*/
     /*hdr->copy_flag = hdr->copy_flag;*/
     /*hdr->compat = hdr->compat;*/
     hdr->vol_id = be32toh(hdr->vol_id);
     hdr->lnum = be32toh(hdr->lnum);
     hdr->data_size = be32toh(hdr->data_size);
     hdr->used_ebs = be32toh(hdr->used_ebs);
     hdr->data_pad = be32toh(hdr->data_pad);
     hdr->data_crc = be32toh(hdr->data_crc);
     hdr->sqnum = be64toh(hdr->sqnum);
     hdr->hdr_crc = be32toh(hdr->hdr_crc);
}

static void conv_ubifs_node(struct ubifs_ch *hdr)
{
    hdr->magic = le32toh(hdr->magic);
    hdr->crc = le32toh(hdr->crc);
    hdr->sqnum = le64toh(hdr->sqnum);
    hdr->len = le32toh(hdr->len);
    /*hdr->node_type = hdr->node_type;*/
    /*hdr->group_type = hdr->group_type;*/
}

#if 0
static void print_ubi_ec(struct ubi_ec_hdr *hdr)
{
    printf("UBI EC: magic:0x%08x, version:0x%02x, crc:0x%08x\n",
        hdr->magic, hdr->version, hdr->hdr_crc);
    printf("  ec:0x%08llx, vid_hdr_offset:0x%08x data_off:0x%08x, image_seq:0x%08x\n",
        hdr->ec, hdr->vid_hdr_offset, hdr->data_offset, hdr->image_seq);
    printf("\n");
}
static void print_ubi_vid(struct ubi_vid_hdr *hdr)
{
    printf("    UBI VID: magic:0x%08x, version:0x%02x, vol_type:0x%02x, volid:0x%08x, crc:0x%08x\n",
        hdr->magic, hdr->version, hdr->vol_type, hdr->vol_id, hdr->hdr_crc);
    printf("      lnum:0x%08x, data_size:0x%08x, used_ebs:0x%08x, data_pad:0x%08x\n",
        hdr->lnum, hdr->data_size, hdr->used_ebs, hdr->data_pad);
    printf("      data_crc:0x%08x, sqnum:0x%08llx\n",
        hdr->data_crc, hdr->sqnum);
    printf("\n");
}
#endif
#if 0
static void print_ubifs_node(struct ubifs_ch *node)
{
    printf("      UBIFS node: magic:0x%08x, sqnum:0x%08llx, len:0x%08x, crc:0x%08x\n",
        node->magic, node->sqnum, node->len, node->crc);
    printf("        node_type:%u, group_type:%u\n",
        node->node_type, node->group_type);
    printf("\n");
}
#endif


static int get_ubi_ec(const struct part_info *info, off_t off, struct ubi_ec_hdr *ubi_ec)
{
    int rv;
    uint32_t crc;

    rv = fd_read(info, off, ubi_ec, UBI_EC_HDR_SIZE);
    if (rv < 0)
    {
        return -1;
    }

    crc = calc_crc(UBI_CRC32_INIT, ubi_ec, UBI_EC_HDR_SIZE_CRC);
    conv_ubi_ec(ubi_ec);

    if (!((ubi_ec->magic == UBI_EC_HDR_MAGIC) && (ubi_ec->hdr_crc == crc)))
    {
        return -1;
    }

    return 0;
}

static int get_ubi_vid(const struct part_info *info, off_t off, struct ubi_vid_hdr *ubi_vid)
{
    int rv;
    uint32_t crc;

    rv = fd_read(info, off, ubi_vid, UBI_VID_HDR_SIZE);
    if (rv < 0)
    {
        return -1;
    }

    crc = calc_crc(UBI_CRC32_INIT, ubi_vid, UBI_VID_HDR_SIZE_CRC);
    conv_ubi_vid(ubi_vid);

    if (!((ubi_vid->magic == UBI_VID_HDR_MAGIC) && (ubi_vid->hdr_crc == crc)))
    {
        return -1;
    }

    return 0;
}

static int get_ubifs_node(const struct part_info *info, off_t off, struct ubifs_ch *ubifs_node, uint32_t maxlen)
{
    int rv;
    uint32_t crc;
    uint32_t len;

    rv = fd_read(info, off, ubifs_node, sizeof(struct ubifs_ch));
    if (rv < 0)
    {
        return -1;
    }

    conv_ubifs_node(ubifs_node);

    if (!(ubifs_node->magic == UBIFS_NODE_MAGIC))
    {
        return -1;
    }
    if ((ubifs_node->node_type < 0) || (ubifs_node->node_type >= UBIFS_NODE_TYPES_CNT))
    {
        return -1;
    }

    len = ubifs_node->len;
    if (len > maxlen)
    {
        return -1;
    }
    if ((len + (off % info->part_erasesize)) > info->part_erasesize)
    {
        return -1;
    }

    rv = fd_read(info, off, ubifs_node, len);
    if (rv < 0)
    {
        return -1;
    }

    /* "+ 8" to calculate CRC over the header without magic and CRC field */
    crc = calc_crc(UBIFS_CRC32_INIT, ((uint8_t *)ubifs_node)+8, len-8);
    if (crc != ubifs_node->crc)
    {
        return -1;
    }

    return 0;
}


bool ubifs_file_exists(const struct part_info *part_info, const char *filename)
{
    off_t off_block;
    off_t off_page;
    bool exists = false;
    int rv;

    struct ubi_ec_hdr ubi_ec;
    struct ubi_vid_hdr ubi_vid;

    uint8_t buf[UBIFS_MAX_DENT_NODE_SZ];
    struct ubifs_ch *ubifs_node = (struct ubifs_ch *)buf;
    struct ubifs_dent_node *ubifs_dent = (struct ubifs_dent_node *)buf;

    uint64_t sqnum = 0;

    off_block = 0;
    while (off_block < part_info->part_size)
    {
        /* get the UBI Erase Counter header */
        rv = get_ubi_ec(part_info, off_block, &ubi_ec);
        if (rv != 0)
        {
            off_block += part_info->part_writesize;
            continue;
        }
        /*print_ubi_ec(&ubi_ec);*/

        off_page = off_block + part_info->part_writesize;

        /* get the UBI VID header, which is in the same erase block, on the next page */
        rv = get_ubi_vid(part_info, off_page, &ubi_vid);
        if (rv != 0)
        {
            off_block += part_info->part_writesize;
            continue;
        }
        /*print_ubi_vid(&ubi_vid);*/


        /* on subsequent pages (after UBI VID header), there should be UBIFS nodes */
        off_page += part_info->part_writesize;
        while (off_page < (off_block + part_info->part_erasesize))
        {
            rv = get_ubifs_node(part_info, off_page, ubifs_node, sizeof(buf));
            if (rv != 0)
            {
                off_page += part_info->part_writesize;
                continue;
            }


            if (ubifs_node->node_type == UBIFS_DENT_NODE)
            {
                if (strncmp((char *)ubifs_dent->name, filename, ubifs_dent->nlen) == 0)
                {
                    /* found dir entry node with correct filename;
                     * save the highest sequence number and
                     * state if file is present, or deleted */
                    if (ubifs_node->sqnum > sqnum)
                    {
                        sqnum = ubifs_node->sqnum;

                        if (ubifs_dent->inum == 0x0)
                        {
                            exists = false;
                        }
                        else
                        {
                            exists = true;
                        }
                    }
                }
            }

            off_page += part_info->part_writesize;
        }

        off_block += part_info->part_erasesize;
    }

    return exists;
}
