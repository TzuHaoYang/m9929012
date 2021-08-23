DF_CRIT_LOW="15"    # % of filesystem free
DF_WARN_LOW="30"    # % of filesystem free
LOG_MODULE="TMP_DF"

# Calcualte free space on /tmp, express it as %
DF=$(df -k /tmp | tail -n +2 | awk '{printf("%0.0f", $4 / $2 * 100.0)}')

if [ "${DF}" -le ${DF_CRIT_LOW} ]
then
    log_err "/tmp free space below ${DF_CRIT_LOW}% (${DF}% free). Listing top 15 offenders:"
    find /tmp -xdev -type f -exec du -k {} \; | sort -nr | head -n 15 | log_err -
    Healthcheck_Fatal
elif [ "${DF}" -le ${DF_WARN_LOW} ]
then
    log_warn "/tmp is getting low on space (${DF}% free)."
fi

Healthcheck_Pass
