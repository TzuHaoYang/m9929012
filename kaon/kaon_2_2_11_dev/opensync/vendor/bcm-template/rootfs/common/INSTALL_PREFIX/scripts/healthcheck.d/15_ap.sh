#!/bin/sh
OVSH=$CONFIG_INSTALL_PREFIX/tools/ovsh

# The device eventually is expected to have at least one AP
# interface created by the external ovs controller. Not
# having any AP interfaces is considered an error and may
# need a recovery.
$OVSH -r s Wifi_VIF_Config -w mode==ap if_name \
    | grep -q .
