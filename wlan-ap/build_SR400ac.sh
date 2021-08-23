#!/bin/bash
set -e

# Define ANSI color codes used in print messages.
ANSI_CLR="\e[0m"
ANSI_RED="${ANSI_CLR}\e[1m\e[31m"
ANSI_GRN="${ANSI_CLR}\e[1m\e[32m"
ANSI_CYA="${ANSI_CLR}\e[1m\e[36m"

#***************************************************************************
#Name : build_SR400ac.sh
#Description : This is the master script to build openwrt image for SR400ac
#with opensync
#***************************************************************************
# Assume build type is "ENGR" if unspecified.
BUILD_TYPE=${1:-ENGR}
# Assume option is "build" if unspecified.
OPTION=${2:-build}

GIT_SERVER="${GIT_SERVER:-bitbucket.org}"
MAKE_OPTS="${MAKE_OPTS:--j$(grep -c processor /proc/cpuinfo) -Orecurse V=s}"

# If defined, ensure pre-commit hooks are installed into host and this repo.
if [ -e .pre-commit-config.yaml ] && [ ! -e /.dockerenv ]
then
   if [ ! -e .git/hooks/pre-commit ]
   then
      if ! pre-commit install
      then
         printf "${ANSI_RED}(E) pre-commit platform must be installed in this repo.${ANSI_CLR}\n"
         printf "${ANSI_RED}    Install once into host using:\n\n    python3 -m pip install pre-commit${ANSI_CLR}\n\n"
         printf "${ANSI_RED}    And, then please try again.${ANSI_CLR}\n\n"
         exit 1
      fi
   fi
fi

#***************************************************************************
# The following "header" is provided by DX, and it forces the build process
# to reinvoke itself inside of a Docker container, if it is not already
# running inside a Docker container.  The SKIP_DOCKER env var can be defined
# to a non-empty-string to bypass the Docker requirement.
#***************************************************************************
if [ ! -e /.dockerenv ] && [ -z "${SKIP_DOCKER}" ]
then
   if [ -z "$(docker --version 2>/dev/null)" ]
   then
      printf "${ANSI_RED}(E) This script requires that docker is installed.${ANSI_CLR}\n"
      exit 1
   fi
   # Not inside Docker, so reinvoke this script with the same args - but from
   # inside a Docker image.
   DOCKER_IMAGE=artifactory.adtran.com/adtran-docker-internal/plumeos-build:11
   CMD=(docker run --rm \
      --group-add sudo \
      -e HOME="${HOME}" \
      -u "$(id -u)":"$(id -g)" \
      -v "$(cd ..; pwd)":"$(cd ..; pwd)" \
      -v /etc/group:/etc/group:ro \
      -v /etc/passwd:/etc/passwd:ro \
      -v /etc/shadow:/etc/shadow:ro \
      -w "$(pwd)" \
   )
   if [ -t 1 ]
   then
      CMD+=(-it)
   fi
   if [ -e "${HOME}/.gitconfig" ]
   then
      CMD+=(-v "${HOME}"/.gitconfig:"${HOME}"/.gitconfig:ro)
   fi
   if [ -e "${HOME}/.gnupg" ]
   then
      CMD+=(\
      -e GIT_SERVER="${GIT_SERVER}" \
      -e GNUPGHOME="${HOME}"/.gnupg \
      -e GPG_AGENT_INFO="${GPG_AGENT_INFO}" \
      -e GPG_TTY="${GPG_TTY}" \
      -e MAKE_OPTS="${MAKE_OPTS}" \
      -v "/run/user/$(id -u)/gnupg:/run/user/$(id -u)/gnupg" \
      -v "${HOME}"/.gnupg:"${HOME}"/.gnupg:ro \
   )
   fi
   if [ -e "${HOME}/.ssh" ]
   then
      CMD+=(-v "${HOME}"/.ssh:"${HOME}"/.ssh:ro)
   fi
   if [ -n "${GIT_LFS_SERVER}" ]
   then
      CMD+=(-e GIT_LFS_SERVER="${GIT_LFS_SERVER}")
   fi
   if [ -e "$(dirname "${SSH_AUTH_SOCK}")" ]
   then
      CMD+=(
         -v "$(dirname "${SSH_AUTH_SOCK}")":"$(dirname "${SSH_AUTH_SOCK}")" \
         -e SSH_AUTH_SOCK="${SSH_AUTH_SOCK}" \
      )
   fi
   CMD+=("${DOCKER_IMAGE}" bash -c "'$0' $1 $2 $3")
   printf "${ANSI_CYA}(I) Launching build command in Docker container wrapper:  (Disable with SKIP_DOCKER=1) ...${ANSI_CLR}\n\n    "
   printf "%s " "${CMD[@]}"
   printf "\n\n"
   exec "${CMD[@]}"
else
   printf "${ANSI_CYA}(I) Beginning %s with arguments: BUILD_TYPE=%s OPTION=%s ...${ANSI_CLR}\n" "$(basename "$0")" "${BUILD_TYPE}" "${OPTION}"
fi
#***************************************************************************

ROOT_PATH=${PWD}
BUILD_DIR=${ROOT_PATH}/openwrt
VENDOR_ADTRAN_BRANCH="dev"
VENDOR_ADTRAN_REPO="git@${GIT_SERVER}:smartrg/device-vendor-adtran.git"
FEED_SRG_BRANCH="opensync-21"
FEED_SRG_REPO="git@${GIT_SERVER}:smartrg/feed-srg-plumeos.git"
PY_VERSION=""
REQ_PY_VER="3.7"
OVERRIDE_PYTHON_VER="3.8"
CUR_PY_VER=$(python3 --version | awk '{print $2}' | cut -c1-3)
IMG_PATH_SR400AC="openwrt/bin/targets/bcm53xx/generic"
IMG_NAME_SR400AC="openwrt-bcm53xx-smartrg-sr400ac-squashfs.trx"
IMG_VERSION=""
FW_IMG_VERSION=""
YEAR=`date +%Y`
MONTH=`date +%m`
YEAR_IN_YY_FMT=${YEAR: -2}
VERSION_METAFILE="version_metafile"
WLAN_AP_COMMIT_ID=""
VENDOR_ADTRAN_COMMIT_ID=""
WLAN_AP_SHORT_SHA=""
VNDR_ADTN_SHORT_SHA=""
FEED_SRG_COMMIT_ID=""
FEED_SRG_SHORT_SHA=""
BUILD_IMAGE_NAME=""
GIT_TAG_NAME=""

if [ -s $VERSION_METAFILE ]; then
     source $VERSION_METAFILE
else
     echo "$VERSION_METAFILE is not present. Exit!!"
     exit 1;
fi

#***************************************************************************
#Usage(): This function describe the usage of build_SR400ac.sh
#***************************************************************************
Usage()
{
   echo ""
   echo "build_SR400ac.sh : This is the master script to build openwrt image for SR400ac with opensync"
   echo "Syntax: ./build_SR400ac.sh [PROD|ENGR] [build|rebuild|help]"
   echo ""
   echo "Prerequisite:"
   echo "-- The python version should be 3.7 or greater to avoid python script failures"
   echo "-- After cloning of wlan-ap repo execute git credential cache command, to cache GIT credentials"
   echo "   git config credential.helper 'cache --timeout=1800'"
   echo ""
   echo "options:"
   echo "PROD     To generate a production image"
   echo "ENGR     To generate a development image"
   echo "build    setup, generate config, patch files and builds the image for SR400ac"
   echo "rebuild  re-compile and build SR400ac image"
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

   if [ -n "${OVERRIDE_PYTHON_VER}" ] && [ -f "/usr/bin/python$OVERRIDE_PYTHON_VER" ]
   then
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
   printf "${ANSI_RED}(E) Require Python version ${REQ_MJR_PY_VER}.${REQ_MIN_PY_VER}, but found version ${CUR_MJR_PY_VER}.${CUR_MIN_PY_VER}.${ANSI_CLR}\n"
   printf "${ANSI_RED}    Aborting!${ANSI_CLR}"
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
      printf "${ANSI_CYA}%s ...${ANSI_CLR}\n" "git clone --no-checkout \"${VENDOR_ADTRAN_REPO}\""
      git clone --no-checkout "${VENDOR_ADTRAN_REPO}"
      cd "${VENDOR_ADTRAN_PATH}"
      if [ -n "${GIT_LFS_SERVER}" ]; then
         printf "${ANSI_CYA}(I) Using git-lfs: ${GIT_LFS_SERVER} ...${ANSI_CLR}\n"
         git config lfs.url "${GIT_LFS_SERVER}"
      fi
      if [ "$BUILD_TYPE" == "PROD" ]; then
         git checkout "${GIT_TAG_NAME}"
      else
         git checkout "${VENDOR_ADTRAN_BRANCH}"
      fi
   else
      echo "device-vendor-adtran repo exists. Skip clone of that repo."
   fi

   cd $VENDOR_ADTRAN_PATH
   VENDOR_ADTRAN_COMMIT_ID=$(git rev-parse HEAD)
   VNDR_ADTN_SHORT_SHA=$(git rev-parse --short HEAD)
   cd $ROOT_PATH
}

#***************************************************************************
#clonefeedSrgPlumeosRepo(): This function is to clone feed-srg-plumeos repo
#***************************************************************************
cloneFeedSrgPlumeosRepo()
{
   cd $ROOT_PATH
   cd ..
   FEED_SRG_PATH="${PWD}/srg-plumeos"
   if [ ! -d "$FEED_SRG_PATH" ]; then
      printf "${ANSI_CYA}%s ...${ANSI_CLR}\n" "git clone --no-checkout \"${FEED_SRG_REPO}\" \"${FEED_SRG_PATH}\""
      git clone --no-checkout "${FEED_SRG_REPO}" "${FEED_SRG_PATH}"
      echo "cloned feed srg"
      cd "${FEED_SRG_PATH}"
      if [ -n "${GIT_LFS_SERVER}" ]; then
         printf "${ANSI_CYA}(I) Using git-lfs: ${GIT_LFS_SERVER} ...${ANSI_CLR}\n"
         git config lfs.url "${GIT_LFS_SERVER}"
      fi
      if [ "$BUILD_TYPE" == "PROD" ]; then
         echo "In prod"
         git checkout "${GIT_TAG_NAME}"
         echo "git checkout"
      else
         git checkout "${FEED_SRG_BRANCH}"
      fi
   else
      echo "feed-srg-plumeos repo exists. Skip clone of that repo."
   fi

   cd $FEED_SRG_PATH
   FEED_SRG_COMMIT_ID=$(git rev-parse HEAD)
   FEED_SRG_SHORT_SHA=$(git rev-parse --short HEAD)
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
   echo "### generate .config for SR400ac target ..."
   cd $BUILD_DIR
   python$PY_VERSION $BUILD_DIR/scripts/gen_config.py sr400ac wlan-ap-consumer wifi-sr400ac srg || exit 1
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
    CFG80211_SEARCH_STR="dev_0.5.0.0 git@github.com:plume-design/opensync-platform-cfg80211.git"
    CFG80211_REPLACE_STR="dev_0.5.0.0 https://github.com/plume-design/opensync-platform-cfg80211.git"
    VENDOR_SEARCH_STR=".*git@github.com:plume-design/opensync-vendor-plume-openwrt.git.*"
    VENDOR_REPLACE_STR="\tgit clone --single-branch --branch $VENDOR_ADTRAN_BRANCH file://$VENDOR_ADTRAN_PATH \$(PKG_BUILD_DIR)/vendor/adtran"
    OPENSYNC_MAKEFILE_PATH="$ROOT_PATH/feeds/wlan-ap-consumer/opensync/Makefile"

    sed -i "s#$CFG80211_SEARCH_STR#$CFG80211_REPLACE_STR#" $OPENSYNC_MAKEFILE_PATH
    sed -i "s#$VENDOR_SEARCH_STR#$VENDOR_REPLACE_STR#" $OPENSYNC_MAKEFILE_PATH
}

#***************************************************************************
#addServiceProviderCerts(): This function is to untar service-provider_opensync-dev.tgz
#from device-vendor-adtran to feeds/wlan-ap-consumer/opensync
#***************************************************************************
addServiceProviderCerts()
{
    echo "### Add serive provider certificates"
    cd $ROOT_PATH
    if [ ! -f "../device-vendor-adtran/third-party/target/SR400ac/service-provider_opensync-dev.tgz" ]; then
       echo "../device-vendor-adtran/third-party/target/SR400ac/service-provider_opensync-dev.tgz tar file not found. exit build!!"
       exit 1
    fi
    tar -xzvf ../device-vendor-adtran/third-party/target/SR400ac/service-provider_opensync-dev.tgz -C $ROOT_PATH/feeds/wlan-ap-consumer/opensync/src/service-provider
}

#***************************************************************************
#applyPatches(): This function is to copy patches from device-vendor-adtran
# to wlan-ap-consumer and apply patches
#***************************************************************************
applyPatches()
{
    echo "### Copy patches from device-vendor-adtran to wlan-ap-consumer"
    cd $ROOT_PATH
    if [ ! -d "../device-vendor-adtran/additional-patches/target/SR400ac/patches/openwrt/" ]; then
       echo "device-vendor-adtan directory doesn't have openwrt patches. exit!!"
       exit 1
    fi

    if [ ! -d "../device-vendor-adtran/additional-patches/target/SR400ac/patches/opensync/" ]; then
       echo "device-vendor-adtan directory doesn't have opensync patches. exit!!"
       exit 1
    fi

    cp ../device-vendor-adtran/additional-patches/target/SR400ac/patches/opensync/* $ROOT_PATH/feeds/wlan-ap-consumer/opensync/patches/.
    cp -r ../device-vendor-adtran/additional-patches/target/SR400ac/patches/openwrt/* $ROOT_PATH/feeds/wlan-ap-consumer/additional-patches/patches/openwrt/.
    /bin/sh $ROOT_PATH/feeds/wlan-ap-consumer/additional-patches/apply-patches.sh
}

#***************************************************************************
#brcmMkPatchToCopyFirmware(): This fuction is to add a patch to broadcom.mk
#to copy brcmfmac43602a3-pcie.bin firmware to /lib/firmware/brcm/.
#SR400ac D0 revision board requires brcmfmac43602a3-pcie.bin version instead
#of brcmfmac43602-pcie.bin version of the firmware
#***************************************************************************
brcmMkPatchToCopyFirmware()
{
    echo "### Apply patch to copy brcmfmac43602a3-pcie.bin firmware ..."
    BRCM_MAKE_FILE_PATCH="0001-firmware-copy-brcmfmac_bin-from-feeds.patch"
    cd $BUILD_DIR
    patch -d "$BUILD_DIR" -p1 < "feeds/wifi/firmware/$BRCM_MAKE_FILE_PATCH"
    cd $ROOT_PATH
}

#***************************************************************************
#applyOpenwrtPatches(): This function is to add openwrt patches specific for
#SR400ac
#***************************************************************************
applyOpenwrtPatches()
{
    TARGET_SPECFIC_OWRT_PATCH_PATH="../../device-vendor-adtran/additional-patches/target/SR400ac/patches/owrtPatches"
    cd $BUILD_DIR
    for file in "$TARGET_SPECFIC_OWRT_PATCH_PATH"/*
    do
        echo "patch -d "$BUILD_DIR" -p1 < $file"
        patch -d "$BUILD_DIR" -p1 < "$file"
		if [ "$file" == "$TARGET_SPECFIC_OWRT_PATCH_PATH/901-smartrg-sr400ac-openwrt-wps.patch" ]; then
		chmod 755 package/base-files/files/etc/hotplug.d/button/wps
		fi

    done
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
    DEVICE="SR400ac"
    PRODUCT_NAME="SR400ac"
    IMG_RELEASE_VER=$VER_SR400AC
    EXT="trx"

    validatePlumeOSVersion "$IMG_RELEASE_VER"

    if [ "$BUILD_TYPE" == "PROD" ]; then
       GIT_TAG_NAME=$(git describe --tags $(git rev-list --tags --max-count=1))
       if [ "$GIT_TAG_NAME" == "" ]; then
          echo "GIT_TAG_NAME is empty cannot generate PROD image name. Exit!!"
          exit 1
       fi
    fi

    cloneVendorAdtranRepo
    cloneFeedSrgPlumeosRepo

    if [ "$BUILD_TYPE" == "PROD" ]; then
       WLAN_AP_SHORT_SHA=`git rev-parse --short HEAD`
       WLAN_AP_COMMIT_ID=`git rev-parse HEAD`
       BUILD_ID=$(echo $GIT_TAG_NAME | awk -F "-" '{print $2}')
       BUILD_TYPE_STR="prod"
    else
       WLAN_AP_COMMIT_ID=`git rev-parse HEAD`
       BUILD_ID=0
       BUILD_TYPE_STR="eng"
    fi

    IMG_VERSION="$IMG_RELEASE_VER.$YEAR_IN_YY_FMT.$MONTH-$BUILD_ID"
    FW_IMG_VERSION="$IMG_VERSION-notgit-adtran-$BUILD_TYPE_STR"
    BUILD_IMAGE_NAME="$PRODUCT_NAME-openwrt-opensync-$IMG_VERSION.$EXT"

    if [ "${OPTION}" == "build" ]; then

       echo "#***********************************************************************************" > $ROOT_PATH/plumeos_release
       echo "#BUILD INFO FILE" >> $ROOT_PATH/plumeos_release
       echo "#***********************************************************************************" >> $ROOT_PATH/plumeos_release
       echo "TARGET=\"$DEVICE\"">> $ROOT_PATH/plumeos_release
       echo "PLUME_OS_IMAGE_NAME=\"$BUILD_IMAGE_NAME\"">>  $ROOT_PATH/plumeos_release
       echo "PLUME_OS_IMAGE_VERSION=\"$FW_IMG_VERSION\"">> $ROOT_PATH/plumeos_release
       echo "BUILD_TYPE=\"$BUILD_TYPE\"">> $ROOT_PATH/plumeos_release
       echo "WLAN_AP_COMMIT_SHA=\"$WLAN_AP_COMMIT_ID\"">>  $ROOT_PATH/plumeos_release
       echo "VENDOR_ADTRAN_COMMIT_SHA=\"$VENDOR_ADTRAN_COMMIT_ID\"">> $ROOT_PATH/plumeos_release
       echo "FEED_SRG_PLUMEOS_COMMIT_SHA=\"$FEED_SRG_COMMIT_ID\"">> $ROOT_PATH/plumeos_release

    elif [ "${OPTION}" == "rebuild" ]; then
       if [ -f $ROOT_PATH/plumeos_release ]; then
          sed -i "s#^PLUME_OS_IMAGE_NAME.*#PLUME_OS_IMAGE_NAME=\"$BUILD_IMAGE_NAME\"#" $ROOT_PATH/plumeos_release
          sed -i "s#^PLUME_OS_IMAGE_VERSION.*#PLUME_OS_IMAGE_VERSION=\"$FW_IMG_VERSION\"#" $ROOT_PATH/plumeos_release
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

      if [ -s "$IMG_PATH_SR400AC/$IMG_NAME_SR400AC" ]; then
         cp -f $IMG_PATH_SR400AC/$IMG_NAME_SR400AC $ROOT_PATH/OUTPUT/.
         mv $ROOT_PATH/OUTPUT/$IMG_NAME_SR400AC $ROOT_PATH/OUTPUT/$GEN_IMG_NAME
      else
         echo "$IMG_PATH_SR400AC/$IMG_NAME_SR400AC not present. Exiting."
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
   printf "${ANSI_CYA}(I) Executing: make PROFILE=SR400ac ${MAKE_OPTS} 2>&1 | tee build-openwrt-SR400ac.log ... ${ANSI_CLR}\n"
   make PROFILE=SR400ac ${MAKE_OPTS} 2>&1 | tee build-openwrt-SR400ac.log
   cd $ROOT_PATH
   echo "Done"
   renameTargetImage
}

#***************************************************************************
#triggerBuild(): This function is the main build function to clone VndrAdtran,
# apply patches, add serviceprovider certs, setup, generate owrt .config
# and compile the image
#***************************************************************************
triggerBuild()
{
   isPythonVerValid
   chkPyVer=$?
   if [ $chkPyVer != 0 ]; then
      echo "Invalid python version. Halt build process"
      exit 1
   fi

   cd $ROOT_PATH
   cloneVendorAdtranRepo
   applyOpenSyncMakefilePatch
   addServiceProviderCerts
   setup
   genConfig
   brcmMkPatchToCopyFirmware
   applyOpenwrtPatches
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
      triggerBuild;
      printf "${ANSI_GRN}(I) Successfully built ${DEVICE_TYPE}!${ANSI_CLR}\n"
   elif [ "${OPTION}" == "rebuild" ]; then
      generateImageNameStr
      compileOpenwrt;
      printf "${ANSI_GRN}(I) Successfully rebuilt OpenWRT!${ANSI_CLR}\n"
   elif [ "${OPTION}" == "help" ]; then
      Usage;
   else
      Usage;
   fi
}

if [ -z "${BUILD_TYPE}" ] || [ -z "${OPTION}" ] || [ "${1}" = "-h" ] || [ "${1}" = "--help" ]
then
   Usage
   exit 1
fi

main
exit 0
