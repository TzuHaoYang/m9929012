#!/bin/bash

firmware_file=`find $(pwd)/sdk-bcm/images -name "bcmOS_EXTENDER_BCM52_nand_fs_image_128_puresqubi_5.02L.07p1-*.w"`

if [ -z $1 ]; then
echo " $0 e/d"
exit
fi

if [ $1 = "e" ]; then
	`echo $(pwd)/firmware_encrypt -e $firmware_file $(pwd)/sdk-bcm/images/bcmOS_EXTENDER_BCM52_nand_fs_image_128_puresqubi_5.02L.07p1-Encrypt.w`
exit
fi

if [ $1 = "d" ]; then
	`echo $(pwd)/firmware_encrypt -d $(pwd)/sdk-bcm/images/bcmOS_EXTENDER_BCM52_nand_fs_image_128_puresqubi_5.02L.07p1-Encrypt.w $(pwd)/sdk-bcm/images/bcmOS_EXTENDER_BCM52_nand_fs_image_128_puresqubi_5.02L.07p1-Decrypt.w`
exit
fi
