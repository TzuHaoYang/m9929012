#!/bin/sh
ifname=$1

if ! wl -i "$ifname" bss >/dev/null 2>/dev/null
then
    # Possibly handled by different driver.
    exit 1
fi

if ! wl -i "$ifname" bss | grep -q up
then
    log_warn "$ifname: bss is not up"
    exit 1
fi

if wl -i "$ifname" bssid | grep -q '00:00:00:00:00:00'
then
    log_warn "$ifname: bss up, but not associated"
    exit 1
fi

exit 0
