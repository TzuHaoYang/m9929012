#ifndef __HCO_RIP_PRIV_H__
#define __HCO_RIP_PRIV_H__

// configurable key length to be used in the configuration
#define MAX_HEX_KEY_128           26 // 104 bit key
#define MAX_HEX_KEY_64            10 // 40 bit key

/*
 * normal RIP: IDs in range [0x0001, 0x00ff]
 */

// bootloader version are the 4 bytes just before the rip [20,24[
#define RIP_PRIV_BOOTLOADER_VERSION      0x0000 // Bootloader version, Not in the RIP
#define RIP_PRIV_CUSTOMIZATION_PATTERN   0x0001
#define RIP_PRIV_PBA_CODE_AND_VARIANT    0x0002
#define RIP_PRIV_PBA_ICS                 0x0003
#define RIP_PRIV_SERIAL_NUMBER           0x0004
#define RIP_PRIV_FACTORY_RELEASE_DATE    0x0005
#define RIP_PRIV_FIA                     0x0006
#define RIP_PRIV_FORMAT_ID_FIM           0x0007
#define RIP_PRIV_DATE_LAST_REPAIR        0x0008
#define RIP_PRIV_WAN_ADDR                0x0009
#define RIP_PRIV_ALCATEL_COMPANY_ID      0x000a
#define RIP_PRIV_FACTORY_ID              0x000b
#define RIP_PRIV_BOARD_NAME              0x000c
#define RIP_PRIV_MEMORY_CONFIG_1         0x000d
#define RIP_PRIV_MEMORY_CONFIG_2         0x000e
#define RIP_PRIV_MEMORY_CONFIG_3         0x000f
#define RIP_PRIV_MEMORY_CONFIG_4         0x0010
#define RIP_PRIV_LOCAL_NETWORK_ADDRESS_2 0x0011
#define RIP_PRIV_NUMBER_OF_USED_PVCS     0x0012
#define RIP_PRIV_VPI_PVC_1               0x0013
#define RIP_PRIV_VCI_PVC_1               0x0014
#define RIP_PRIV_VPI_PVC_2               0x0015
#define RIP_PRIV_VCI_PVC_2               0x0016
#define RIP_PRIV_VPI_PVC_3               0x0017
#define RIP_PRIV_VCI_PVC_3               0x0018
#define RIP_PRIV_VPI_PVC_4               0x0019
#define RIP_PRIV_VCI_PVC_4               0x001a
#define RIP_PRIV_VPI_PVC_5               0x001b
#define RIP_PRIV_VCI_PVC_5               0x001c
#define RIP_PRIV_VPI_PVC_6               0x001d
#define RIP_PRIV_VCI_PVC_6               0x001e
#define RIP_PRIV_VPI_PVC_7               0x001f
#define RIP_PRIV_VCI_PVC_7               0x0020
#define RIP_PRIV_VPI_PVC_8               0x0021
#define RIP_PRIV_VCI_PVC_8               0x0022
#define RIP_PRIV_VPI_PVC_9               0x0023
#define RIP_PRIV_VCI_PVC_9               0x0024
#define RIP_PRIV_VPI_PVC_10              0x0025
#define RIP_PRIV_VCI_PVC_10              0x0026
#define RIP_PRIV_VPI_PVC_11              0x0027
#define RIP_PRIV_VCI_PVC_11              0x0028
#define RIP_PRIV_VPI_PVC_12              0x0029
#define RIP_PRIV_VCI_PVC_12              0x002a
#define RIP_PRIV_VPI_PVC_13              0x002b
#define RIP_PRIV_VCI_PVC_13              0x002c
#define RIP_PRIV_VPI_PVC_14              0x002d
#define RIP_PRIV_VCI_PVC_14              0x002e
#define RIP_PRIV_VPI_PVC_15              0x002f
#define RIP_PRIV_VCI_PVC_15              0x0030
#define RIP_PRIV_VPI_PVC_16              0x0031
#define RIP_PRIV_VCI_PVC_16              0x0032
#define RIP_PRIV_MODEM_ACCESS_CODE       0x0033
#define RIP_PRIV_SECURE_REMOTE_MGMT_PSWD 0x0034
#define RIP_PRIV_LOCAL_NETWORK_ADDRESS_3 0x0035
#define RIP_PRIV_HARDWARE_VERSION        0x0036
#define RIP_PRIV_PUBLIC_DSA_NORM         0x0037

#define RIP_PRIV_WLAN_BRCM_SROM_MAP      0x0038
#define RIP_PRIV_WLAN_BRCM_SROM_MAP2     0x0039
#define RIP_PRIV_FIRSTUSE                0x003a
#define RIP_PRIV_FACTORY                 0x003b
#define RIP_PRIV_ADMIN_PWD               0x003c
#define RIP_PRIV_WLAN_SSID               0x003d

#if defined(CONFIG_CUSTOM_MAXIS) || defined(CONFIG_CUSTOM_CELCOM)
#define RIP_PRIV_SPT_PWD                 0x003e
#define RIP_PRIV_USER_PWD                0x003f
#define RIP_PRIV_RIP_VERSION             0x0040
#endif

#if defined(CONFIG_CUSTOM_PLUME)
#define RIP_PRIV_CA_PEM_KEY              0x0041
#define RIP_PRIV_CLIENT_PEM_KEY          0x0042
#define RIP_PRIV_DEC_KEY                 0x0043
#define RIP_PRIV_SEED                    0x0044
#define RIP_PRIV_MFG_DATE                0x0045
#endif

/*
 * extended RIP: IDs > 0x00ff
 */
#define RIP_PRIV_PUBLIC_DSA_SYST         0x0100 /* DSA Public Key, for Regular Firmware Signature */
#define RIP_PRIV_PUBLIC_DSA_RESC         0x0101 /* DSA Public Key, for Rescue Firmware Signature */
#define RIP_PRIV_MODELNAME               0x0102 /* String, Represents the board (ex: Livebox Mini...) */
#define RIP_PRIV_PRODUCT_CLASS           0x0103 /* String, Represents the product type (ex: Livebox, Livebox2...) */
#define RIP_PRIV_CLIENT_CERTIFICATE      0x0104 /* PEM (x509v3), Unique Device Certificate */
#define RIP_PRIV_PRIVATE_KEY             0x0105 /* PEM (PKCS#1), RSA Private Key (unique per device) */
#define RIP_PRIV_H235_KEY                0x0106 /* String, Secret Key (unique per device) for voice activation */


/*
 * Some fields are not stored in the rip or extended rip,
 * but they are calculated or hardcoded. Nevertheless they
 * have an id assigned to be able to retrieve them via the rip api.
 */
#define RIP_PRIV_MANUFACTURER            0x1000 /* HARDCODED: String, Board Manufacturer */
#define RIP_PRIV_MANUFACTURER_OUI        0x1001 /* CALCULATED: RAW Data, Board Manufacturer OUI (ex:"0090D0") */
#define RIP_PRIV_MANUFACTURER_URL        0x1002 /* HARDCODED: String, Board Manufacturer Web Site URL */

#define RIP_PRIV_WLAN_KEY                0x1003 /* CALCULATED from serial number */
#define RIP_PRIV_WLAN_BSSID              0x1004 /* CALCULATED? only if it is stored in RIP, else return 0xffffffffffff */

// maximum sizes for the various fields
#define RIP_PRIV_BOOTLOADER_VERSION_SIZE        12
#define RIP_PRIV_CUSTOMIZATION_PATTERN_SIZE      2
#define RIP_PRIV_PBA_CODE_AND_VARIANT_SIZE      12
#define RIP_PRIV_PBA_ICS_SIZE                    2
#define RIP_PRIV_SERIAL_NUMBER_SIZE             24 
#define RIP_PRIV_FACTORY_RELEASE_DATE_SIZE       6
#define RIP_PRIV_FIA_SIZE                        2
#define RIP_PRIV_FORMAT_ID_FIM_SIZE              2
#define RIP_PRIV_DATE_LAST_REPAIR_SIZE           6
#define RIP_PRIV_WAN_ADDR_SIZE                   6
#define RIP_PRIV_ALCATEL_COMPANY_ID_SIZE         4
#define RIP_PRIV_FACTORY_ID_SIZE                 4
#define RIP_PRIV_BOARD_NAME_SIZE                 8
#define RIP_PRIV_MEMORY_CONFIG_1_SIZE            1
#define RIP_PRIV_MEMORY_CONFIG_2_SIZE            1
#define RIP_PRIV_MEMORY_CONFIG_3_SIZE            1
#define RIP_PRIV_MEMORY_CONFIG_4_SIZE            1
#define RIP_PRIV_LOCAL_NETWORK_ADDRESS_2_SIZE    6
#define RIP_PRIV_NUMBER_OF_USED_PVCS_SIZE        1
#define RIP_PRIV_VPI_PVC_SIZE                    1
#define RIP_PRIV_VCI_PVC_SIZE                    2
#define RIP_PRIV_MODEM_ACCESS_CODE_SIZE          5
#define RIP_PRIV_SECURE_REMOTE_MGMT_PSWD_SIZE    5
#define RIP_PRIV_LOCAL_NETWORK_ADDRESS_3_SIZE    6

#define RIP_PRIV_PUBLIC_DSA_SYST_SIZE          404
#define RIP_PRIV_PUBLIC_DSA_RESC_SIZE          404
#define RIP_PRIV_MODELNAME_SIZE                 64
#define RIP_PRIV_PRODUCT_CLASS_SIZE             64
#define RIP_PRIV_CLIENT_CERTIFICATE_SIZE      2048
#define RIP_PRIV_PRIVATE_KEY_SIZE             2048
#define RIP_PRIV_H235_KEY_SIZE                  16
#define RIP_PRIV_HARDWARE_VERSION_SIZE          64
#define RIP_PRIV_PUBLIC_DSA_NORM_SIZE          404

#define RIP_PRIV_WLAN_BRCM_SROM_MAP_SIZE       468
#define RIP_PRIV_WLAN_BRCM_SROM_MAP2_SIZE     1180
#define RIP_PRIV_FIRSTUSE_SIZE                   8

#define RIP_PRIV_MANUFACTURER_SIZE              64
#define RIP_PRIV_MANUFACTURER_OUI_SIZE           3
#define RIP_PRIV_MANUFACTURER_URL_SIZE          64

#define RIP_PRIV_WLAN_KEY_SIZE     MAX_HEX_KEY_128
#define RIP_PRIV_WLAN_BSSID_SIZE                 6
#define RIP_PRIV_FACTORY_SIZE                    2
#define RIP_PRIV_ADMIN_PWD_SIZE                139
#define RIP_PRIV_WLAN_SSID_SIZE                 33

#if defined(CONFIG_CUSTOM_MAXIS) || defined(CONFIG_CUSTOM_CELCOM)
#define RIP_PRIV_SPT_PWD_SIZE                  139
#define RIP_PRIV_USER_PWD_SIZE                 139
#define RIP_PRIV_RIP_VERSION_SIZE              64
#endif

#if defined(CONFIG_CUSTOM_PLUME)
#define RIP_PRIV_CA_PEM_KEY_SIZE              1024*7
#define RIP_PRIV_CLIENT_PEM_KEY_SIZE	      1024*2
#define RIP_PRIV_DEC_KEY_SIZE                 512
#define RIP_PRIV_SEED_SIZE                    16
#define RIP_PRIV_MFG_DATE_SIZE                16
#endif

#endif /* __HCO_RIP_PRIV_H__ */

