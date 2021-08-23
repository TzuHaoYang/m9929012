#!/bin/sh
#
# Check NTP to see if it's sync'd or not.  Dumb check, but seems to work...

let cur_tm="$(date +"%s")"
[ $cur_tm -gt 1000000 ] && Healthcheck_Pass

Healthcheck_Fail
