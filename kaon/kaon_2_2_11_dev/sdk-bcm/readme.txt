Copyright 2020 Broadcom Corp.  Proprietary and confidential.

------------
Software Version:
impl69, WLAN software package version 17.10.157.2802

------------
Install:
- Extract the tarball over to the source tree

- 5.02L.07p1:
   Install patch
     $ ./apply_bsp_patch_all.sh
   Duplicate Archer binaries
     $ pushd bcmdrivers/broadcom/char/archer/impl1;for f in *impl61*; do cp $f ${f/impl61/impl69};done;popd



------------
Update Profile:

5.02L.07p1

- 63178/47622/675X: ./release/maketargets <PROFILE>_WL20D1D1

- else:             ./release/maketargets <PROFILE>_CPE_WL20D1D1

- make PROFILE=<PROFILE>


5.04L.02

- ./release/maketargets <PROFILE>_WL20D1D1

- make PROFILE=<PROFILE>


------------
MFG/DVT Test build switch (for applicable boot loader):
- Use regular firmware:

    CFE:   WLan Feature                      : 0x00

    uboot: => setenv wlFeature 0; saveenv

   Check wl ver command does NOT return "WLTEST" in the version string


- Use mfgtest firmware:

   CFE: WLan Feature                      : 0x02

   uboot: => setenv wlFeature 2; saveenv

   Check wl ver command should return "WLTEST" in the version string

------------
Additional notes:

Notes about mfgtest firmware:
   **********************************************************************************************************************************************
***Mfgtest firmware is to support capability required by WLAN hardware (MFGc/DVT) testing. For ANY other purposes, regular firmware should be used.***
   ***********************************************************************************************************************************************

-Default image will include both mfgtest and regular firmware, CFE "WLan Feature" selects which firmware to use
-If there is size limitation or else requirement to remove mfgtest firmware:
  -BUILD_BCM_WLAN_NO_MFGBIN option (WLNOMFGBIN.arch) is available to delete mfgtest firmware from target directory
-If source change required by mfgtest is made, mfgtest prebuilt binary should be rebuilt to take effect:
  e.g. #make PROFILE=<profile> WLTEST=1; #this builds and saves mfgtest wlan binaries in the source tree, the image built at this point is mfgtest capable
       #make clean;                      #clean up before switch to normal build after building with WLTEST=1
       #make PROFILE=<profile>;          #go on with normal builds as usual,there is no need to rebuild with WLTEST=1
                                         #until the time mfgtest binary needs to be updated because of source changes.

------------
Emphasize again because it is VERY important:

Please make sure that non-MFGTEST image is used before starting any WFA certification testing.
For MFGTEST image (not for WFA certification testing), the wl ver shows WLTEST, e.g.,

# wl ver
Broadcom BCA: 17.10 RC121.11
wl0: Feb  6 2020 17:00:26 version 17.10.121.11 (r783116 WLTEST) FWID 01-88eb0b67


For non-MFGTEST image (good for WFA certificaiton testing), the wl ver does not have WLTEST, e.g.,
# wl ver
Broadcom BCA: 17.10 RC121.11
wl0: Feb  6 2020 17:17:05 version 17.10.121.11 (r783116) FWID 01-b12d23ed

-Mfgtest firmware is to support capability required by WLAN hardware (MFGc/DVT) testing. 
 For software/field/certification testing and final production, regular firmware should be used.
