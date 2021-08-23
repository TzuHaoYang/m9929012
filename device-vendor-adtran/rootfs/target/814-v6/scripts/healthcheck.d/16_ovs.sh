#!/bin/sh
# Script checks if ovs-vswitchd is running without RCU warnings causing service effectively to hang
# ovs-vswitchd uses Read-Copy-Update algorithm to enable concurrent non-blocking access to memory
# resources, but sometimes one of reading threads does not release reading lock and effectively
# RCU algorithm cannot work properly and the service goes into deadlock state prining timeout warnings
# to the system log. This script detects this situation analysing system logs and runs recovery action

LOG_MODULE="OVS_Chk"
LOGREAD=$(which logread)
MESSAGES=/tmp/messages

# log time-stamp length
TSLEN=15

# RCU blocking time error threshold in [ms]
RCU_BLOCK_ERR_THR=$((60 * 1000))

# RCU blocking time warning threshold in [ms]
RCU_BLOCK_WARN_THR=$(($RCU_BLOCK_ERR_THR / 2))

# name of monitored process
PROC_NAME="ovs-vswitchd"

# function expects arg1=process name and returns PID of the process
get_pid()
{
    pgrep $1
}

# Function returns last matching log line of RCU blocked warning from local logs
# Local log is expected to have following format of time and warning message:
# Aug 12 12:00:07 ??? ovs_rcu ??? blocked nnn ms waiting for ??? to quiesce
#
get_rcu_log_line()
{
    if [ ! -z $LOGREAD ]; then
        $LOGREAD -p $PROC_NAME | egrep 'ovs_rcu.+blocked [[:digit:]]+ ms waiting for \w+ to quiesce' | tail -n 1
    else
        if [ -e $MESSAGES ]; then
            cat $MESSAGES | egrep 'ovs_rcu.+blocked [[:digit:]]+ ms waiting for \w+ to quiesce' | tail -n 1
        fi
    fi
}

# Function returns log time in seconds since epoch start
# Expects arg1=log line to extract time from in following format: Aug 12 12:00:02 ....
get_log_time_sec()
{
    # extract time stamp from log line
    _tstamp=${1:0:$TSLEN}
    # convert stamp to seconds since epoch start
    date -D "%b %d %H:%M:%S" -d "$_tstamp" +"%s"
}

# Gets age of log line in seconds, expects args1 to be log line
get_log_age()
{
    # compute log age in seconds
    echo $(( $(date +"%s") - $(get_log_time_sec "$1") ))
}

# extracts blocking time from ovs-vswitchd RCU blocking warning log
# expects arg1 to be log line, returns blocking time in [ms] units
get_blocking_time()
{
    bt=$(echo $1 | egrep -o 'blocked .+' | egrep -o ' [[:digit:]]+ ms')
    # remove trailing ' ms' and return
    echo ${bt:0:$((${#bt}-3))}
}

# Process ID used in module scope
PID=

check_fail()
{
    msg="$PROC_NAME warning."
    if [ $# -gt 0 ]; then
        msg="${msg} $*"
    fi
    log_warn $msg
    Healthcheck_Fail
}

check_fatal()
{
    msg="$PROC_NAME fatal error."
    if [ $# -gt 0 ]; then
        msg="${msg} $*"
    fi
    log_err $msg " Rebooting POD..."
    # start rebooting the POD
    Healthcheck_Fatal
    # return negative check result
    Healthcheck_Fail
}

check_pass()
{
    Healthcheck_Pass
}

ovs_vswitchd_check()
{
    PID=$(get_pid $PROC_NAME)
    # when service PID not found report error
    if [ -z "$PID" ]; then
        check_fail "$PROC_NAME is dead, no PID found."
    fi

    # read last rcu warning log line printed by the service
    line=$(get_rcu_log_line)

    # check if at least one warning log was found
    if [ -z "$line" ]; then
        check_pass
    fi

    # compute age of this log line
    age=$(get_log_age "$line")

    # detect old log in case service was restarted, but log remained
    if [ $age -ge $HEALTHCHECK_INTERVAL ]; then
        check_pass
    fi

    # extract RCU blocking time, wait at least threshold time to raise error
    blocktime=$(get_blocking_time "$line")
    if [ $blocktime -lt $RCU_BLOCK_WARN_THR ]; then
        # warning timeout still acceptable
        check_pass
    fi

    # service warning OR error
    if [ $blocktime -lt $RCU_BLOCK_ERR_THR ]; then
        # warning timeout still acceptable, but check should fail with warning
        check_fail "RCU deadlock detected"
    else
        # deadlock timeout: fatal error
        check_fatal "RCU max deadlock time exceeded $(($RCU_BLOCK_ERR_THR/1000)) sec"
    fi
}

ovs_vswitchd_check
