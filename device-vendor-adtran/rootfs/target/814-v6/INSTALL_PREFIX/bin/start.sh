#!/bin/sh
# {# jinja-parse #}
INSTALL_PREFIX={{INSTALL_PREFIX}}

BR_WAN=
BR_HOME="br0"
BOOT_TIMEOUT=20

# Get the OpenSync state from DM
bcmcli cms opensync_mode |  grep -E "cloud|monitor|battery|custom"
if [  $? != 0 ]; then
    echo "OpenSync disabled, Bye..."
    exit 0
fi

echo "OpenSync enable, starting up..."

# WAR: if this is first time device is booting, then wait for the
# device to boot all apps
brctl show | grep $BR_HOME > /dev/null && sleep $BOOT_TIMEOUT

# check for override file
if [ -e /tmp/opensync_disable ]; then
    echo "OpenSync override detected, exiting, Bye..."
    exit 0
fi

# Start openvswitch
echo -n 'Starting OpenvSwitch ...'
$INSTALL_PREFIX/scripts/openvswitch start

# stop hostap
echo -n 'stopping hostapd...'
$INSTALL_PREFIX/scripts/hostap stop

#kill dhcp server apps
killall -9 dhcpd

# disable radios
ip link set wl0 down
ip link set wl1 down
wl -i wl0 down
wl -i wl1 down

# Remove br0 that is created by L07 SDK
brctl show | grep $BR_HOME
if [  $? == 0 ]; then
    # remove primary wlan interfaces if already in the bridge
    brctl delif br0 wl0
    brctl delif br0 wl1

    ip link set br0 down &> /dev/null
    ip link del br0 &> /dev/null
fi

# Create run dir for dnsmasq
mkdir -p /var/run/dnsmasq

# Add offset to a MAC address
mac_set_local_bit()
{
    local MAC="$1"

    # ${MAC%%:*} - first digit in MAC address
    # ${MAC#*:} - MAC without first digit
    printf "%02X:%s" $(( 0x${MAC%%:*} | 0x2 )) "${MAC#*:}"
}

# Get the MAC address of an interface
mac_get()
{
    ifconfig "$1" | grep -o -E '([A-F0-9]{2}:){5}[A-F0-9]{2}'
}

##
# Configure bridges
#
MAC_ETH0=$(mac_get eth0)
if [ ! -z $BR_WAN ]; then
    MAC_BRHOME=$(mac_set_local_bit ${MAC_ETH0})
else
    MAC_BRHOME=$MAC_ETH0
fi

if [ ! -z $BR_WAN ]; then
    echo "Adding $BR_WAN with MAC address $MAC_ETH0"
    ovs-vsctl add-br $BR_WAN
    ovs-vsctl set bridge $BR_WAN other-config:hwaddr="$MAC_ETH0"
    ovs-vsctl set int $BR_WAN mtu_request=1500

    # This is gateway example with one ethernet port,
    # so we add eth0 into br-wan where dhcpc is running
    ovs-vsctl add-port $BR_WAN eth0
fi

echo "Adding $BR_HOME with MAC address $MAC_BRHOME"
ovs-vsctl add-br $BR_HOME
ovs-vsctl set bridge $BR_HOME other-config:hwaddr="$MAC_BRHOME"
ifconfig $BR_HOME up

ovs-ofctl add-flow $BR_HOME table=0,priority=50,dl_type=0x886c,actions=local

echo "Adding $BR_HOME.l2uf1"
ovs-vsctl add-port $BR_HOME $BR_HOME.l2uf1 -- set interface $BR_HOME.l2uf1 type=internal ofport_request=500
ovs-ofctl add-flow $BR_HOME "table=0,priority=250,dl_dst=01:00:00:00:00:00/01:00:00:00:00:00,dl_type=0x05ff,actions=NORMAL,output:500"
ifconfig $BR_HOME.l2uf1 up

# add local ifs to br-home
ovs-vsctl add-port $BR_HOME eth0
ovs-vsctl add-port $BR_HOME eth1
ovs-vsctl add-port $BR_HOME eth2
ovs-vsctl add-port $BR_HOME eth3

# Enable radios
wl -i wl0 up
wl -i wl1 up
ip link set wl0 up
ip link set wl1 up

# start hostap
echo -n 'starting hostapd...'
$INSTALL_PREFIX/scripts/hostap start

# Configure ARPs
echo 1 | tee /proc/sys/net/ipv4/conf/*/arp_ignore

##
# Install and configure SSL certs
#
mkdir -p /var/.certs

# start debugnetwork in dev version
cat $INSTALL_PREFIX/.versions | grep "FIRMWARE:" | grep -E "dev|debug"
if [  $? == 0 ]; then
    pgrep dropbear
    if [  $? != 0 ]; then
        echo "Development version, starting debugnet/debugbear"
        /bin/sh ${INSTALL_PREFIX}/scripts/factory/debugnet start
        /bin/sh ${INSTALL_PREFIX}/scripts/factory/debugbear start
    fi
fi

# Update Open_vSwitch table: Must be done here instead of pre-populated
# because row doesn't exist until openvswitch is started
ovsdb-client transact '
["Open_vSwitch", {
    "op": "insert",
    "table": "SSL",
    "row": {
        "ca_cert": "/var/.certs/.ca",
        "certificate": "/var/.certs/.cl",
        "private_key": "/var/.certs/.dk"
    },
    "uuid-name": "ssl_id"
}, {
    "op": "update",
    "table": "Open_vSwitch",
    "where": [],
    "row": {
        "ssl": ["set", [["named-uuid", "ssl_id"]]]
    }
}]'

# Change interface stats update interval to 1 hour
ovsdb-client transact '
["Open_vSwitch", {
    "op": "update",
    "table": "Open_vSwitch",
    "where": [],
    "row": {
        "other_config": ["map", [["stats-update-interval", "3600000"] ]]
    }
}]'

# WAR: disable HW acceleration since it break session of leafs
# with the cloud
fcctl config --hw-accel 0
fcctl flush

# WAR: logger for DevQA
killall -9 syslogd
syslogd -s 2048 -S -b 1 -f /etc/syslog.config

# Start htpdate
# echo -n 'start htpdate...'
# $INSTALL_PREFIX/scripts/htpdate start

start-stop-daemon -S -b -p /var/run/dm.pid -x ${INSTALL_PREFIX}/bin/dm

# Start healtcheck
echo -n 'Starting healtcheck ...'
$INSTALL_PREFIX/scripts/healthcheck start
