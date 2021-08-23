#!/bin/sh
# {# jinja-parse #}
INSTALL_PREFIX={{INSTALL_PREFIX}}
#
# When triggered by 'lm' process, it gathers system logs and stats, into a
# /var/log/lm/system_logs_<timestamp>.tar.gz
#
# parameters:
# #1 - url where logs will be uploaded to
# #2 - upload token / upload log file
# #3 - log manager's home directory
# #4 - path to syslog messages
# #5 - path to copy of syslog messages
# #6 - temporary folder
# #7 - path to crash dump folder
# #8 - path to ovsdb file
# #9 - path to ovsdb log file
#

PATH=${INSTALL_PREFIX}/scripts/:${INSTALL_PREFIX}/tools:${PATH}
timeout=$(timeout -t 0 true > /dev/null 2>&1 && echo timeout -t || echo timeout)


log()
{
    [ -n "$LM_STDOUT" ] && {
        echo "$@" 1>&2
    } || {
        echo "$@"
    }
}

errorout()
{
    log -n "Error:$1 "
    log "Error invoking lm_logs_collector.sh script"
    log "Syntax: lm_logs_collector.sh <upload_location> <upload_token>"
    log "        /var/log/lm syslog syslog_copy tmp crash <ovsdb_config_file> <ovsdb_log>"
    exit 1
}


#
# Parse optional arguments
#
while [ -n "$1" ]; do
    case "$1" in
        --stdout)
            LM_STDOUT=1
            shift
            ;;

        --*)
            log "Unknown option: $1"
            exit 1
            ;;

        *)
            break
            ;;
    esac
done

if [ -z "$LM_STDOUT" ]; then
    if [ -z "$1" -o -z "$2" ]; then
        errorout
    fi
fi


LM_UPLOAD_LOG_LOCATION=${1:-INVALID_URL}
LM_UPLOAD_LOG_FILE=${2:-INVALID_FILE}
LM_DIR_HOME=${3:-/var/log/lm}
LM_DIR_SYSLOG=${4:-syslog}
LM_DIR_SYSLOG_COPY=${5:-syslog_copy}
LM_DIR_TEMP=${6:-tmp}
LM_DIR_CRASH=${7:-crash}
LM_DIR_OVSDB_CFG=${8:-/tmp/etc/openvswitch/conf.db}
LM_DIR_OVSDB_LOG=${9:-/tmp/ovsdb.log}

LM_DIR_TEMP=$LM_DIR_HOME/$LM_DIR_TEMP
LM_TIME_STAMP="$(date +'%Y%m%d_%H%M%S')"
LM_DIR_LOG="$LM_DIR_TEMP/logs_$LM_TIME_STAMP"

LM_LOG_ARCHIVE="${INSTALL_PREFIX}/log_archive"

lm_upload_logs()
{
    cd $LM_DIR_HOME
    mv system_logs_*.gz $LM_UPLOAD_LOG_FILE
    if [ -n "$LM_STDOUT" ]; then
        cat $LM_UPLOAD_LOG_FILE
        rm -f $LM_UPLOAD_LOG_FILE
    else
        curl --verbose \
            --cacert /var/.certs/.up \
            --form filename=@$LM_UPLOAD_LOG_FILE \
            $LM_UPLOAD_LOG_LOCATION
        if [ $? -eq 0 ]; then
            logger "[LOGPULL] Uploading logpull $LM_UPLOAD_LOG_FILE successful"
            rm -f $LM_UPLOAD_LOG_FILE
            rm ${LM_LOG_ARCHIVE}/logpull.tgz      2> /dev/null
            rm ${LM_LOG_ARCHIVE}/logpull.failed   2> /dev/null
        else
            logger "[LOGPULL] Uploading logpull $LM_UPLOAD_LOG_FILE FAILED!"
            mv $LM_UPLOAD_LOG_FILE          ${LM_LOG_ARCHIVE}/logpull.tgz
            echo $LM_UPLOAD_LOG_FILE     >  ${LM_LOG_ARCHIVE}/logpull.failed
            echo $LM_UPLOAD_LOG_LOCATION >> ${LM_LOG_ARCHIVE}/logpull.failed
        fi
    fi
}

collect_cmd()
{
    # Note: The "sed" below replaces the / character with the UTF-8 / character
    OUTPUT="$LM_DIR_LOG/$(echo -n "$@" | tr -C "A-Za-z0-9.-" _)"
    ("$@") > "$OUTPUT" 2>&1 || true
}

collect_file_safe()
{
    local src=$1
    local dst=$2

    local file_size=$(du -k $src | awk '{ print $1 }')
    local free_memory=$(grep MemFree /proc/meminfo | awk '{ print $2 }')

    # We are leaving at least 10 MB of free memory
    [ $(( $free_memory - $file_size )) -gt 10000 ] && {
        cp $src $dst
        return 0
    }

    return 1
}

collect_hostap()
{
    # Collect hostapd and supplicant config files
    for FN in /var/run/*.config /var/run/*.pskfile; do
        collect_cmd cat $FN
    done

    for sockdir in $(find /var/run/hostapd-* -type d); do
        for ifname in $(ls $sockdir/); do
            collect_cmd $timeout 1 hostapd_cli -p $sockdir -i $ifname status
            collect_cmd $timeout 1 hostapd_cli -p $sockdir -i $ifname all_sta
            collect_cmd $timeout 1 hostapd_cli -p $sockdir -i $ifname get_config
        done
    done

    for sockdir in $(find /var/run/wpa_supplicant-* -type d); do
        for ifname in $(ls $sockdir/); do
            collect_cmd $timeout 1 wpa_cli -p $sockdir -i $ifname status
            collect_cmd $timeout 1 wpa_cli -p $sockdir -i $ifname list_n
            collect_cmd $timeout 1 wpa_cli -p $sockdir -i $ifname scan_r
        done
    done
}

collect_sysinfo()
{
#    collect_cmd cat /.version
#    collect_cmd cat /.versions
    collect_cmd uptime

    collect_cmd ps
    collect_cmd top -n 1 -b
    collect_cmd free
    collect_cmd cat /proc/meminfo

    collect_cmd cat /proc/mtd
    collect_cmd mount
    collect_cmd df -k

    collect_cmd lspci
    collect_cmd dmesg

    collect_cmd mpstat -P ALL
    collect_cmd pidstat -p ALL
    collect_cmd pidstat -R -p ALL
    collect_cmd pidstat -R -p ALL
    collect_cmd cat /proc/loadavg
}

collect_ovs()
{
    collect_cmd ovsdb-client dump
    collect_cmd ovsdb-client -f json dump
    collect_cmd ovsdb-tool show-log "$LM_DIR_OVSDB_CFG"
    collect_cmd ovs-ofctl dump-flows br-home
    collect_cmd ovs-ofctl dump-flows br-wan
    collect_cmd ovs-appctl fdb/show br-home
    collect_cmd ovs-appctl fdb/show br-wan
    collect_cmd ovs-appctl mdb/show br-home
    collect_cmd ovs-appctl mdb/show br-wan
    collect_cmd ovs-appctl ovs/route/show
    collect_cmd ovs-appctl dpif/show
    collect_cmd ovs-appctl dpif/dump-flows br-home
    collect_cmd ovs-vsctl show
    collect_cmd ovs-vsctl list interface
    collect_cmd ovs-vsctl list bridge

    [ -e "$LM_DIR_OVSDB_CFG" ] && cp "$LM_DIR_OVSDB_CFG" "conf.db"
    [ -e "$LM_DIR_OVSDB_LOG" ] && cp "$LM_DIR_OVSDB_LOG" "ovsdb.log"
}

collect_network()
{
    collect_cmd ifconfig -a
    collect_cmd ip -d link show
    collect_cmd ip neigh show
    collect_cmd sh -c 'grep -H . /sys/class/net/*/softwds/*'
    collect_cmd route -n
    collect_cmd iptables -L -v -n
    collect_cmd iptables -t nat -L -v -n

    collect_cmd cat /proc/net/dev
    collect_cmd cat /var/etc/dnsmasq.conf
    collect_cmd cat /tmp/dhcp.leases
}

collect_bm()
{
    # This will put dbg events in log/messages
    killall -s SIGUSR1 bm
    sleep 1
}

collect_core_dump()
{
    # Collect all core dump files
    mkdir -p Core
    find /tmp/ -name '*core.gz' -exec mv -v '{}' Core/ ';'
}

collect_pcaps()
{
    # Collect all pcaps files
    echo "no pcaps"
}

collect_ramoops_log()
{
    # Collect ramops files
    echo "no ramops"
}

collect_alt_partition()
{
    # Collect part files
    echo "no alt_partition"
}

collect_cms()
{
    # Collect cms file
    collect_cmd bcmcli cms serial
    collect_cmd bcmcli cms entity
    collect_cmd bcmcli cms wan
    collect_cmd bcmcli cms opensync_mode
    collect_cmd bcmcli cms opensync_url
    collect_cmd bcmcli cms ntp
}

collect_adtran()
{
    # Collect Adtran file
    collect_cmd /opt/scripts/dumpsysinfo.sh
    collect_cmd cat /tmp/messages
}

collect_extra()
{
    # Collect extra
    echo "no extra"
}


# This function packs all the files in the working directory /var/log/tmp/logs_<time_stamp>
# and cleans it up once it is done with the job.
pack_all_logs_stats()
{
    mkdir -p $LM_DIR_TEMP
    mkdir -p $LM_DIR_LOG

    cd $LM_DIR_LOG

    collect_sysinfo
    collect_ovs
    collect_network
    collect_bm
    collect_hostap
    collect_alt_partition
    collect_core_dump
    collect_pcaps
    collect_ramoops_log
    collect_cms
    collect_adtran
    collect_extra

    for f in "$(ls $INSTALL_PREFIX/bin/lm_logs_collector.d/[0-9][0-9]*)"; do
        logger "[LOGPULL] executing $f"
        . "$f"
    done

    cd $LM_DIR_TEMP
    # Assuming files that previously failed to export are there, bundling them as well
    mv $LM_DIR_HOME/*.gz .
    tar cvzf system_logs_$LM_TIME_STAMP.tar.gz * 1>&2
    mv system_logs_$LM_TIME_STAMP.tar.gz $LM_DIR_HOME/
    rm * -rf

    # Initially when this feature started, it has been thought that we would
    # filter out special data/stats from syslogs, that would have been pumped into
    # syslogs, by managers on receipt of logging trigger. So, we have been collecting
    # a copy of syslogs accumulated during that window.  While this functionality of
    # gathering a copy is there in LM code, we are cleaning up this sub-dir,
    # /var/log/lm/syslog_copy here, at the end of bundling the accumulated logs.
    rm $LM_DIR_HOME/$LM_DIR_SYSLOG_COPY/* -rf
}


pack_all_logs_stats
lm_upload_logs
log "Done"
