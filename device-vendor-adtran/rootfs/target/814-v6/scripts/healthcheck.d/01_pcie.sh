#!/bin/sh
let pci_cnt="$(lspci | wc -l)"
if [ $pci_cnt -eq 0 ]; then
    Healthcheck_Fail
fi

Healthcheck_Pass
