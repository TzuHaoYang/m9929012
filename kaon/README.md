ssh

```
PORT : 22
ID : osync
PW : osync123
```

make:
```
./docker/dock-run 
make PROFILE=OS_EXTENDER_BCM52 OPENSYNC_SRC=<CODE PATH>/kaon_2_2_11_dev/opensync 
```

f/w upgrade:
```
tftp the image to /tmp then 
bcm_flasher /tmp/bcmOS_EXTENDER_BCM52_nand_fs_image_128_puresqubi.w
bcm_bootstate 1 
reboot
```
