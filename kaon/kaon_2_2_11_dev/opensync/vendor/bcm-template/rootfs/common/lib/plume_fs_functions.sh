#!/bin/sh


get_part_mtd()
{
    cat /proc/mtd | grep '"'$1'"' | cut -d':' -f1
}

get_ubi_dev()
{
    MTD=${1}
    MTD=${MTD/mtd/}; # replace "mtd" with nothing

    UBI=`grep -l $MTD /sys/class/ubi/*/mtd_num`
    UBI=${UBI/\/mtd_num/}; # remove "/mtd_num" from the path
    UBI=${UBI/\/sys\/class\/ubi\//}; # remove "/sys/class/ubi/" from the path
    DEV=/dev/$UBI # mdev should already populate the new device node for ubi in /dev.

    echo "${DEV}"
}


get_ubi_sub_dev()
{
    UBI_DEV=${1}
    SUB_NAME=${2}

    UBI_DEV=${UBI_DEV/\/dev\//}; # replace "mtd" with nothing

    UBI=`grep -l $SUB_NAME /sys/class/ubi/$UBI_DEV/$UBI_DEV_*/name`
    UBI=${UBI/\/name/}; # remove "/mtd_num" from the path
    UBI=${UBI/\/sys\/class\/ubi\//}; # remove "/sys/class/ubi/" from the path
    UBI=${UBI/\/$UBI_DEV/}; # remove "$UBI_DEV" from the path
    DEV=/dev/$UBI

    echo "${DEV}"
}

ubi_attach()
{
    # param1: MTD name (example: "common")
    # param2 optional: "format" to format the partition as UBI if not already; default is no format

    MTD_NAME=${1}
    FORMAT=${2:-false}

    MTD=$(get_part_mtd $MTD_NAME)

    # Try attaching UBI
    ubiattach -p /dev/$MTD > /dev/null 2>&1
    if [ $? -ne 0 ]
    then
        if [ "${FORMAT}" != "format" ];
        then
            logger "ERROR: Attaching /dev/$MTD as UBI failed"
            return 1
        fi

        logger "Formatting partition /dev/$MTD as UBI"
        ubiformat /dev/$MTD -q -y > /dev/null 2>&1
        if [ $? -ne 0 ];
        then
            logger "ERROR: Formatting /dev/$MTD as UBI failed"
            return 1
        fi

        # Format succeded, try attaching again
        ubiattach -p /dev/$MTD > /dev/null 2>&1
        if [ $? -ne 0 ];
        then
            logger "ERROR: Attaching /dev/$MTD as UBI failed"
            return 1
        fi
    fi

    return 0
}

ubi_detach()
{
    # param1: MTD name (example: "common")

    MTD_NAME=${1}

    MTD=$(get_part_mtd $MTD_NAME)
    ubidetach -p /dev/$MTD > /dev/null 2>&1
    if [ $? -ne 0 ]
    then
        return 1
    fi
    return 0
}

ubi_is_attached()
{
    # param1: MTD name (example: "common")

    MTD_NAME=${1}

    MTD=$(get_part_mtd $MTD_NAME)

    for mtd_num in $(cat /sys/class/ubi/ubi*/mtd_num);
    do
        if [ "$MTD" == "mtd$mtd_num" ];
        then
            return 0
        fi
    done

    return 1
}

ubifs_mkvol()
{
    # param1: MTD name (example: "common")
    # param2: UBIFS subvolume name (example: "common_config")
    # param3: UBIFS subvolume size (example: "1MiB")

    MTD_NAME=${1}
    VOL_NAME=${2}
    VOL_SIZE=${3}

    MTD=$(get_part_mtd $MTD_NAME)
    UBIDEV=$(get_ubi_dev $MTD)

    logger "Creating UBIFS subvolume $VOL_NAME"
    ubimkvol $UBIDEV --name=$VOL_NAME --type=dynamic --size=$VOL_SIZE > /dev/null 2>&1
    if [ $? -ne 0 ]
    then
        logger "ERROR: Creating UBIFS subvolume $VOL_NAME failed"
        return 1
    fi

    return 0
}

ubifs_vol_exists()
{
    # param1: MTD name (example: "common")
    # param2: UBIFS subvolume name (example: "common_config")

    MTD_NAME=${1}
    VOL_NAME=${2}

    MTD=$(get_part_mtd $MTD_NAME)
    UBIDEV=$(get_ubi_dev $MTD)

    UBINUM=${UBIDEV/\/dev\//}
    for name in $(cat /sys/class/ubi/$UBINUM_*/name);
    do
        if [ "$name" == "$VOL_NAME" ];
        then
            return 0
        fi
    done

    return 1
}

ubifs_mount()
{
    # param1: MTD name (example: "common")
    # param2: UBIFS subvolume name (example: "common_config")
    # param3: path where to mount at (example: "/mnt/data/config")
    # param4: optional: mount as Read-Write or Read-Only; (example "rw" or "ro", defaults to "ro")

    MTD_NAME=${1}
    VOL_NAME=${2}
    MNT_PATH=${3}
    MNT_RW=${4:-ro}

    MTD=$(get_part_mtd $MTD_NAME)
    UBIDEV=$(get_ubi_dev $MTD)
    UBISUBDEV=$(get_ubi_sub_dev $UBIDEV $VOL_NAME)

    if [ "$MNT_RW" == "rw" ];
    then
        OPTS=""
    else
        OPTS="-o ro"
    fi


    mkdir -p "$MNT_PATH"
    mount -t ubifs $OPTS "$UBISUBDEV" "$MNT_PATH" > /dev/null 2>&1
    if [ $? -ne 0 ];
    then
        logger "ERROR: Could not mount UBIFS subvolume $VOL_NAME"
        return 1
    fi

    return 0
}

ubifs_umount()
{
    # param1: MTD name (example: "common")
    # param2: UBI subvolume name (example: "common_config")

    MTD_NAME=${1}
    VOL_NAME=${2}

    MTD=$(get_part_mtd $MTD_NAME)
    UBIDEV=$(get_ubi_dev $MTD)
    UBISUBDEV=$(get_ubi_sub_dev $UBIDEV $VOL_NAME)

    umount "${UBISUBDEV}"
    if [ $? -ne 0 ];
    then
        logger "ERROR: Could not umount UBIFS subvolume $VOL_NAME"
        return 1
    fi

    return 0
}

squashfs_mount()
{
    # param1: MTD name (example: "rootfs")
    # param2: UBI subvolume name (example: "ubi_rootfs")
    # param3: path where to mount at (example "/mnt/alt")

    MTD_NAME=${1}
    VOL_NAME=${2}
    MNT_PATH=${3}


    MTD=$(get_part_mtd $MTD_NAME)
    UBIDEV=$(get_ubi_dev $MTD)
    UBISUBDEV=$(get_ubi_sub_dev $UBIDEV $VOL_NAME)
    UBIBLOCKDEV=${UBISUBDEV/ubi/ubiblock}

    ubiblock --create "$UBISUBDEV"
    if [ $? -ne 0 ];
    then
        logger "ERROR: Could not create UBI block on $UBISUBDEV"
        return 1
    fi

    mkdir -p "$MNT_PATH"
    mount -t squashfs -o ro "$UBIBLOCKDEV" "$MNT_PATH"
    if [ $? -ne 0 ];
    then
        logger "ERROR: Could not mount squashfs $UBIBLOCKDEV"
        return 1
    fi

    return 0
}

squashfs_umount()
{
    # param1: MTD name (example: "rootfs")
    # param2: UBI subvolume name (example: "ubi_rootfs")

    MTD_NAME=${1}
    VOL_NAME=${2}

    MTD=$(get_part_mtd $MTD_NAME)
    UBIDEV=$(get_ubi_dev $MTD)
    UBISUBDEV=$(get_ubi_sub_dev $UBIDEV $VOL_NAME)
    UBIBLOCKDEV=${UBISUBDEV/ubi/ubiblock}

    umount "${UBIBLOCKDEV}"
    if [ $? -ne 0 ];
    then
        logger "ERROR: Could not umount UBIFS subvolume $VOL_NAME"
        return 1
    fi

    return 0
}

part_is_mounted()
{
    # param1: path where partition should be mounted under (example "/mnt/data/config")

    # NOTE: Can also be done with get_ubi_sub_dev()

    MNT_PATH=${1}

    cat /proc/mounts | awk '{ print $2}' | grep "$MNT_PATH" > /dev/null 2>&1
    if [ $? -eq 0 ];
    then
        return 0
    fi

    return 1
}

part_remount_ro()
{
    # param1: mount point to remount as Read-Only
    MNT_PATH="$1"

    mount -o remount,ro "$MNT_PATH"
}

part_remount_rw()
{
    # param1: mount point to remount as Read-Write
    MNT_PATH="$1"

    mount -o remount,rw "$MNT_PATH"
}
