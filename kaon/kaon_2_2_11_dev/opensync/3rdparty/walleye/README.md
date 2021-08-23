device-3rdparty-walleye
======================

Walleye
-------------------------

**Integration steps:**

1. Unpack to `3rdparty/walleye`

2. Create target symlinks in: `3rdparty/walleye/rootfs/target/`

   e.g.  
   `cd 3rdparty/walleye/rootfs/target/ && ln -s bcm52-arm-linux-glibcgnueabi <TARGET>`  
   where `<TARGET>` is your target name

3. Include 3rdparty layers in `vendor/<VENDOR>/`
   
   by making sure that in `vendor/<VENDOR>/` in appropriate `build/<makefile>.mk` 
   there is a line or lines that will include 3rdparty addons makefiles.

   e.g.  
   `vendor/<VENDOR>/build/3rdparty.mk: -include 3rdparty/*/build/build.mk`

   `3rdparty/*/build/build.mk` will add the 3rdparty addon to the build layers 
   conditionally to Kconfig.
   
   Make sure that `kconfig/Kconfig.3rdparty` is added to menuconfig.

4. Enable/disable Kconfig for the 3rdparty addon 
   
   in appropriate `vendor/<VENDOR>` Kconfig files: 

   e.g.  
   `vendor/<VENDOR>/kconfig/target/<TARGET>: CONFIG_3RDPARTY_WALLEYE=y`

   This way if a Kconfig option for a specific 3rdparty addon is enabled
   that addon will add itself to the build layers (binaries will be added to
   rootfs and any possible sources added to the build).

5. Sanity check

   * Make sure `libfsm_walleye_dpi.so` and other binary artifacts are included 
     in image rootfs.

