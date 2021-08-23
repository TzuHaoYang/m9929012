#!/bin/sh

if [ "$1" = "" ]; then
	echo "Usage:" > /dev/console
	echo "  $0 <ifname>" > /dev/console
fi

# ifname that wps is started on
ifname=$2
ifname_1=$1
echo "wps_server_monitor called with $ifname $ifname_1" > /dev/console

START_FILE=/tmp/wps_started
STATUS_FILE=/tmp/wps_hosts

#
# Blue LEDs will be used for WPS.
LED="/sys/class/leds/blue"

set_led()
{
	local mode="$1"
	# if we have an LED use it.
	if [ "$LED" != "" ]; then
		echo $mode > $LED/trigger
	fi
}

STAGE=0
STAGE_1=0

echo 1 > $START_FILE
echo -n > $STATUS_FILE

#logger -p crit -t wps_server_monitor "WPS started on $ifname $ifname_1"
echo "WPS started on $ifname $ifname_1" > /dev/console

#This will make the WPS LED blink to indicate that WPS pairing is active
echo "LED timer written to $LED/trigger" > /dev/console
set_led timer

while true
do
	sleep 1
	STATUS=$(hostapd_cli -p /var/run/hostapd-phy0 -i $ifname wps_get_status | grep "Status" | awk '{print $3}')

	if [ "$STATUS" == "Active" ]; then
		STAGE=1
	else
		if [ "$STAGE" == "1" ] && [ "$STATUS" == "Disabled" ]; then
			RESULT=$(hostapd_cli -p /var/run/hostapd-phy0 -i $ifname wps_get_status | grep "result" | awk '{print $4}')
			if [ "$RESULT" == "Success" ]; then
				PEER=$(hostapd_cli -p /var/run/hostapd-phy0 -i $ifname wps_get_status | grep "Peer" | awk '{print $3}')
				echo "$PEER" >> $STATUS_FILE
				echo "WPS connected $PEER on $ifname" > /dev/console
                #This will make the WPS LED ON to indicate that pairing is successful
				set_led default-on
				# restart for the next host
				hostapd_cli  -p /var/run/hostapd-phy0 -i $ifname wps_pbc 
			fi
		fi
	fi

	if [ -n "$ifname_1" ]; then
		STATUS_1=$(hostapd_cli  -p /var/run/hostapd-phy1 -i $ifname_1 wps_get_status | grep "Status" | awk '{print $3}')

		if [ "$STATUS_1" == "Active" ]; then
			STAGE_1=1
		else
			if [ "$STAGE_1" == "1" ] && [ "$STATUS_1" == "Disabled" ]; then
				RESULT_1=$(hostapd_cli -p /var/run/hostapd-phy1 -i $ifname_1 wps_get_status | grep "result" | awk '{print $4}')
				if [ "$RESULT_1" == "Success" ]; then
					PEER=$(hostapd_cli -p /var/run/hostapd-phy1 -i $ifname_1 wps_get_status | grep "Peer" | awk '{print $3}')
					echo "$PEER" >> $STATUS_FILE
					echo "WPS connected $PEER on $ifname_1" > /dev/console
					#This will make the WPS LED ON to indicate that pairing is successful
					set_led default-on
					hostapd_cli -p /var/run/hostapd-phy1 -i $ifname_1 wps_pbc 
				fi
			fi
		fi
	else
		STATUS_1="NONE"
	fi

	if [ "$STATUS" != "Active" ] && [ "$STATUS_1" != "Active" ]; then
		break
	fi

	sleep 4
done

#This will make the WPS LED OFF to indicate that WPS pairing process is inactive
set_led none

rm -f $START_FILE


