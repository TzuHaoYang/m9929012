#!/bin/sh
# Copyright (c) 2020, Plume Design Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#    1. Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#    2. Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#    3. Neither the name of the Plume Design Inc. nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Plume Design Inc. BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# {# jinja-parse #}
INSTALL_PREFIX={{INSTALL_PREFIX}}

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

mac_set_last_bit()
{
    echo "$1" | sed -e 's/0$/1/'
}

##
# Configure bridges
#
MAC_ETH0=$(mac_get eth0)
MAC_ETH1=$(mac_set_last_bit ${MAC_ETH0})
# Set the local bit on eth0
# MAC_ETH1=$(mac_set_local_bit ${MAC_ETH0})

echo "Start openvswitch"
/etc/init.d/openvswitch start
sleep 1

echo "Rename wlan interface to wifi prefix"
ip link set wlan0 name wifi2g
ip link set wlan1 name wifi5g
iw dev wifi2g set type __ap
iw dev wifi5g set type __ap


echo "Adding br-wan with MAC address $MAC_ETH0"
ovs-vsctl add-br br-wan
ovs-vsctl set bridge br-wan other-config:hwaddr="$MAC_ETH0"
ovs-vsctl set int br-wan mtu_request=1500
ovs-vsctl add-port br-wan eth0.2

echo "Adding br-home with MAC address $MAC_ETH1"
ovs-vsctl add-br br-lan
ovs-vsctl set bridge br-lan other-config:hwaddr="$MAC_ETH1"
ovs-vsctl add-port br-lan eth0.1

echo "Enabling LAN interface eth1"
ifconfig eth1 up

/etc/init.d/wpad/ start/

mkdir -p /var/certs
cp ${INSTALL_PREFIX}/certs/* /var/certs/

# Update Open_vSwitch table: Must be done here instead of pre-populated
# because row doesn't exist until openvswitch is started
ovsdb-client transact '
["Open_vSwitch", {
    "op": "insert",
    "table": "SSL",
    "row": {
        "ca_cert": "/var/certs/ca.pem",
        "certificate": "/var/certs/client.pem",
        "private_key": "/var/certs/client_dec.key"
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
