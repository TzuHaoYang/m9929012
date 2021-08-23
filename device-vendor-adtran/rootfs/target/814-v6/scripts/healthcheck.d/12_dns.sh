#!/bin/sh

LOG_MODULE="DNS"
RESOLV_CONF="/etc/resolv.conf"
HOSTNAME="wildfire.plumenet.io"
DNS_SERVERS="209.244.0.3 64.6.64.6 84.200.69.80"
PLOOKUP=$CONFIG_INSTALL_PREFIX/tools/plookup
timeout=$(timeout -t 0 true > /dev/null 2>&1 && echo timeout -t || echo timeout)

ret=0

is_internet_available()
{
    # Check if internet is available by pinging random two addresses from
    # list of global root DNS server (also used by CM).
    # Info: https://www.iana.org/domains/root/servers
    local addr0="198.41.0.4"
    local addr1="192.228.79.201"
    local addr2="192.33.4.12"
    local addr3="199.7.91.13"
    local addr4="192.5.5.241"
    local addr5="198.97.190.53"
    local addr6="192.36.148.17"
    local addr7="192.58.128.30"
    local addr8="193.0.14.129"
    local addr9="199.7.83.42"

    local r1=$(( 0x$(head -c 20 /dev/urandom | md5sum | head -c8) % 10 ))
    local r2=$(( (r1 + 4) % 10 ))
    eval addr_first="\$addr$r1"
    eval addr_second="\$addr$r2"

    ping $addr_first -c 2 -s 4 -w 2 > /dev/null || {
        ping $addr_second -c 2 -s 4 -w 2 > /dev/null || {
            # Internet not available after double check
            return 1
        }
    }

    return 0
}

check_dns_servers()
{

	local logMsg=$1
	local servers=$2

	pass=0
	testCnt=0

	for dns in $servers; do
		$timeout 10 $PLOOKUP $HOSTNAME $dns > /dev/null 2>&1
		if [ $? -ne 0 ]; then
			log_err "$dns failed"
		else
			pass=$((pass + 1))
		fi
		testCnt=$((testCnt + 1))
	done

	if [ $pass -ne $testCnt ]; then
		log_debug "$logMsg $pass/$testCnt"
	fi

	if [ $pass -gt 0 ]; then
		Healthcheck_Pass
	fi
}

#Check only device in Gateway mode
#gw_mode=`ovs-vsctl list-ifaces br-wan | grep eth`
#if [ -z "$gw_mode" ]; then
#	Healthcheck_Pass
#else
#	gw_ip=`ifconfig br-wan | grep "inet addr"`
#	if [ -z "$gw_ip" ]; then
#		log_info "Skip healthcheck, ip addr not ready"
#		Healthcheck_Pass
#	fi
#fi

# In case that internet is not available we shouldn't execute DNS checks
is_internet_available
if [ $? -ne 0 ]; then
    log_info "Skip healthcheck, internet not available"
    Healthcheck_Pass
fi

# Check default DNS server configuration
nslookup $HOSTNAME > /dev/null 2>&1
if [ $? -ne 0 ]; then
	log_err "Default DNS failed"
else
	Healthcheck_Pass
fi

#
# On Adtran DNS is not under our control.
# All we can do is wait.
#

# Check default DNS servers
#default_servers=`awk '$1 == "nameserver" {print $2}' $RESOLV_CONF`
#check_dns_servers "Check default DNS servers" $default_servers

# Check reference DNS servers
#check_dns_servers "Check reference DNS servers" $DNS_SERVERS

# Return fail, no DNS available
Healthcheck_Fail
