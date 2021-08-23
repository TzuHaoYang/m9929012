#!/bin/sh
die() { log_warn "$*"; Healthcheck_Fail; }
pidof hostapd > /dev/null || die hostapd not found
pidof wpa_supplicant > /dev/null || die wpa_supplicant not found
Healthcheck_Pass
