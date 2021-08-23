#!/bin/sh

case "$1" in
        start)
                echo "SPU registering crypto algs"
                spuctl register 
                exit 0
                ;;

        stop)
                echo "SPU unregistering crypto algs"
		spuctl unregister
                exit 0
                ;;

        *)
                echo "$0: unrecognized option $1"
                exit 1
                ;;

esac
