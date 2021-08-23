#!/bin/sh
# {# jinja-parse #}
INSTALL_PREFIX={{INSTALL_PREFIX}}

[ -z "$2" ] && echo "Error: should be run by odhcpc6c" && exit 1

. ${INSTALL_PREFIX}/bin/dns_sub.sh

OPTS_FILE=/var/run/odhcp6c_$1.opts

#
# Remove the "prefix" part from a `ip -6 route` command. For example:
#
#     "fe80::/64 dev br-home proto kernel metric 256"
#       returns
#     "dev br-home proto kernel metric 256"
#
#     "unreachable fd36:a253:1d8e::/48 dev lo proto static metric 2147483647 error -113"
#       returns
#     "dev lo proto static metric 2147483647 error -113"
#
ip6route_no_prefix()
{
    case "$1" in
        unreach*)
            shift
    esac
    shift

    echo "$@"
}

#
# Take a `ip -6 route` command and return the `SELECTOR` (ip -6 route help) part
# of the command line. In essence, this command takes the arguments and strips
# them according to a white list.
#
ip6route_get_select()
{
    # The first argument to an "ip route" command can be "unreachable" -- this
    # keyword cannot be used as a route selector
    case "$1" in
        unreach*)
            shift
    esac

    # The first argument is the route prefix
    sel="$1"
    shift

    while [ -n "$1" ]
    do
        case "$1" in
            via|table|dev|proto|type|scope)
                sel="${sel} $1 $2"
                shift
                ;;
        esac
        shift
    done

    echo "$sel"
}

#
# Map pairs of arguments to key=value pairs
#
ip6route_to_keyvalue()
{
    while [ -n "$1" ]
    do
        printf "%s=%s " "$1" "$2"
        shift 2 || break
    done
}

ip6route_check()
{
    sel=$(ip6route_get_select "$@")
    route=$(ip6route_no_prefix $(ip -6 route show $sel | head -1))

    # `ip -6 route SELECTION` will show only fields that are not already present
    # in SELECTION so pass both `sel` and `route` to ip6route_to_keyvalue().
    # Both the `sel` and `route` variables start with the prefix address, strip
    # it.
    COPT="$(ip6route_to_keyvalue $(ip6route_no_prefix $sel) $route)"

    for O in $(ip6route_to_keyvalue $(ip6route_no_prefix "$@"))
    do
        if ! echo "$COPT" | grep -qE "(^| )$O($| )"
        then
            return 1
        fi
    done
    return 0
}

# Check if a similar route already exist, if it does, do nothing
ip6route_replace()
{
    ip6route_check "$@" && return 0
    ip -6 route replace "$@"
}

update_resolv()
{
    dns_reset "$1_ipv6"
    [ -n "$2" ] && dns_add "$1_ipv6" "nameserver $2"
    dns_apply "$1_ipv6"
}

setup_interface()
{
    local device="$1"

    # Merge RA-DNS
    for radns in $RA_DNS; do
        local duplicate=0
        for dns in $RDNSS; do
            [ "$radns" = "$dns" ] && duplicate=1
        done
        [ "$duplicate" = 0 ] && RDNSS="$RDNSS $radns"
    done

    local dnspart=""
    for dns in $RDNSS; do
        if [ -z "$dnspart" ]; then
            dnspart="\"$dns\""
        else
            dnspart="$dnspart, \"$dns\""
        fi
    done

    update_resolv "$device" "$dns"

    local prefixpart=""
    for entry in $PREFIXES; do
        local addr="${entry%%,*}"
                entry="${entry#*,}"
                local preferred="${entry%%,*}"
                entry="${entry#*,}"
                local valid="${entry%%,*}"
                entry="${entry#*,}"
        [ "$entry" = "$valid" ] && entry=

        local class=""
        local excluded=""

        while [ -n "$entry" ]; do
            local key="${entry%%=*}"
                    entry="${entry#*=}"
            local val="${entry%%,*}"
                    entry="${entry#*,}"
            [ "$entry" = "$val" ] && entry=

            if [ "$key" = "class" ]; then
                class=", \"class\": $val"
            elif [ "$key" = "excluded" ]; then
                excluded=", \"excluded\": \"$val\""
            fi
        done

        local prefix="{\"address\": \"$addr\", \"preferred\": $preferred, \"valid\": $valid $class $excluded}"

        if [ -z "$prefixpart" ]; then
            prefixpart="$prefix"
        else
            prefixpart="$prefixpart, $prefix"
        fi

        # TODO: delete this somehow when the prefix disappears
        ip6route_replace unreachable "$addr" proto ra
    done

    # Merge addresses
    for entry in $RA_ADDRESSES; do
        local duplicate=0
        local addr="${entry%%/*}"
        for dentry in $ADDRESSES; do
            local daddr="${dentry%%/*}"
            [ "$addr" = "$daddr" ] && duplicate=1
        done
        [ "$duplicate" = "0" ] && ADDRESSES="$ADDRESSES $entry"
    done

    for entry in $ADDRESSES; do
        local addr="${entry%%,*}"
        entry="${entry#*,}"
        local preferred="${entry%%,*}"
        entry="${entry#*,}"
        local valid="${entry%%,*}"

        ip -6 address replace "$addr" dev "$device" preferred_lft "$preferred" valid_lft "$valid"
    done


    for entry in $RA_ROUTES; do
        local addr="${entry%%,*}"
        entry="${entry#*,}"
        local gw="${entry%%,*}"
        entry="${entry#*,}"
        local valid="${entry%%,*}"
        entry="${entry#*,}"
        local metric="${entry%%,*}"

        if [ -n "$gw" ]; then
            ip6route_replace "$addr" via "$gw" metric "$metric" dev "$device" proto ra
        else
            ip6route_replace "$addr" metric "$metric" dev "$device" proto ra
        fi

        for prefix in $PREFIXES; do
            local paddr="${prefix%%,*}"
            [ -n "$gw" ] && ip6route_replace "$addr" via "$gw" metric "$metric" dev "$device" from "$paddr" proto ra
        done
    done

    # Apply hop-limit
    [ -n "$RA_HOPLIMIT" -a "$RA_HOPLIMIT" != "0" -a "/proc/sys/net/ipv6/conf/$device/hop_limit" ] && {
        echo "$RA_HOPLIMIT" > "/proc/sys/net/ipv6/conf/$device/hop_limit"
    }

    export > "$OPTS_FILE"
}

teardown_interface()
{
    rm -f "$OPTS_FILE"
    local device="$1"
    ip -6 route flush dev "$device" proto ra
    ip -6 address flush dev "$device" scope global
    update_resolv "$device" ""
}

(
    case "$2" in
        bound)
            teardown_interface "$1"
            setup_interface "$1"
        ;;
        informed|updated|rebound|ra-updated)
            setup_interface "$1"
        ;;
        stopped|unbound)
            teardown_interface "$1"
        ;;
        started)
            teardown_interface "$1"
        ;;
    esac

    # user rules
    [ -f /etc/odhcp6c.user ] && . /etc/odhcp6c.user
) 9>/tmp/odhcp6c.lock.$1
rm -f /tmp/odhcp6c.lock.$1
