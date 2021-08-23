#!/bin/sh
# {# jinja-parse #}
INSTALL_PREFIX={{INSTALL_PREFIX}}

BR_HOME="br0"
OVSH=$(which ovsh)
WL=$(which wlctl)

gre_filter()
{
    awk '
    /[0-9]+: ([^:])+:/ {
        IF=substr($2, 1, index($2, "@") - 1)
    }

    / +gretap remote/ {
        print IF
    }'
}

gre_stop()
{
    ip -d link show | gre_filter | while read IF
    do
        ip link set dev $IF down
    done
}

gre_purge()
{
    ip -d link show | gre_filter | while read IF
    do
        echo "Removing GRE tunnel: $IF"
        ip link del "$IF"
    done
}

# if there is no master processes, opensync is not running
pgrep $INSTALL_PREFIX/bin/dm > /dev/null || exit 0

# Stop healtcheck
echo 'Stopping healtcheck ...'
$INSTALL_PREFIX/scripts/healthcheck stop

# It is useful to know which interfaces were present before starting
# the teardown in case something stalls later on
ls /sys/class/net

# save NOP list (no radar tool)
#${INSTALL_PREFIX}/bin/nol.sh save

# NM/WM/SM can interact with wifi driver therefore
# nas and epad must be killed afterwards to
# avoid races and unexpected driver sequences.
echo "killing managers"
killall -s SIGKILL dm cm nm wm sm bm om qm fsm fcm pm xm

# From this point on CM is dead and no one is kicking
# watchdog.  There's less than 60s to complete everything
# down below before device throws a panic and reboots.

# Stop htpdate
# echo -n 'stop htpdate...'
# $INSTALL_PREFIX/scripts/htpdate stop

# Stop hostapd, wpa_supplicant
echo -n 'stop hostapd...'
$INSTALL_PREFIX/scripts/hostap stop
killall -s SIGKILL hostapd wpa_supplicant

# Stop miniupnd
#/etc/init.d/miniupnpd stop
#echo "miniupnpd stop"

# Purge all GRE tunnels
gre_stop
gre_purge

# Stop cloud connection
echo "Removing manager"
ovs-vsctl del-manager

# Stop openvswitch
echo "openvswitch stop"
ip link set $BR_HOME down
ovs-vsctl del-br $BR_HOME
$INSTALL_PREFIX/scripts/openvswitch stop

# We don't want to leave stale wifi interfaces running
# around. WM2 won't touch anything that isn't in Config
# table so remove all extra vifs and put the primary
# channels down.
for i in $(ls /sys/class/net/)
do
    echo "$i" | grep "^wl" && {
        ip addr flush dev $i
        ip link set dev $i down
        wl -i $i bss down
        wl -i $i down
    }
done

# This takes a few seconds to execute. Would be
# probably better to have a tool linked to
# libnvram or something.
if test -e /data/.kernel_nvram.setting
then
    nvram getall | grep "=" | cut -d= -f1 | sed 's/^/unset /' | xargs -n 1024 nvram
    nvram loadfile /data/.kernel_nvram.setting
fi

# This prevents garbage from `nvram commit` from polluting the kv store
nvram getall \
    | grep "=" \
    | awk '/^wl/ || /^wps/ || /^lan/ || /^nas_/' \
    | awk '!(/^wl._dis_ch_grp/ || /^wl._radarthrs/)' \
    | cut -d= -f1 \
    | sed 's/^/unset /' \
    | xargs -n 1024 nvram


# Stop DNS service
for PID in $(pgrep dnsmasq); do echo "kill dns: $(cat /proc/$PID/cmdline)"; kill $PID; done

# Stop all DHCP clients
killall udhcpc

# Remove existing DHCP leases
rm /tmp/dhcp.leases

# Reset DNS files
rm -rf /tmp/dns

# Remove certificates
rm -rf /var/.certs > /dev/null 2>&1

sleep 2
