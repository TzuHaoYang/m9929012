#!/bin/sh
die() { log_warn "$*"; Healthcheck_Fail; }
pidof hostapd || die hostapd not found
pidof wpa_supplicant wpa_supplicant not found
Healthcheck_Pass
