#!/bin/sh

[ $# -gt 0 ] && IMG_FILE="$1" || IMG_FILE=/tmp/fw.w

if [ ! -f "$IMG_FILE" ]; then
    echo "$IMG_FILE missing"
    exit 1
fi

if bcm_flasher "$IMG_FILE" && bcm_bootstate BOOT_SET_NEW_IMAGE; then
    sync
    reboot
else
    exit 1
fi
