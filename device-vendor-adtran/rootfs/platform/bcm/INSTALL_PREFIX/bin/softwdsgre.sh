#!/bin/sh -xe
# {# jinja-parse #}
INSTALL_PREFIX={{INSTALL_PREFIX}}
ifname=$1
parent=$2
local_ip=$3
remote_ip=$4
ovsh=${INSTALL_PREFIX}/tools/ovsh

ip2mac() {
	timeout -t 15 -s KILL sh -x <<-. | grep .
		while ! ip -4 neigh show to $remote_ip dev $parent \
				| awk '{print \$3}' \
				| sed 1q \
				| grep .
		do
			timeout -t 1 -s KILL ping -c 1 $remote_ip >/dev/null
		done
.
}

get_remote_mac() {
	case "$parent" in
		wl0|wl1)
			ip2mac | grep .
			;;
		*)
			$ovsh -r s DHCP_leased_IP hwaddr -w inet_addr==$remote_ip | grep .
			;;
	esac
}

ip link add link $parent name $ifname type softwds
cd /sys/class/net/$ifname/softwds
echo Y > wrap
echo $local_ip > ip4gre_local_ip
echo $remote_ip > ip4gre_remote_ip
get_remote_mac | sed 1q > addr
