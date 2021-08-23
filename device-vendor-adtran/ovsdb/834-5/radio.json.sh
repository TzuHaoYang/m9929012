##
# Pre-populate WiFi related OVSDB tables
#

generate_onboarding_ssid()
{
    cat << EOF
        "$OS_ONBOARDING_SSID"
EOF
}

generate_onboarding_psk()
{
    cat << EOF
        ["map",
            [
                ["encryption","WPA-PSK"],
                ["key", "$OS_ONBOARDING_PSK"]
            ]
       ]
EOF
}

cat << EOF
[
    "Open_vSwitch",
    {
        "op": "insert",
        "table": "Wifi_Radio_Config",
        "row": {
            "enabled": true,
            "if_name": "phy1",
            "freq_band": "2.4G",
            "channel": 1,
            "channel_mode": "cloud",
            "channel_sync": 0,
            "hw_type": "mt7622",
            "ht_mode": "HT20",
            "hw_mode": "11n",
            "vif_configs": ["set", [ ["named-uuid", "id0"] ] ]
        }
    },
    {
        "op": "insert",
        "table": "Wifi_Radio_Config",
        "row": {
            "enabled": true,
            "if_name": "phy0",
            "freq_band": "5G",
            "channel": 44,
            "channel_mode": "cloud",
            "channel_sync": 0,
            "hw_type": "mt7615",
            "ht_mode": "HT80",
            "hw_mode": "11ac",
            "dfs_demo": false,
            "vif_configs": ["set", [ ["named-uuid", "id1"] ] ]
        }
    },
    {
        "op": "insert",
        "table": "Wifi_VIF_Config",
        "row": {
            "enabled": true,
            "mode": "ap",
            "if_name": "wifi2g",
            "bridge": "br-lan",
            "vif_dbg_lvl": 0,
            "vif_radio_idx": 2,
            "btm": 1,
            "ap_bridge": false,
            "ssid": "834Cloud2G",
            "security": ["map",[["encryption","WPA-PSK"],["key","834cloud2G"],["mode","2"],["oftag","home--1"]]]
        },
        "uuid-name": "id0"
    },
    {
        "op": "insert",
        "table": "Wifi_VIF_Config",
        "row": {
            "enabled": true,
            "mode": "ap",
            "if_name": "wifi5g",
            "bridge": "br-lan",
            "vif_dbg_lvl": 0,
            "vif_radio_idx": 2,
            "btm": 1,
            "ap_bridge": false,
            "ssid": "834Cloud5G",
            "security": ["map",[["encryption","WPA-PSK"],["key","834cloud5G"],["mode","2"],["oftag","home--1"]]]
        },
        "uuid-name": "id1"
    }
]
EOF
