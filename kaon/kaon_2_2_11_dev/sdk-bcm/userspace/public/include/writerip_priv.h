#ifndef __RIP_PRIV_H__
#define __RIP_PRIV_H__

#include "hco_rip_api.h"
#include "hco_rip_priv.h"

//#define KAON_MODEL_NAME "AR1344P"

#define FLAG_STRING         1
#define FLAG_STRING_SIZE    2
#define FLAG_ARRAY          4
#define FLAG_MULTILINE      8
#define FLAG_MACADDRESS     16
#define FLAG_RIP_RW         32

#if defined(CONFIG_CUSTOM_PLUME)
#define RIP_RO_TLB_SIZE     8*1024    /* 2 pages */
#else
#define RIP_RO_TLB_SIZE     1*1024*1024    /* 1M RIP */
#endif

typedef struct {
  char            serial_number[RIP_COMMON_SERIAL_NUMBER_SIZE];
  unsigned char   mac_addr_base[RIP_COMMON_WAN_ADDR_SIZE];
  char            manufacturer[RIP_COMMON_MANUFACTURER_SIZE];
  char            manufacturer_oui[RIP_COMMON_MANUFACTURER_OUI_SIZE];
  char            manufacturer_url[RIP_COMMON_MANUFACTURER_URL_SIZE];
  char            model_name[RIP_COMMON_MODELNAME_SIZE];
  char            product_class[RIP_COMMON_PRODUCT_CLASS_SIZE];
  char            hardware_version[RIP_COMMON_HARDWARE_VERSION_SIZE];
  char            bootloader_version[RIP_COMMON_BOOTLOADER_VERSION_SIZE];
  unsigned char   public_rsa_normal[RIP_COMMON_PUBLIC_DSA_NORM_SIZE];
  unsigned char   public_rsa_rescue[RIP_COMMON_PUBLIC_DSA_RESC_SIZE];
  char            client_certificate[RIP_COMMON_CLIENT_CERTIFICATE_SIZE];
  char            private_key[RIP_COMMON_PRIVATE_KEY_SIZE];
  char            wlan_key[RIP_COMMON_WLAN_KEY_SIZE];
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP
  char            wlan_brcm_srom_map1[RIP_COMMON_WLAN_BRCM_SROM_MAP_SIZE];
#endif
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP2
  char            wlan_brcm_srom_map2[RIP_COMMON_WLAN_BRCM_SROM_MAP2_SIZE];
#endif
/* Optional field to specify factory test mode. Factory test mode is only available on some projects. */
/* Since version 1.1. */
#ifdef RIP_PRIV_FACTORY
  char            factory[RIP_COMMON_FACTORY_SIZE];
#endif
#ifdef RIP_PRIV_ADMIN_PWD
  char            admin_pwd[RIP_PRIV_ADMIN_PWD_SIZE];
#endif
#ifdef RIP_PRIV_WLAN_SSID
  char            wlan_ssid[RIP_PRIV_WLAN_SSID_SIZE];
#endif
#if defined(CONFIG_CUSTOM_MAXIS) || defined(CONFIG_CUSTOM_CELCOM)
#ifdef RIP_PRIV_SPT_PWD
  char            spt_pwd[RIP_PRIV_SPT_PWD_SIZE];
#endif
#ifdef RIP_PRIV_USER_PWD
  char            user_pwd[RIP_PRIV_USER_PWD_SIZE];
#endif
#ifdef RIP_PRIV_RIP_VERSION
  char            rip_ver[RIP_PRIV_RIP_VERSION_SIZE];
#endif
#endif

#if defined(CONFIG_CUSTOM_PLUME)
#ifdef RIP_PRIV_CA_PEM_KEY
  unsigned char   ca_pem[RIP_PRIV_CA_PEM_KEY_SIZE];
  int ca_pem_size;
#endif // RIP_PRIV_CA_PEM_KEY
#ifdef RIP_PRIV_CLIENT_PEM_KEY
  unsigned char   client_pem[RIP_PRIV_CLIENT_PEM_KEY_SIZE];
  int client_pem_size;
#endif // RIP_PRIV_CLIENT_PEM_KEY
#ifdef RIP_PRIV_DEC_KEY
  unsigned char   dec_key[RIP_PRIV_DEC_KEY_SIZE];
  int dec_key_size;
#endif // RIP_PRIV_DEC_KEY
#ifdef RIP_PRIV_SEED
  char   seed_key[RIP_PRIV_SEED_SIZE];
#endif // RIP_PRIV_SEED
#ifdef RIP_PRIV_MFG_DATE
  char   mfg_date[RIP_PRIV_MFG_DATE_SIZE];
#endif // RIP_PRIV_MFG_DATE
#endif // CONFIG_CUSTOM_PLUME

} RIP_RO_tlb;

typedef struct {
  union {
    RIP_RO_tlb    rip_ro_tlb;
    unsigned char padding[RIP_RO_TLB_SIZE];  /* 8k (2 pages) */
  };
  unsigned long checksum;
} MW_RIP_RO, *PMW_RIP_RO;

struct rip_entry {
  unsigned short id;
  const char *name;
  unsigned long length;
  const void *data;
  unsigned long flags;
  char *filename;
};

#define VERSION_MAJOR  0
#define VERSION_MINOR  4

#define RIP_MTD    "/dev/mtd8"

#define DEFAULT_SN      "BS10065102000000"
#define DEFAULT_MAC     "\x02\x10\x18\x01\x00\x01"

int write_rip_field(char *field, char *value);
int rip_to_certi_files(void);
int dump_rip_to_files(char* path);
int write_rip_use_files(char *path);
void rip_list_fields(void);
int set_default_rip(void);
int print_mtd_rip(char* field);
int rip_init(void);
int dump_rip(MW_RIP_RO *rip_ro_p);
int rip_to_nvram(void);
int decrypt_certi_file(void);
int change_ssh_pw(void);
int change_ssh_pw2(void);
int encrypt_certi_file(char *path);

uint64_t mac2int(const uint8_t hwaddr[]);
void int2mac(const uint64_t mac, uint8_t *hwaddr);

typedef enum
{
    RIP_SET_DEFAULT,
    RIP_SET_FILES,
    RIP_SET_FIELD,  /* TODO: Set one field in the RIP */
    RIP_GET_ALL,
    RIP_GET_FIELD,  /* TODO: Get one field from the RIP */
    RIP_LIST_FIELDS,
    RIP_DUMP_TOFILE,
    /* Add your action here */
    RIP_TO_NVRAM,
    DECRYPT_CERTI,
    RIP_DEC_FW,
    CHANGE_PW,
    ENC_CERTI,
    RIP_NOACTION
} RIP_ACTIONS_E;

/*
 * RIP:
 * "serial_number.txt" : "BS10065102000000"
 * "mac_address.txt"   : "00:00:00:00:00:00"
 * "manufacturer.txt"  : "Kaonmedia Co., Ltd."
 * "manufacturer_oui.bin" : Binary 3bytes
 * "manufacturer_url.txt" : "http://www.kaonmedia.com"
 * "model_name.txt" : "ARXXXX"
 * "product_class.txt" : "ARROUTER" or "REPEATER"
 * "hardware_version.txt": "1.x.x"
 * "bootloader_version.txt": "1.0.38-163.231"
 * "wlan_key.txt": wlan password
 * "admin_pwd.txt": "admin"
 * "ssid" : KaonAP
 *
 */


#endif /* __RIP_PRIV_H__ */
