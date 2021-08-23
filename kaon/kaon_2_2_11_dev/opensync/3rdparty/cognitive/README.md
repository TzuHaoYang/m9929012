device-3rdparty-cognitive
-------------------------

RF motion sensing
Version: 3.6.1

Changelist:
- update localizer
- addressing COGEXT-171
- addressing COGEXT-140
- add cpu and memory monitoring metric
- updated list of approved CA
- leafblower syntax error bug
- modifications of startup code for Askey AP5620W

Cognitive userspace code
- binary files deliverables for Cognitive rf sensing software

Build and software requirements:
- cognitive motion80211 patch included in qsdk build
- mosquitto added in part with the build
- CMM (csc_man), cognitive motion manager included in the build inside Kconfig

Cloud requirements:
- WifiMotion set to auto: true

Contents:
```
- rootfs/target/PIRANHA2_QSDK53
  -- csc
  ---- bin
  ------ borgmon.service
  ------ micropython
  ------ csc_root.pem
  ------ mosquitto.service
  ------ run
  ------ rx_motion
  ------ rx_motion.service
  -- lib
  ------ ld-linux-armhf.so.3
  ------ libc.so.6
  ------ libdl.so.2
  ------ libffi.so.6
  ------ libgcc_s.so.1
  ------ libm.so.6
  ------ libnss_dns.so.2
  ------ libnss_files.so.2
  ------ libpthread.so.0
  ------ libresolv.so.2
  ------ librt.so.1
  -- etc
  ---- init.d
  ------ borg
  ------ mosquitto
  ------ rx_motion
  ---- version
- src/csc_man
  -- inc
  ---- csc_man.h
  -- src
  ---- csc_man_main.c
  ---- csc_man_motion.c
  ---- csc_man_ovsdb.c
  ---- csc_man_cert.c
  -- unit.mk
- src/lib/target/kconfig
  -- Kconfig.managers

```
