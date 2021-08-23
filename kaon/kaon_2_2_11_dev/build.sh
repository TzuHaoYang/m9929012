#!/bin/bash

FW_NUM=$1

if [ ${#FW_NUM} -eq 4 ];then
	echo "build number is $FW_NUM"
else
	FW_NUM=0000
	echo "build number is $FW_NUM"
fi

rm -rf sdk-bcm/images

export PLUME_DIR=$(pwd)/..
export OPENSYNC_ROOT=$(pwd)/opensync
export SDK_ROOT=$(pwd)/sdk-bcm
echo $PLUME_DIR
echo $OPENSYNC_ROOT
echo $PLUME_SRC
export PLUME_SRC=$OPENSYNC_ROOT
echo $PLUME_SRC
ls
cd sdk-bcm/
ls
make PLUME_SRC=$OPENSYNC_ROOT PROFILE=OS_EXTENDER_BCM52 BACKHAUL_PASS=welcome8 BACKHAUL_SSID=we.piranha BUILD_NUMBER=$FW_NUM

cd ..
echo `./firmware_enc.sh e`
