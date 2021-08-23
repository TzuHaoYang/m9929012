#!/bin/sh

. /lib/plume_fs_functions.sh

plume_factory_mode()
{
    if [ -e /tmp/factory ]; then
        return 0
    fi

    return 1
}

plume_development_mode()
{
    if [ -e /data/plume_development ]; then
        return 0
    fi

    return 1
}

mount_inactive_rootfs()
{
    MNT_DIR="$1"

    ubi_attach "rootfs_update"
    if [ $? -ne 0 ]; then
        return 1
    fi

    squashfs_mount "rootfs_update" "rootfs_ubifs" "$MNT_DIR"
    if [ $? -ne 0 ]; then
        return 1
    fi

    return 0
}

umount_inactive_rootfs()
{
    MNT_DIR="$1"

    squashfs_umount "rootfs_update" "rootfs_ubifs"
    if [ $? -ne 0 ]; then
        return 1
    fi

    ubi_detach "rootfs_update"
    if [ $? -ne 0 ]; then
        return 1
    fi

    return 0
}

mount_inactive_overlay()
{
    MNT_DIR="$1"

    ubi_attach "rootfs_update"
    if [ $? -ne 0 ]; then
        return 1
    fi

    ubifs_mount "rootfs_update" "rootfs_overlay" "$MNT_DIR"
    if [ $? -ne 0 ]; then
        return 1
    fi

    return 0
}

umount_inactive_overlay()
{
    MNT_DIR="$1"

    ubifs_umount "rootfs_update" "rootfs_overlay" "$MNT_DIR"
    if [ $? -ne 0 ]; then
        return 1
    fi

    ubi_detach "rootfs_update"
    if [ $? -ne 0 ]; then
        return 1
    fi

    return 0
}
