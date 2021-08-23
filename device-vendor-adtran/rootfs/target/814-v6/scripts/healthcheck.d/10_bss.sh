#!/bin/sh
OVSH=$CONFIG_INSTALL_PREFIX/tools/ovsh
BSS_IS_UP_D=$CONFIG_INSTALL_PREFIX/scripts/bss_is_up.d

run_parts() {
    dir=$1
    shift
    for i in $(ls $dir/*)
    do
        ( . "$i" "$@" ) && return 0
    done
    return 1
}

for ifname in $($OVSH -r s Wifi_VIF_Config if_name -w enabled==true)
do
    run_parts "$BSS_IS_UP_D" "$ifname" && continue
    log_warn "$ifname: bss is not up"
    exit 1
done
