#!/bin/sh -e
netdev=$1
action=$2

case "$action" in
tx)
	smac=$(cat /sys/class/net/$1/address)
	dmac=$(wl -i $netdev assoclist | cut -d' ' -f2 | head -n1)
	cd /proc/self/net/pktgen
	! test -e $netdev || echo rem_device_all > kpktgend_0
	echo add_device $netdev > kpktgend_0
	echo count 10000000 > $netdev
	echo pkt_size 1500 > $netdev
	echo delay 0 > $netdev
	echo src_min 1.1.1.2 > $netdev
	echo src_max 1.1.1.2 > $netdev
	echo dst 1.1.1.1 > $netdev
	echo dst_mac $dmac > $netdev
	echo src_mac $smac > $netdev
	pid=
	trap "cat $netdev" INT
	echo start > pgctrl
	;;
rx)
	while sleep 1
	do
		cat /sys/class/net/$netdev/statistics/rx_bytes
	done \
	| awk '{print 8*($1 - x)/1024/1024; x=$1}'
	;;
*)
	cat <<.
usage:
	$0 <ifname> <tx|rx>

	tx - uses pktgen to run traffic
	rx - uses while-loop to dump all incoming byte traffic

example:
	on STA receiver:   $0 wl0 rx
	on AP transmitter: $0 wl0.2 tx

.
	exit 1
	;;
esac
