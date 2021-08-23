#!/bin/sh

case "$1" in
	start)
      echo "Starting Micro HTTPD Daemon..."
      /bin/mhd 8000 &
		exit 0
		;;

	stop)
		echo "Stopping Micro HTTPD Daemon..."
		killall mhd
      exit 0
		;;

	*)
		echo "$0: unrecognized option $1"
		exit 1
		;;

esac

