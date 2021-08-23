#!/bin/sh

bcmwl_list_vifs()
{
    ls /sys/class/net | grep ^wl
}

bcmwl_list_phys()
{
    ls /sys/class/net | grep ^wl | grep -vF .
}

collect_bcmwl()
{
    collect_cmd nvram getall
    collect_cmd cat /data/.kernel_nvram.setting
    for i in $(bcmwl_list_vifs); do
        collect_cmd wl -i $i assoc
        collect_cmd wl -i $i chanspec
        collect_cmd wl -i $i curpower
        collect_cmd wl -i $i PM
        for sta in $(wl -i $i assoclist | cut -d' ' -f2)
        do
            collect_cmd wl -i $i sta_info $sta
        done
    done
    for i in $(bcmwl_list_phys); do
        collect_cmd wl -i $i muinfo
        collect_cmd wl -i $i muinfo -v
        collect_cmd wl -i $i mu_policy
        collect_cmd wl -i $i mu_features
        collect_cmd wl -i $i max_muclients
        collect_cmd wl -i $i he enab
        collect_cmd wl -i $i he features
        collect_cmd wl -i $i he bsscolor
        collect_cmd wl -i $i he range_ext
        collect_cmd wl -i $i twt enab
        collect_cmd wl -i $i twt list
        collect_cmd wl -i $i dfs_status
        collect_cmd wl -i $i dfs_status_all
        collect_cmd wl -i $i chan_info
        collect_cmd wl -i $i scanresults
        collect_cmd wl -i $i chanim_stats
        collect_cmd wl -i $i chanspecs
        collect_cmd wl -i $i chanspec_txpwr_max
        collect_cmd wl -i $i txpwr
        collect_cmd wl -i $i txpwr1
        collect_cmd wl -i $i txpwr_target_max
        collect_cmd wl -i $i curppr
        collect_cmd sh -c "old=\$(dhdctl -i $i dconpoll); dhdctl -i $i dconpoll 100; sleep 1; dmesg; dhdctl -i $i dconpoll \$old"
    done

    collect_cmd cat /proc/fcache/misc/host_dev_mac
    collect_cmd ls -al /data

    if [ -e /etc/patch.version ]; then
        collect_cmd cat /etc/patch.version
    fi
}

collect_flowcache()
{
    find /proc/fcache/ -type f -exec echo {} \; -exec cat {} \; > /tmp/fcache_logs
    mv /tmp/fcache_logs "$LM_DIR_LOG"/_tmp_fcache_logs
    collect_cmd fcctl status
}

collect_platform_bcm()
{
    collect_bcmwl
    collect_flowcache
}

collect_platform_bcm
