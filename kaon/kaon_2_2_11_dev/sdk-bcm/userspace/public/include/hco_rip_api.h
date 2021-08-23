/**
 */


#ifndef __HCO_RIP_API_H__  
#define __HCO_RIP_API_H__  

/**
 * API major version
 */
#define LIBRIP_API_MAJOR 1

/**
 * API minor version
 */
#define LIBRIP_API_MINOR 9
 

#include "hco_rip_priv.h"  
 
/*  
 * Common RIP Fields mapped on vendor specific defines.  
 */  

/* Mandatory fields for all projects. */
#define RIP_COMMON_SERIAL_NUMBER        RIP_PRIV_SERIAL_NUMBER        /* String, Serial Number */  
#define RIP_COMMON_WAN_ADDR             RIP_PRIV_WAN_ADDR             /* RAW Data, WAN and LAN MAC Address */
#define RIP_COMMON_MANUFACTURER         RIP_PRIV_MANUFACTURER         /* String, Board Manufacturer */  
#define RIP_COMMON_MANUFACTURER_OUI     RIP_PRIV_MANUFACTURER_OUI     /* RAW Data, Board Manufacturer OUI */  
#define RIP_COMMON_MANUFACTURER_URL     RIP_PRIV_MANUFACTURER_URL     /* String, Board Manufacturer Web Site URL */  
#define RIP_COMMON_MODELNAME            RIP_PRIV_MODELNAME            /* String, Represents the board */  
#define RIP_COMMON_PRODUCT_CLASS        RIP_PRIV_PRODUCT_CLASS        /* String, Represents the product type  */  
#define RIP_COMMON_HARDWARE_VERSION     RIP_PRIV_HARDWARE_VERSION     /* String, Board version (prefered: X.Y.Z) */  
#define RIP_COMMON_BOOTLOADER_VERSION   RIP_PRIV_BOOTLOADER_VERSION   /* Binary array of 4 bytes, represents the bootloader version */

/* The following fields may not be required for all projects. */
#define RIP_COMMON_PUBLIC_DSA_NORM      RIP_PRIV_PUBLIC_DSA_NORM      /* DSA Public Key, for Regular Firmware Signature */  
#define RIP_COMMON_PUBLIC_DSA_RESC      RIP_PRIV_PUBLIC_DSA_RESC      /* DSA Public Key, for Rescue Firmware Signature */  
#define RIP_COMMON_CLIENT_CERTIFICATE   RIP_PRIV_CLIENT_CERTIFICATE   /* PEM (x509v3), Unique Device Certificate */  
#define RIP_COMMON_PRIVATE_KEY          RIP_PRIV_PRIVATE_KEY          /* PEM (PKCS#1), RSA Private Key (unique per device) */  
#define RIP_COMMON_H235_KEY             RIP_PRIV_H235_KEY             /* String, Secret Key (unique per device) for voice 
                                                                         activation */  
/* Since version 1.5. */
#ifdef RIP_PRIV_CLIENT_CERTIFICATE2
#define RIP_COMMON_CLIENT_CERTIFICATE2  RIP_PRIV_CLIENT_CERTIFICATE2  /* PEM (x509v3), Unique Device Secondary Certificate */  
#define RIP_COMMON_PRIVATE_KEY2         RIP_PRIV_PRIVATE_KEY2         /* PEM (PKCS#1), RSA Private Secondary Key (unique per device) */  
#endif

/* Mandatory wireless LAN fields. */
#define RIP_COMMON_WLAN_KEY             RIP_PRIV_WLAN_KEY             /* ASCII array of 26 hexadecimal digits, default 104bit WEP key */ 
#define RIP_COMMON_WLAN_BSSID           RIP_PRIV_WLAN_BSSID           /* Unused */

/* Optional field to specify factory test mode. Factory test mode is only available on some projects. */
/* Since version 1.1. */
#ifdef RIP_PRIV_FACTORY
#define RIP_COMMON_FACTORY              RIP_PRIV_FACTORY              /* String, if "1", /etc/init.d/factory script is executed at boot */
#endif

/* Optional DECT fields. */
/* Since version 1.2. */
#ifdef RIP_PRIV_DECT_RFPI
#define RIP_COMMON_DECT_RFPI            RIP_PRIV_DECT_RFPI            /* RAW Data, 5 bytes, DECT base station RFPI */
#endif
/* DSP Group. */
#ifdef RIP_PRIV_DECT_DSPG_RXTUN
#define RIP_COMMON_DECT_DSPG_RXTUN      RIP_PRIV_DECT_DSPG_RXTUN      /* RAW Data, 1 byte DECT base station calibration data */
#endif
#ifdef RIP_PRIV_DECT_DSPG_PORBGCFG
#define RIP_COMMON_DECT_DSPG_PORBGCFG   RIP_PRIV_DECT_DSPG_PORBGCFG   /* RAW Data, 1 byte DECT base station VDDC calibration data (DCX79) */
#endif

/* Optional wireless LAN fields. */
/* Since version 1.2. */
#ifdef RIP_PRIV_WLAN_CELENO_EEPROM
#define RIP_COMMON_WLAN_CELENO_EEPROM   RIP_PRIV_WLAN_CELENO_EEPROM   /* RAW Data, Celeno EEPROM data */
#endif
#ifdef RIP_PRIV_WLAN_CELENO_EEPROM2
#define RIP_COMMON_WLAN_CELENO_EEPROM2  RIP_PRIV_WLAN_CELENO_EEPROM2  /* RAW Data, Celeno EEPROM data for second interface */
#endif
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP
#define RIP_COMMON_WLAN_BRCM_SROM_MAP   RIP_PRIV_WLAN_BRCM_SROM_MAP   /* RAW Data, Broadcom WLAN SROM variable structure */
#endif
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP2
#define RIP_COMMON_WLAN_BRCM_SROM_MAP2  RIP_PRIV_WLAN_BRCM_SROM_MAP2  /* RAW Data, Broadcom SROM map for second interface */
#endif

/* Optional first use date field. This is only available on some projects. */
/* Since version 1.3. */
#ifdef RIP_PRIV_FIRSTUSE
#define RIP_COMMON_FIRSTUSE  RIP_PRIV_FIRSTUSE /* String, date of first use in format YYYYMMDD.
                                                  On read, if not set, must return success with an empty string value.
                                                  On write, if already set, must fail with return code -1. */
#endif

/* Optional xTU-R System Vendor ID. */
/* Since version 1.4. */
/* See: ITU-T G.997.1, Section 7.4.4, referring to G.994.1, Section 9.3.3.1
 * o The T.35 country code (2 octets) as the country of the system integrator (CPE vendor).
 * o The T.35 provider code (vendor identification, 4 octets) as the system integrator (CPE vendor).
 * o An additional 2 octets vendor-specific information.
 */
#ifdef RIP_PRIV_ITU_VENDOR_ID
#define RIP_COMMON_ITU_VENDOR_ID             RIP_PRIV_ITU_VENDOR_ID   /* RAW Data, 8 bytes, vendor ID */
#endif

/* Since version 1.6. */
/* Mandatory field for GPON projects. */
#ifdef RIP_PRIV_GPON_SERIAL_NUMBER
#define RIP_COMMON_GPON_SERIAL_NUMBER        RIP_PRIV_GPON_SERIAL_NUMBER   /* Special format 8 byte field, 
                                                                              first 4 bytes: ascii code identifying the vendor */
#endif

/* Since version 1.7. */
/* Optional KNX install code. */
#ifdef RIP_PRIV_KNX_INSTALL_CODE
#define RIP_COMMON_KNX_INSTALL_CODE          RIP_PRIV_KNX_INSTALL_CODE   /* 16 byte field encoded as 32 byte lower case hex ASCII */
#endif

/* Since version 1.8. */
/* Optional admin credentials */
/* - plain text string (up to 64 chars) */
/* - pre-hashed string (up to 138 chars) with the format '{SSHA256}SALT:HASH' */
#ifdef RIP_PRIV_ADMIN_PWD
#define RIP_COMMON_ADMIN_PWD                 RIP_PRIV_ADMIN_PWD   /* String, factory provisioned  */
#endif

/* Since version 1.9. */
/* WLAN SSID to ease Wi-Fi installation. */
/* - plain text string (up to 32 chars) */
#ifdef RIP_PRIV_WLAN_SSID
#define RIP_COMMON_WLAN_SSID                 RIP_PRIV_WLAN_SSID
#endif

#if defined(CONFIG_CUSTOM_MAXIS) || defined(CONFIG_CUSTOM_CELCOM)
/* Since version 1.8. */
/* Optional spt credentials */
/* - plain text string (up to 64 chars) */
/* - pre-hashed string (up to 138 chars) with the format '{SSHA256}SALT:HASH' */
#ifdef RIP_PRIV_SPT_PWD
#define RIP_COMMON_SPT_PWD                   RIP_PRIV_SPT_PWD   /* String, factory provisioned  */
#endif

/* Since version 1.8. */
/* Optional user credentials */
/* - plain text string (up to 64 chars) */
/* - pre-hashed string (up to 138 chars) with the format '{SSHA256}SALT:HASH' */
#ifdef RIP_PRIV_USER_PWD
#define RIP_COMMON_USER_PWD                  RIP_PRIV_USER_PWD   /* String, factory provisioned  */
#endif

#ifdef RIP_PRIV_RIP_VERSION
#define RIP_COMMON_RIP_VERSION                           RIP_PRIV_RIP_VERSION
#endif
#endif

#ifdef RIP_PRIV_CA_PEM_KEY
#define RIP_COMMON_CA_PEM_KEY                RIP_PRIV_CA_PEM_KEY  /* PLUME Certificate data. under 7KB */
#endif

#ifdef RIP_PRIV_CLIENT_PEM_KEY
#define RIP_COMMON_CLIENT_PEM_KEY            RIP_PRIV_CLIENT_PEM_KEY /* PLUME Certificate data. under 2KB */
#endif

#ifdef RIP_PRIV_DEC_KEY
#define RIP_COMMON_DEC_KEY                   RIP_PRIV_DEC_KEY /* PLUME Certificate data. under 512B */
#endif

#ifdef RIP_PRIV_SEED
#define RIP_COMMON_SEED                      RIP_PRIV_SEED      /* ecript / decript pwd for key PLUME data */
#endif

#ifdef RIP_PRIV_MFG_DATE
#define RIP_COMMON_MFG_DATE                  RIP_PRIV_MFG_DATE  /* mfg date*/
#endif

// maximum sizes for the various fields  
#define RIP_COMMON_SERIAL_NUMBER_SIZE        RIP_PRIV_SERIAL_NUMBER_SIZE  
#define RIP_COMMON_WAN_ADDR_SIZE             RIP_PRIV_WAN_ADDR_SIZE  
#define RIP_COMMON_MANUFACTURER_SIZE         RIP_PRIV_MANUFACTURER_SIZE  
#define RIP_COMMON_MANUFACTURER_OUI_SIZE     RIP_PRIV_MANUFACTURER_OUI_SIZE  
#define RIP_COMMON_MANUFACTURER_URL_SIZE     RIP_PRIV_MANUFACTURER_URL_SIZE  
#define RIP_COMMON_MODELNAME_SIZE            RIP_PRIV_MODELNAME_SIZE  
#define RIP_COMMON_PRODUCT_CLASS_SIZE        RIP_PRIV_PRODUCT_CLASS_SIZE  
#define RIP_COMMON_HARDWARE_VERSION_SIZE     RIP_PRIV_HARDWARE_VERSION_SIZE  
#define RIP_COMMON_BOOTLOADER_VERSION_SIZE   RIP_PRIV_BOOTLOADER_VERSION_SIZE
#define RIP_COMMON_PUBLIC_DSA_NORM_SIZE      RIP_PRIV_PUBLIC_DSA_NORM_SIZE  
#define RIP_COMMON_PUBLIC_DSA_RESC_SIZE      RIP_PRIV_PUBLIC_DSA_RESC_SIZE  
#define RIP_COMMON_CLIENT_CERTIFICATE_SIZE   RIP_PRIV_CLIENT_CERTIFICATE_SIZE  
#define RIP_COMMON_PRIVATE_KEY_SIZE          RIP_PRIV_PRIVATE_KEY_SIZE  
#define RIP_COMMON_H235_KEY_SIZE             RIP_PRIV_H235_KEY_SIZE
#define RIP_COMMON_CLIENT_CERTIFICATE2_SIZE  RIP_PRIV_CLIENT_CERTIFICATE2_SIZE  
#define RIP_COMMON_PRIVATE_KEY2_SIZE         RIP_PRIV_PRIVATE_KEY2_SIZE    
#define RIP_COMMON_WLAN_KEY_SIZE             RIP_PRIV_WLAN_KEY_SIZE  
#define RIP_COMMON_WLAN_BSSID_SIZE           RIP_PRIV_WLAN_BSSID_SIZE
#ifdef RIP_PRIV_FACTORY
#define RIP_COMMON_FACTORY_SIZE              RIP_PRIV_FACTORY_SIZE
#endif
#ifdef RIP_PRIV_DECT_RFPI
#define RIP_COMMON_DECT_RFPI_SIZE            RIP_PRIV_DECT_RFPI_SIZE
#endif
#ifdef RIP_PRIV_DECT_DSPG_RXTUN
#define RIP_COMMON_DECT_DSPG_RXTUN_SIZE      RIP_PRIV_DECT_DSPG_RXTUN_SIZE
#define RIP_COMMON_DECT_DSPG_PORBGCFG_SIZE   RIP_PRIV_DECT_DSPG_PORBGCFG_SIZE
#endif
#ifdef RIP_PRIV_WLAN_CELENO_EEPROM
#define RIP_COMMON_WLAN_CELENO_EEPROM_SIZE   RIP_PRIV_WLAN_CELENO_EEPROM_SIZE
#endif
#ifdef RIP_PRIV_WLAN_CELENO_EEPROM2
#define RIP_COMMON_WLAN_CELENO_EEPROM2_SIZE  RIP_PRIV_WLAN_CELENO_EEPROM2_SIZE
#endif
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP
#define RIP_COMMON_WLAN_BRCM_SROM_MAP_SIZE   RIP_PRIV_WLAN_BRCM_SROM_MAP_SIZE
#endif
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP2
#define RIP_COMMON_WLAN_BRCM_SROM_MAP2_SIZE  RIP_PRIV_WLAN_BRCM_SROM_MAP2_SIZE
#endif
#ifdef RIP_PRIV_FIRSTUSE
#define RIP_COMMON_FIRSTUSE_SIZE             RIP_PRIV_FIRSTUSE_SIZE
#endif
#ifdef RIP_PRIV_ITU_VENDOR_ID
#define RIP_COMMON_ITU_VENDOR_ID_SIZE        RIP_PRIV_ITU_VENDOR_ID_SIZE
#endif
#ifdef RIP_PRIV_GPON_SERIAL_NUMBER
#define RIP_COMMON_GPON_SERIAL_NUMBER_SIZE   RIP_PRIV_GPON_SERIAL_NUMBER_SIZE
#endif
#ifdef RIP_PRIV_KNX_INSTALL_CODE
#define RIP_COMMON_KNX_INSTALL_CODE_SIZE     RIP_PRIV_KNX_INSTALL_CODE_SIZE
#endif
#ifdef RIP_PRIV_ADMIN_PWD
#define RIP_COMMON_ADMIN_PWD_SIZE            RIP_PRIV_ADMIN_PWD_SIZE
#endif
#ifdef RIP_PRIV_WLAN_SSID
#define RIP_COMMON_WLAN_SSID_SIZE            RIP_PRIV_WLAN_SSID_SIZE
#endif

#if defined(CONFIG_CUSTOM_MAXIS) || defined(CONFIG_CUSTOM_CELCOM)
#ifdef RIP_PRIV_SPT_PWD
#define RIP_COMMON_SPT_PWD_SIZE              RIP_PRIV_SPT_PWD_SIZE
#endif
#ifdef RIP_PRIV_USER_PWD
#define RIP_COMMON_USER_PWD_SIZE             RIP_PRIV_USER_PWD_SIZE
#endif
#ifdef RIP_PRIV_RIP_VERSION
#define RIP_COMMON_RIP_VERSION_SIZE          RIP_PRIV_RIP_VERSION_SIZE
#endif
#endif

#ifdef RIP_PRIV_CA_PEM_KEY_SIZE
#define RIP_COMMON_CA_PEM_KEY_SIZE           RIP_PRIV_CA_PEM_KEY_SIZE
#endif

#ifdef RIP_PRIV_CLIENT_PEM_KEY_SIZE
#define RIP_COMMON_CLIENT_PEM_KEY_SIZE       RIP_PRIV_CLIENT_PEM_KEY_SIZE
#endif

#ifdef RIP_PRIV_DEC_KEY_SIZE
#define RIP_COMMON_DEC_KEY_SIZE              RIP_PRIV_DEC_KEY_SIZE
#endif

#ifdef RIP_PRIV_SEED_SIZE
#define RIP_COMMON_SEED_SIZE                 RIP_PRIV_SEED_SIZE
#endif

#ifdef RIP_PRIV_MFG_DATE_SIZE
#define RIP_COMMON_MFG_DATE_SIZE             RIP_PRIV_MFG_DATE_SIZE
#endif

/**
 * Return values for hco_rip_write(). Recommended for use by hci_rip_read().
 */
#define RIP_OK         0     /* Success. */
#define RIP_ERR_PERM   -1    /* Operation not permitted: this field cannot be written. */
#define RIP_ERR_NOENT  -2    /* Invalid field number. */
#define RIP_ERR_IO     -5    /* I/O error while writing. */
#define RIP_ERR_NOSPC  -28   /* Data too large. */


 
/***************************************  
* Function: hco_rip_init  
*
*    Gives the HardCo the opportunity to initialize its internal structures
*    if necessary.
*
*    return:  
*       < 0 : Error  
*       >=0 : N/A  
*/  
 
int hco_rip_init(void);  
 


/***************************************  
* Function: hco_rip_read(_ex)  
*    Params:  
*    field - RIP_COMMON_XXX  
*    data    - Reserved memory to store the requested data.  
*  
*    length - Works in 2 directions  
*              When called - it represents the size of the allocated data field.  
*              After return it contains the size of the data field.  
*              When length is too small for requested field an error occurs.  
*  
*    return:  
*       < 0 : Error  
*       >=0 : N/A  
*  
*/  
 
int hco_rip_read(unsigned short field, void * data, unsigned long *length);  


/***************************************  
* Function: hco_rip_write
*    Params:  
*    field  - RIP_COMMON_XXX  
*    data   - Memory to copy the data from
*  
*    length - Represents the size of the data field
*  
*    return: RIP_OK on success
*            RIP_ERR_PERM, RIP_ERR_NOENT, RIP_ERR_IO, RIP_ERR_NOSPC 
*        
*    Note that under normal conditions, writing to the RIP is not allowed.
*    This function should return RIP_ERR_PERM in those cases.
*    Exceptions must be agreed on on a per-project and per-parameter basis.
*/  
 
int hco_rip_write(unsigned short field, void * data, unsigned long length);
 

#endif /* __HCO_RIP_API_H__ */ 

