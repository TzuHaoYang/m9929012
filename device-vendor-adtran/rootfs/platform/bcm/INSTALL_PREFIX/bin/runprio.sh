#!/bin/sh

# run a command with increased process priority
# and temporary elevated system rt_runtime

RT_RUNTIME=95000
CHRT_PRIO="-r 5"
SIGNALS="INT HUP TERM EXIT QUIT"

restore_rt_runtime()
{
    RET=$?
    if [ -n "$ORIG_RT_RUNTIME" ]; then
        logger "[$PPID]: $0: restoring sched_rt_runtime to $ORIG_RT_RUNTIME"
        echo "$ORIG_RT_RUNTIME" > /proc/sys/kernel/sched_rt_runtime_us
    fi
    trap - $SIGNALS
    exit $RET
}

# temporary elevate sched_rt_runtime
ORIG_RT_RUNTIME=$(grep "BRCM_SCHED_RT_RUNTIME=" /etc/build_profile 2>/dev/null | cut -d= -f2)
if [ -n "$ORIG_RT_RUNTIME" -a "$RT_RUNTIME" -gt "$ORIG_RT_RUNTIME" ]; then
    logger "[$PPID]: $0: elevating sched_rt_runtime to $RT_RUNTIME"
    trap restore_rt_runtime $SIGNALS
    echo "$RT_RUNTIME" > /proc/sys/kernel/sched_rt_runtime_us
else
    logger "[$PPID]: $0: WARNING: not changing sched_rt_runtime $ORIG_RT_RUNTIME to $RT_RUNTIME"
fi

# if args start with - pass them to chrt
if [ "${1:0:1}" = "-" ]; then
    CHRT_PRIO=
fi

# run command with increased prio
logger "[$PPID]: $0: /usr/bin/chrt $CHRT_PRIO $*"
/usr/bin/chrt $CHRT_PRIO "$@"

# restore original sched_rt_runtime
restore_rt_runtime

