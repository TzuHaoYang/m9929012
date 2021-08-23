#!/bin/bash -e

myname=$(basename $0)

#######
# Variables that can be set and overridden by environment
#
# BUILD_START_DIR
# BUILD_PROFILES
# DOCKER_RUN_PATH
# DOCKER_TAG
#
# DOCKER_CLEAN
# BUILD_CLEAN
# OPENSYNC_SRC
# OPENSYNC_INCREMENTAL
# SDK_INCREMENTAL
# MAKE_OP
# MAKE_APPEND

ADTRAN_BCM_TARGETS="814-v6"


[ "$CLEAN_BUILD" = "true" ] && BUILD_CLEAN=1

if [ -z "$WORKSPACE" ]; then
    echo "Manual build (jenkins env not detected)"
    # set WORKSPACE and other env options suitable for manual build
    WORKSPACE="$PWD"
    : ${OPENSYNC_INCREMENTAL=1}
    : ${SDK_INCREMENTAL=1}
    : ${BUILD_NUMBER=0}
    : ${BUILD_PROFILES="dev-debug"}
else
    # under jenkins use JOB_NAME as DOCKER_TAG
    [ -n "$JOB_NAME" ] && : ${DOCKER_TAG=$(echo -n $JOB_NAME | tr A-Z a-z)}
fi

echo_report()
{
    if [ $# -eq 0 ]; then echo; return; fi  # empty line

    timestamp="[$(date --rfc-3339=seconds)]"
    echo "${timestamp}[${myname}] $@"
}

docker_run()
{
    echo_report "Executing: dock-run $@"

    (cd "${BUILD_START_DIR}" && \
        DOCKER_TAG="${DOCKER_TAG}" \
        ${DOCKER_RUN_PATH} "$@")
}

#=============================================================================

if [ -z "$1" ]; then
    echo "usage: $0 <TARGET>"
    exit 1
fi

echo_report
echo_report "Started"
start_time="$(date -u +%s)"

TGT=$1

# source optional external target jenkins env
external_target()
{
    local basedir=$(readlink -f -- `dirname $0`/../../../..)
    local found=0
    local F
    for F in "$basedir"/vendor/*/build/"jenkins.$TGT.sh"; do
        if [ -f "$F" ]; then
            found=1
            echo include "$F"
            source "$F"
        fi
    done
    if [ "$found" = 0 ]; then
        false
        return
    fi
    if [ -z "$PLATFORM" ]; then
        echo Unknown PLATFORM for: $TGT
        exit 1
    fi
    true
}

[[ $ADTRAN_BCM_TARGETS =~ (^|[[:space:]])"$TGT"($|[[:space:]]) ]] && {
    PLATFORM=BCM
}

if [ -z "$OPENSYNC_SRC" -a -n "$BUILD_PLUME_SRC" ]; then
    # backward compatibility
    OPENSYNC_SRC="$BUILD_PLUME_SRC"
fi

# ": ${A=x}" sets the value unless the variable is already set
case $TGT in
    814-v6)
        : ${BUILD_START_DIR="${WORKSPACE}/device-sdk-adtran-bcm-dsl"}
        : ${OPENSYNC_SRC="${WORKSPACE}/plume"}
        : ${DOCKER_RUN_PATH="${BUILD_START_DIR}/docker/dock-run"}
        : ${BUILD_PROFILES="development"}
        : ${MAKE_ARGS_TGT="CONFIG=814-v6"}
        ;;
    *)
        if ! external_target; then
            echo Unknown TARGET: $TGT
            exit 1
        fi
        ;;
esac

if [ -n "${IMAGE_SIGN}" ]; then
    MAKE_ARGS_TGT="${MAKE_ARGS_TGT} IMAGE_SIGN=${IMAGE_SIGN}"
fi

echo_report
echo_report "------------------------------------------------------------------------------"
echo_report "Pre-build cleanup"
echo_report "------------------------------------------------------------------------------"

# optional clean docker
[ "${DOCKER_CLEAN}" = "true" ] && {
    echo_report
    echo_report "DOCKER_CLEAN=${DOCKER_CLEAN}"
    echo_report
    if [ -z "$DOCKER_TAG" ]; then
        echo "ERROR unknown DOCKER_TAG"
        exit 1
    fi
    ( set -x
    docker ps --filter ancestor="${DOCKER_TAG}" -q | xargs -r docker kill
    docker ps --filter ancestor="${DOCKER_TAG}" -q -a | xargs -r docker rm
    docker images -q "${DOCKER_TAG}" | xargs -r docker rmi
    )
}

echo_report
echo_report "Docker image (re)build"
echo_report
(
    docker_run --build-docker-image
)

# clean cache statistics
echo
echo "=== CCACHE CLEAR ==="
docker_run -qqq ccache -z


if [ "${PLATFORM}" = "QCA" ]; then
    MAKE_ARGS="TARGET=${TGT}"
elif [ "${PLATFORM}" = "BCM" ]; then
    MAKE_ARGS="OPENSYNC_SRC=${OPENSYNC_SRC} PLUME_SRC=${OPENSYNC_SRC}"
fi

if [ "${PLATFORM}" = "QCA" ]; then
    if [ "${BUILD_CLEAN}" = "1" ]; then
        echo_report
        echo_report "------------------------------------------------------------------------------"
        echo_report "Full clean (BUILD_CLEAN=${BUILD_CLEAN})"
        echo_report "------------------------------------------------------------------------------"
        ( set -x
        git clean -dfx
        git submodule foreach git clean -dfx
        )
    else
        # Do the minimal cleanup required to ensure a correct build
        echo_report
        echo_report "------------------------------------------------------------------------------"
        echo_report "Pre-build SDK 'mini-clean'"
        echo_report "------------------------------------------------------------------------------"
        docker_run -qqq make mini-clean ${MAKE_ARGS}

        if [ "${OPENSYNC_INCREMENTAL}" != "1" ]; then
            echo_report
            echo_report "------------------------------------------------------------------------------"
            echo_report "Clean OpenSync"
            echo_report "------------------------------------------------------------------------------"
            docker_run -qqq make clean ${MAKE_ARGS}
        fi
    fi
elif [ "${PLATFORM}" = "BCM" ]; then
    if [ "${BUILD_CLEAN}" = "1" ]; then
        # clean full SDK
        echo_report
        echo_report "------------------------------------------------------------------------------"
        echo_report "Full clean (BUILD_CLEAN=${BUILD_CLEAN})"
        echo_report "------------------------------------------------------------------------------"
        ( set -x
        git -C "${BUILD_START_DIR}" clean -fdx
        )
        OPENSYNC_INCREMENTAL=0
    elif [ "${SDK_INCREMENTAL}" != "1" ]; then
        # clean all SDK builds (old and current, but keep cache)
        echo_report
        echo_report "------------------------------------------------------------------------------"
        echo_report "Clean all SDK builds"
        echo_report "------------------------------------------------------------------------------"
        if grep -q cleanall "${BUILD_START_DIR}/Makefile" && [ ! -e "${BUILD_START_DIR}"/Makefile.plume ]; then
            docker_run -qqq make cleanall ${MAKE_ARGS}
        else
            ( set -x
            rm -rf "${BUILD_START_DIR}"/.build* "${BUILD_START_DIR}"/build*
            )
        fi
    else
        # clean old SDK builds and images
        echo_report
        echo_report "------------------------------------------------------------------------------"
        echo_report "Clean old SDK builds"
        echo_report "------------------------------------------------------------------------------"
        if grep -q cleanold "${BUILD_START_DIR}/Makefile"; then
            docker_run -qqq make cleanold ${MAKE_ARGS}
        else
            ( set -x
            rm -rf "${BUILD_START_DIR}"/build*.old.*
            )
        fi
        ( set -x
        rm -rfv "${BUILD_START_DIR}"/build*-y/images
        )
    fi
    if [ "${OPENSYNC_INCREMENTAL}" != "1" ]; then
        # clean OpenSync
        echo_report
        echo_report "------------------------------------------------------------------------------"
        echo_report "Clean OpenSync"
        echo_report "------------------------------------------------------------------------------"
        find ${OPENSYNC_SRC} -maxdepth 5 -name .git | while read DOTGIT; do
            ( set -x
            git -C "${DOTGIT%/.git}" clean -fdx
            )
        done
    fi
fi


# Build all deploy profiles
deploy_profiles=( $BUILD_PROFILES )
for ((bp=0;bp < ${#deploy_profiles[@]};bp++)); do
    BLD_PROFILE="${deploy_profiles[bp]}"

    if [ "${PLATFORM}" = "BCM" ]; then
        : ${MAKE_OP="build"}
    elif [ "${PLATFORM}" = "QCA" ]; then
        if [ $bp -eq 0 ]; then
            # Our first profile, do a full OS make
            : ${MAKE_OP="os"}
        else
            # Subsequent profiles, just do deploy profile
            MAKE_OP="openwrt_pack"
        fi
    fi

    echo_report
    echo_report "------------------------------------------------------------------------------"
    echo_report "Building FW image with deployment profile: ${BLD_PROFILE}"
    echo_report "------------------------------------------------------------------------------"
    echo_report
    docker_run --skip-docker-rebuild \
        make ${MAKE_OP} \
        ${MAKE_ARGS} \
        ${MAKE_ARGS_TGT} \
        BUILD_NUMBER="${BUILD_NUMBER}" \
        IMAGE_DEPLOYMENT_PROFILE="${BLD_PROFILE}" \
        V=s \
        ${MAKE_APPEND}
done

#=============================================================================

echo "=== CCACHE REPORT ==="
docker_run -qqq ccache -s
echo "=== CCACHE END ==="

end_time="$(date -u +%s)"
elapsed="$(($end_time-$start_time))"
elapsed_hms=$(date -d@"${elapsed}" -u "+%-kh %Mm %Ss")

echo_report
echo_report "Build duration: ${elapsed_hms}"
echo_report
echo_report "------------------------------------------------------------------------------"
echo_report "Script finished successfully."
echo_report "------------------------------------------------------------------------------"
echo_report
