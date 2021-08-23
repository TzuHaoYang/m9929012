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

# It is useful to know which interfaces were present before starting
# the teardown in case something stalls later on
ls /sys/class/net

# NM/WM/SM can interact with wifi driver therefore
# nas and epad must be killed afterwards to
# avoid races and unexpected driver sequences.
echo "killing managers"
killall -s SIGKILL $(cd ${INSTALL_PREFIX}/bin/ && ls *m | grep -v -x fm)

# From this point on CM is dead and no one is kicking
# watchdog.  There's less than 60s to complete everything
# down below before device throws a panic and reboots.

# Purge all GRE tunnels
gre_stop
gre_purge

# Stop hostapd, wpa_supplicant
echo 'stop hostapd...'
killall hostapd
killall wpa_supplicant

# Stop openvswitch
echo 'stop openvswitch...'
for BR in $(ovs-vsctl list-br)
do
    ovs-vsctl del-br $BR
done
/etc/init.d/openvswitch stop

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

# disable vif
ifconfig eth0.1 down
ifconfig eth0.2 down
