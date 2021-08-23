#!/bin/bash
set -e
#***************************************************************************
#Name : build_834_5.sh
#Description : This is the master script to build openwrt image for 834_5
#with opensync
#***************************************************************************
BUILD_TYPE=${1}
OPTION=${2}
DEVICE_TYPE=${3}

ROOT_PATH=${PWD}
BUILD_DIR=${ROOT_PATH}/openwrt
VENDOR_ADTRAN_BRANCH="dev"
VENDOR_ADTRAN_REPO="https://bitbucket.org/smartrg/device-vendor-adtran.git"
PY_VERSION=""
REQ_PY_VER="3.6"
OVERRIDE_PYTHON_VER="3.8"
CUR_PY_VER=$(python3 --version | awk '{print $2}' | cut -c1-3)
IMG_PATH_834_5="openwrt/bin/targets/mediatek/mt7622/"
IMG_NAME_834_5="openwrt-mediatek-mt7622-smartrg_sr402ac-squashfs-sysupgrade-emmc.bin.gz"
IMG_VERSION="0.0"
YEAR=`date +%Y`
MONTH=`date +%m`
YEAR_IN_YY_FMT=${YEAR: -2}
VERSION_METAFILE="version_metafile"
WLAN_AP_COMMIT_ID=""
VENDOR_ADTRAN_COMMIT_ID=""
WLAN_AP_SHORT_SHA=""
VNDR_ADTN_SHORT_SHA=""
BUILD_IMAGE_NAME=""

if [ -s $VERSION_METAFILE ]; then
     source $VERSION_METAFILE
else
     echo "$VERSION_METAFILE is not present. Exit!!"
     exit 1;
fi

#***************************************************************************
#Usage(): This function describe the usage of build_834_5.sh
#***************************************************************************
Usage()
{
   echo ""
   echo "build_834_5.sh : This is the master script to build openwrt image for 834_5 with opensync"
   echo "Syntax: ./build_834_5.sh [PROD|ENGR] [build|rebuild|help]"
   echo ""
   echo "Prerequisite:"
   echo "-- The python version should be 3.6 or greater to avoid python script failures"
   echo "-- After cloning of wlan-ap repo execute git credential cache command, to cache GIT credentials"
   echo "   git config credential.helper 'cache --timeout=1800'"
   echo ""
   echo "options:"
   echo "PROD     To generate a production image"
   echo "ENGR     To generate a development image"
   echo "build    setup, generate config, patch files and builds the image for 834-5"
   echo "rebuild  re-compile and build 834_5 image"
   echo "help     help list"
   echo ""
   exit 1
}
#***************************************************************************
#isPythonVerValid(): This function is to validate the required python version
#***************************************************************************
isPythonVerValid()
{
   REQ_MJR_PY_VER=$(echo $REQ_PY_VER | cut -d'.' -f1)
   REQ_MIN_PY_VER=$(echo $REQ_PY_VER | cut -d'.' -f2)
   CUR_MJR_PY_VER=$(echo $CUR_PY_VER | cut -d'.' -f1)
   CUR_MIN_PY_VER=$(echo $CUR_PY_VER | cut -d'.' -f2)

   if [ -f "/usr/bin/python$OVERRIDE_PYTHON_VER" ]; then
      PY_VERSION=$OVERRIDE_PYTHON_VER
      echo "Valid python $PY_VERSION version exist. Continue the build process"
      return 0
   else
      if [ "$CUR_MJR_PY_VER" -ge "$REQ_MJR_PY_VER" ]; then
         if [ "$CUR_MIN_PY_VER" -ge "$REQ_MIN_PY_VER" ]; then
            echo "Valid python $PY_VERSION version exist. Continue the build process"
            PY_VERSION=$CUR_PY_VER
            return 0
         fi
      fi
   fi
   return 1
}

#***************************************************************************
#cloneVendorAdtranRepo(): This function is to clone device-vendor-adtran repo
#***************************************************************************
cloneVendorAdtranRepo()
{
   cd $ROOT_PATH
   cd ..
   VENDOR_ADTRAN_PATH="${PWD}/device-vendor-adtran"
   if [ ! -d "$VENDOR_ADTRAN_PATH" ]; then
      git clone -b $VENDOR_ADTRAN_BRANCH $VENDOR_ADTRAN_REPO
   else
      echo "device-vendor-adtran repo exists.Skip clone of vendor-adtran repo."
   fi

   cd $VENDOR_ADTRAN_PATH
   VENDOR_ADTRAN_COMMIT_ID=$(git rev-parse HEAD)
   VNDR_ADTN_SHORT_SHA=$(git rev-parse --short HEAD)
   cd $ROOT_PATH
}

#***************************************************************************
#setup(): This function is to trigger setup.py to clone openwrt
#***************************************************************************
setup()
{
   echo "### Trigger setup.py"
   if [ ! "$(ls -A $BUILD_DIR)" ]; then
       python$PY_VERSION setup.py --setup || exit 1
   else
       python$PY_VERSION setup.py --rebase
       echo "### OpenWrt repo already setup"
   fi
}

#***************************************************************************
#genConfig(): This function is to genrate openwrt's .config file based on
#the inputs provided
#***************************************************************************
genConfig()
{
   echo "### generate .config for 834_5 target ..."
   cd $BUILD_DIR
   python$PY_VERSION $BUILD_DIR/scripts/gen_config.py 834-5 wlan-ap-consumer wifi-834-5 || exit 1
   cd ..
}

#***************************************************************************
#applyOpenSyncMakefilePatch(): This function is to patch opensync's Makefile
# to update vendor repo vendor-plume-openwrt to device-vendor-adtran repo.
#***************************************************************************
applyOpenSyncMakefilePatch()
{
    echo "### Apply opensync Makefile patches to get device-vendor-adtran repo"
    cd $ROOT_PATH
    cd ..
    VENDOR_ADTRAN_PATH="${PWD}/device-vendor-adtran"
    if [ ! -d "$VENDOR_ADTRAN_PATH" ]; then
       echo "device-vendor-adtran directory not found!!"
       exit 1
    fi
    cd $ROOT_PATH
    CFG80211_SEARCH_STR="git@github.com:plume-design/opensync-platform-cfg80211.git"
    CFG80211_REPLACE_STR="https://github.com/plume-design/opensync-platform-cfg80211.git"
    VENDOR_SEARCH_STR=".*git@github.com:plume-design/opensync-vendor-plume-openwrt.git.*"
    VENDOR_REPLACE_STR="\tgit clone --single-branch --branch $VENDOR_ADTRAN_BRANCH file://$VENDOR_ADTRAN_PATH \$(PKG_BUILD_DIR)/vendor/adtran"
    OPENSYNC_MAKEFILE_PATH="$ROOT_PATH/feeds/wlan-ap-consumer/opensync/Makefile"

    sed -i "s#$CFG80211_SEARCH_STR#$CFG80211_REPLACE_STR#" $OPENSYNC_MAKEFILE_PATH
    sed -i "s#$VENDOR_SEARCH_STR#$VENDOR_REPLACE_STR#" $OPENSYNC_MAKEFILE_PATH
}

#***************************************************************************
#addServiceProviderCerts(): This function is to untar service-provider_prod.tgz
#from device-vendor-adtran to feeds/wlan-ap-consumer/opensync
#***************************************************************************
addServiceProviderCerts()
{
    echo "### Add serive provider certificates"
    cd $ROOT_PATH
    if [ ! -f "../device-vendor-adtran/third-party/target/834-5/service-provider_prod.tgz" ]; then
       echo "../device-vendor-adtran/third-party/target/834-5/service-provider_prod.tgz tar file not found. exit build!!"
       exit 1
    fi
    tar -xzvf ../device-vendor-adtran/third-party/target/834-5/service-provider_prod.tgz -C $ROOT_PATH/feeds/wlan-ap-consumer/opensync/src/service-provider
}

#***************************************************************************
#applySetupConfigPatch(): This function is to patch setup.py 
# to update config.yml to openwrt21.2 new config file 
#***************************************************************************
applySetupConfigPatch()
{
    echo "### Apply setup patch to get openwrt 21.02 version "
    cd $ROOT_PATH

    OWRT21_CONFIG_PATH="$ROOT_PATH/configOwrt21_02.yml"
    if [ ! -f "$OWRT21_CONFIG_PATH" ]; then
       echo "openwrt 21.02 config file  not found!!"
       exit 1
    fi
    cd $ROOT_PATH
    CONFIG_SEARCH_STR="config.yml"
    CONFIG_REPLACE_STR="configOwrt21_02.yml"
    OSYNC_SETUP_PATH="$ROOT_PATH/setup.py"

    sed -i "s#$CONFIG_SEARCH_STR#$CONFIG_REPLACE_STR#" $OSYNC_SETUP_PATH
}

#***************************************************************************
#applyPatches(): This function is to copy patches from device-vendor-adtran
# to wlan-ap-consumer and apply patches
#***************************************************************************
applyPatches()
{
    echo "### Copy patches from device-vendor-adtran to wlan-ap-consumer"
    cd $ROOT_PATH
    if [ ! -d "../device-vendor-adtran/additional-patches/target/834-5/patches/openwrt/" ]; then
       echo "device-vendor-adtan directory doesn't have openwrt patches. exit!!"
       exit 1
    fi

    if [ ! -d "../device-vendor-adtran/additional-patches/target/834-5/patches/opensync/" ]; then
       echo "device-vendor-adtan directory doesn't have opensync patches. exit!!"
       exit 1
    fi

    cp ../device-vendor-adtran/additional-patches/target/834-5/patches/opensync/* $ROOT_PATH/feeds/wlan-ap-consumer/opensync/patches/.
    #cleanup default openwrt package patches before copying latest openwrt package patches
    rm -rf $ROOT_PATH/feeds/wlan-ap-consumer/additional-patches/patches/openwrt/*
    cp -r ../device-vendor-adtran/additional-patches/target/834-5/patches/openwrt/* $ROOT_PATH/feeds/wlan-ap-consumer/additional-patches/patches/openwrt/.
    /bin/sh $ROOT_PATH/feeds/wlan-ap-consumer/additional-patches/apply-patches.sh
}

#***************************************************************************
#applyOpenwrtPatches(): This function is to add openwrt patches specific for
#834-5
#***************************************************************************
applyOpenwrtPatches()
{
    TARGET_SPECFIC_OWRT_PATCH_PATH="../../device-vendor-adtran/additional-patches/target/834-5/patches/owrtPatches"
    cd $BUILD_DIR
    for file in "$TARGET_SPECFIC_OWRT_PATCH_PATH"/*
    do
	if [ "$file" == "$TARGET_SPECFIC_OWRT_PATCH_PATH/908-wps-support.patch" ]; then
	    mkdir package/base-files/files/etc/hotplug.d/button
	fi
        echo "patch -d "$BUILD_DIR" -p1 < $file"
        patch -d "$BUILD_DIR" -p1 < "$file"
	if [ "$file" == "$TARGET_SPECFIC_OWRT_PATCH_PATH/908-wps-support.patch" ]; then
	    chmod 755 package/base-files/files/etc/hotplug.d/button/wps
	    chmod 755 package/base-files/files/etc/hotplug.d/button/reset
	fi
    done
    cd $ROOT_PATH
}

#***************************************************************************
#updateWlanApConsumerfeeds(): This function is to update wlan-ap-consumer feeds
# for 834-5
#***************************************************************************
updateWlanApConsumerfeeds()
{
    FEEDS_PATH="../device-vendor-adtran/additional-patches/target/834-5/feeds"
    WLAN_AP_CONSUMER_PATH="$ROOT_PATH/feeds/wlan-ap-consumer"
    echo "### Update wlan-ap-consumer feeds"
    # update wlan-ap-consumer with required packages
    cd $ROOT_PATH
    rm -rf $WLAN_AP_CONSUMER_PATH/miniupnpd
    rm -rf $WLAN_AP_CONSUMER_PATH/python3-jinja2
    rm -rf $WLAN_AP_CONSUMER_PATH/python3-kconfiglib
    rm -rf $WLAN_AP_CONSUMER_PATH/python3-markupsafe
    cp -r $FEEDS_PATH/wlan-ap-consumer/python3* $WLAN_AP_CONSUMER_PATH/.
}

#***************************************************************************
#addPkgFeeds(): This function is to add required packages as feeds for
#834-5
#***************************************************************************

addPkgFeeds()
{
    TARGET_SPECFIC_FEEDS_PATH="../../device-vendor-adtran/additional-patches/target/834-5/feeds"
    PKG_FEEDS_PATH="$BUILD_DIR/feeds/packages/libs"
    PKG_NET_CONF_PATH="$BUILD_DIR/package/network/config"
    echo "### Copy packages in feeds from device-vendor-adtran to wlan-ap-consumer and libs path"
    cd $BUILD_DIR
    # update protobuf with required version of package
    rm -rf $PKG_FEEDS_PATH/protobuf/*
    rm -rf $PKG_FEEDS_PATH/protobuf-c/*
    cp -r $TARGET_SPECFIC_FEEDS_PATH/pkg-overrides/protobuf/* $PKG_FEEDS_PATH/protobuf/.
    cp -r $TARGET_SPECFIC_FEEDS_PATH/pkg-overrides/protobuf-c/* $PKG_FEEDS_PATH/protobuf-c/.
    cd $ROOT_PATH
}

#***************************************************************************
#validatePlumeOSVersion: This function is to validate plume OS version range
#***************************************************************************
validatePlumeOSVersion()
{
    PLUME_OS_VERSION="$1"
    if [ -z "$PLUME_OS_VERSION" ]
    then
        echo "Invalid plumeOS version"
        exit 1;
    fi
    Major=`echo $PLUME_OS_VERSION | awk -F . '{print $1}'`
    Minor=`echo $PLUME_OS_VERSION | awk -F . '{print $2}'`

    if [ -z "$Major" ] || [ -z "$Minor" ] 
    then
       echo "Invalid plumeOS version"
       exit 1;
    fi

    if [ "$Major" -ge 0 ]  && [ "$Major" -lt 100 ]
    then
        if [ "$Minor" -ge 0 ] && [ "$Minor" -lt 100 ]
        then
            echo "PlumeOS version present : $Major.$Minor"
        else
            echo "PlumeOS Minor version not in range"
            exit 1;
        fi
    else
        echo "PlumeOS Major version not in range"
        exit 1;
    fi
}

#***************************************************************************
#generateImageNameStr: This function is to generate the image name to be renamed
#***************************************************************************
generateImageNameStr()
{
    DEVICE="834-5"
    PRODUCT_NAME="834_5"
    IMG_RELEASE_VER="$VER_834_5"
    EXT="bin"

    validatePlumeOSVersion "$IMG_RELEASE_VER"

    cloneVendorAdtranRepo

    if [ "$BUILD_TYPE" == "PROD" ]; then
       WLAN_AP_SHORT_SHA=`git rev-parse --short HEAD`
       WLAN_AP_COMMIT_ID=`git rev-parse HEAD`
       BUILD_ID=$(echo "$WLAN_AP_SHORT_SHA""_""$VNDR_ADTN_SHORT_SHA")
    else
       WLAN_AP_COMMIT_ID=`git rev-parse HEAD`
       BUILD_ID=0
    fi

    IMG_VERSION="$IMG_RELEASE_VER.$YEAR_IN_YY_FMT$MONTH.$BUILD_ID"

    BUILD_IMAGE_NAME="$PRODUCT_NAME-openwrt-opensync-$IMG_VERSION.$EXT"

    if [ "${OPTION}" == "build" ]; then

       echo "#***********************************************************************************" > $ROOT_PATH/plumeos_release
       echo "#BUILD INFO FILE" >> $ROOT_PATH/plumeos_release
       echo "#***********************************************************************************" >> $ROOT_PATH/plumeos_release
       echo "TARGET=\"$DEVICE\"">> $ROOT_PATH/plumeos_release
       echo "PLUME_OS_IMAGE_NAME=\"$BUILD_IMAGE_NAME\"">>  $ROOT_PATH/plumeos_release
       echo "PLUME_OS_IMAGE_VERSION=\"$IMG_VERSION\"">> $ROOT_PATH/plumeos_release
       echo "BUILD_TYPE=\"$BUILD_TYPE\"">> $ROOT_PATH/plumeos_release
       echo "WLAN_AP_COMMIT_SHA=\"$WLAN_AP_COMMIT_ID\"">>  $ROOT_PATH/plumeos_release
       echo "VENDOR_ADTRAN_COMMIT_SHA=\"$VENDOR_ADTRAN_COMMIT_ID\"">> $ROOT_PATH/plumeos_release

    elif [ "${OPTION}" == "rebuild" ]; then
       if [ -f $ROOT_PATH/plumeos_release ]; then
          sed -i "s#^PLUME_OS_IMAGE_NAME.*#PLUME_OS_IMAGE_NAME=\"$BUILD_IMAGE_NAME\"#" $ROOT_PATH/plumeos_release
          sed -i "s#^PLUME_OS_IMAGE_VERSION.*#PLUME_OS_IMAGE_VERSION=\"$IMG_VERSION\"#" $ROOT_PATH/plumeos_release
          sed -i "s#^BUILD_TYPE.*#BUILD_TYPE=\"$BUILD_TYPE\"#" $ROOT_PATH/plumeos_release
       else
          echo "plumeos_release file not found. Exit!!"
          exit 1
       fi
    else
       echo "Invalid build options. Exit!!"
       exit 1
    fi
}

#***************************************************************************
#addPlumeosReleasefile(): This function is to add release file with img details
#***************************************************************************
addPlumeosReleasefile()
{
    cd $BUILD_DIR
    if [  -s "$ROOT_PATH/plumeos_release" ]; then
       echo "### Add plumeos_release file"
       cp $ROOT_PATH/plumeos_release $BUILD_DIR/package/base-files/files/etc/.
    fi
}

#***************************************************************************
#renameTargetImage(): This function is to rename image with proper naming convention
#***************************************************************************
renameTargetImage()
{
   cd $ROOT_PATH
   if [ -d "$ROOT_PATH/OUTPUT" ]; then
      rm -rf $ROOT_PATH/OUTPUT/*
   else
      mkdir -p $ROOT_PATH/OUTPUT
   fi

   if [ -s "$ROOT_PATH/plumeos_release" ]; then
      source $ROOT_PATH/plumeos_release
      TARGET_DEVICE="$TARGET"
      GEN_IMG_NAME="$PLUME_OS_IMAGE_NAME"
      IMG_NAME_834_5_WO_GZ=$(echo $IMG_NAME_834_5 | awk -F . '{print $1"."$2}')

      if [ "$TARGET_DEVICE" = "834-5" ]; then
         if [ -s "$IMG_PATH_834_5/$IMG_NAME_834_5" ]; then
            cp -f $IMG_PATH_834_5/$IMG_NAME_834_5 $ROOT_PATH/OUTPUT/.
	    gzip -d $ROOT_PATH/OUTPUT/$IMG_NAME_834_5 &
	    sleep 10
            if [ -s "$ROOT_PATH/OUTPUT/$IMG_NAME_834_5_WO_GZ" ]; then
               mv $ROOT_PATH/OUTPUT/$IMG_NAME_834_5_WO_GZ $ROOT_PATH/OUTPUT/$GEN_IMG_NAME
            else
               echo "$ROOT_PATH/OUTPUT/$IMG_NAME_834_5_WO_GZ image file not found. Exiting!!"
               exit 1
            fi
         else
            echo "$ROOT_PATH/OUTPUT/$IMG_NAME_834_5 gzip image file not found. Exiting!!"
            exit 1
         fi
      else
         echo "Invalid $TARGET_DEVICE. Exiting!!"
         exit 1
      fi
   fi
}

#***************************************************************************
#compileOpenwrt(): This function is to make openwrt image
#***************************************************************************
compileOpenwrt()
{
    echo "### Building image ..."
    cd $BUILD_DIR
    if [ ! -d "$BUILD_DIR" ]; then
       echo "$BUILD_DIR Not found exit compilation!!"
       exit 1
    fi
    addPlumeosReleasefile
    make -j8 V=s 2>&1 | tee build.log
    cd $ROOT_PATH
    echo "Done"
    renameTargetImage
}

#***************************************************************************
#updateTgtProfileToBpi(): This function is to update target profile to bpi
#***************************************************************************
updateTgtProfileToBpi()
{
    echo "### Update target profile to bananapi-r64 in wifi-834-5.yml"
    sed -i "s/DEVICE_smartrg_sr402ac/DEVICE_bpi_bananapi-r64-rootdisk/" $ROOT_PATH/profiles/wifi-834-5.yml
}

#***************************************************************************
#triggerBuild(): This function is the main build function to clone VndrAdtran,
# apply patches, add serviceprovider certs, setup, generate owrt .config
# and compile the image
#***************************************************************************
triggerBuild()
{
   buildDevice=${1}
   isPythonVerValid
   chkPyVer=$?
   if [ $chkPyVer != 0 ]; then
      echo "Invalid python version. Halt build process"
      exit 1
   fi

   if [ "${buildDevice}" == "bpi" ]; then
      echo "build Image for Banana Pi r64"
      updateTgtProfileToBpi
   else
      echo "build Image for 834-5"
   fi

   cd $ROOT_PATH
   cloneVendorAdtranRepo
   applyOpenSyncMakefilePatch
   addServiceProviderCerts
   applySetupConfigPatch
   updateWlanApConsumerfeeds
   setup
   applyOpenwrtPatches
   genConfig
   addPkgFeeds
   applyPatches
   compileOpenwrt
}

#***************************************************************************
#main() : This is the main function of this script
#***************************************************************************
main()
{
   if [ "${OPTION}" == "build" ]; then
      generateImageNameStr
      triggerBuild ${DEVICE_TYPE};
   elif [ "${OPTION}" == "rebuild" ]; then
      generateImageNameStr
      compileOpenwrt;
   elif [ "${OPTION}" == "help" ]; then
      Usage;
   else
      Usage;
   fi
}

if [ -z "$1" ]; then
   Usage
   exit 1
fi

main
exit 0
