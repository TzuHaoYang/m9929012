#include "bcm63xx_util.h"
#include "bcm63xx_auth.h"
#include "flash_api.h"
#include "shared_utils.h"
#include "lib_math.h"
#include "bcm_otp.h"
#include "bcm63xx_blparms.h"
#include "btrm_if.h"
#include "lib_byteorder.h"
#include "lib_malloc.h"
#include "rom_parms.h"
#include "bcm63xx_nvram.h"

#include "cfe_ubifs.h"


static void calc_extra_part_layout(struct part_info *info, int parts_num)
{
    int i;
    ssize_t extra = 0;

    info[parts_num-1].part_off = NVRAM.ulNandPartOfsKb[NP_DATA] * 1024;
    info[parts_num-1].part_size = NVRAM.ulNandPartSizeKb[NP_DATA] * 1024;

    /* skip the /data partition */
    for (i = BCM_MAX_EXTRA_PARTITIONS - 2; i >= 0; i--)
    {
        ssize_t extra_part_size = convert_to_data_partition_entry_to_bytes(NVRAM.part_info[i].size);

        extra += extra_part_size;
        info[i].part_size = extra_part_size;
        info[i].part_off = info[parts_num-1].part_off - extra;
    }
}

static void get_extra_part_info(struct part_info *info, uint32_t part_idx)
{
    struct part_info extra_parts[BCM_MAX_EXTRA_PARTITIONS];

    info->part_erasesize = flash_get_sector_size(0);
    /* TODO: how do we get this programmatically */
    info->part_writesize = 2048;

    calc_extra_part_layout(extra_parts, BCM_MAX_EXTRA_PARTITIONS);

    info->part_size = extra_parts[part_idx].part_size;
    info->part_off = extra_parts[part_idx].part_off;
}


bool extra_part_file_exists(int extra_part, const char *filename)
{
    struct part_info part;

    get_extra_part_info(&part, extra_part);

    return ubifs_file_exists(&part, filename);
}
