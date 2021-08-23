/*
 * ============================================================================
 *
 * ============================================================================
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <mtd/mtd-user.h>
#include <writerip_priv.h>
#include <sys/types.h>
#include <time.h>

#include "file_security.h"
#include "cms.h"
#include "cms_util.h"
#include "cms_boardioctl.h"
#include "cms_boardcmds.h"

//#define DEBUG_WRITE_RIP

#ifdef DEBUG_WRITE_RIP
#define DEBUG(format,...)    printf(format, ##__VA_ARGS__)
#else
#define DEBUG(format,...)
#endif

extern const unsigned char wl_srom_map_1108[RIP_COMMON_WLAN_BRCM_SROM_MAP_SIZE];
extern const unsigned char wl_srom_map_4366[RIP_PRIV_WLAN_BRCM_SROM_MAP2_SIZE];

/* Generated using: openssl dsaparam -C -genkey 1024  -noout */
static const unsigned char norm_dsa1024_p[] = {
  0x8D,0x2E,0x5C,0xE4,0x63,0xD8,0x4F,0x55,0xEF,0xD5,0x20,0x30,
  0xE9,0xD2,0x10,0x21,0x26,0x48,0xBF,0x8B,0x38,0xCA,0x5F,0x01,
  0x46,0x74,0xAE,0xFD,0x56,0x94,0xFA,0x04,0xDC,0xFF,0xD8,0x67,
  0xA2,0xAB,0x6C,0xD8,0x49,0xB9,0x2F,0xE4,0xDF,0x03,0xA2,0x75,
  0xA2,0x44,0x69,0x0A,0x2B,0x0D,0x70,0xAB,0xD9,0xB9,0xDF,0x49,
  0x8A,0x1A,0x08,0xC7,0x32,0x6E,0x8F,0xDE,0x91,0x16,0xCD,0xB3,
  0x23,0x1D,0x21,0x64,0x53,0xE8,0xDA,0xD0,0x7C,0xD3,0xE2,0xC6,
  0xD9,0x12,0x4D,0x09,0x13,0x9E,0xE8,0x36,0x09,0xA2,0xC3,0x98,
  0x42,0x94,0x3C,0xC6,0x81,0x3C,0x98,0xE7,0xB9,0x69,0x0E,0x64,
  0x20,0x5B,0x17,0xCC,0xDA,0xAA,0xCD,0x10,0xBF,0x88,0x13,0x6C,
  0x89,0xCB,0x55,0x9C,0x15,0x04,0x31,0xC7,
  };
static const unsigned char resc_dsa1024_p[] = {
  0xB6,0x4B,0xC3,0xD1,0x49,0xA1,0x96,0x70,0xBC,0xA9,0x95,0x20,
  0x6C,0x32,0x52,0x6C,0x60,0x03,0xC0,0x1F,0x28,0x11,0x41,0x54,
  0xAB,0x2C,0x67,0xAF,0xCD,0x27,0x9F,0x3A,0x65,0xC7,0xD1,0x5E,
  0xB3,0x9C,0xEB,0xFC,0xCE,0xE7,0xBF,0xEF,0x59,0x12,0xBA,0x46,
  0xCE,0x96,0x24,0x14,0xC3,0xE3,0x1F,0x61,0x5A,0x2C,0xA0,0x60,
  0x32,0xFF,0xFD,0xB3,0x17,0x6D,0xCD,0xA2,0xA3,0xDA,0x44,0x3B,
  0x25,0x1A,0xEA,0x90,0xA3,0x43,0x02,0x38,0xC9,0xE7,0x44,0x9E,
  0x8C,0x30,0x6F,0xE5,0x3B,0x4B,0x38,0x3F,0x84,0x2A,0xEB,0x96,
  0x1F,0x9B,0x61,0x43,0xE1,0x74,0x01,0xBB,0xD7,0xCB,0x64,0x08,
  0xEE,0x8C,0x9F,0xD3,0x46,0xAE,0x82,0x66,0x77,0x36,0x70,0x8C,
  0xC5,0x7A,0x7F,0x54,0xCD,0x1C,0xD7,0x53,
};

/* openssl req -x509 -sha256 -nodes -days 7300 -newkey rsa:1024 -keyout privateKey.key -out certificate.crt */
static const char cert[] = "\
-----BEGIN CERTIFICATE-----\n\
MIICgjCCAeugAwIBAgIJANLfux43DNaEMA0GCSqGSIb3DQEBCwUAMFoxCzAJBgNV\n\
BAYTAkJFMRMwEQYDVQQIDApTb21lLVN0YXRlMREwDwYDVQQHDAhXaWpnbWFhbDET\n\
MBEGA1UECgwKU29mdEF0SG9tZTEOMAwGA1UEAwwFYWRtaW4wHhcNMTYxMjIxMTUx\n\
MDAwWhcNMzYxMjE2MTUxMDAwWjBaMQswCQYDVQQGEwJCRTETMBEGA1UECAwKU29t\n\
ZS1TdGF0ZTERMA8GA1UEBwwIV2lqZ21hYWwxEzARBgNVBAoMClNvZnRBdEhvbWUx\n\
DjAMBgNVBAMMBWFkbWluMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDRrBwR\n\
YnYSOMU0pnvzU9zzIbhBJGn3a6dOQ9Y7nc2odhEK6MccPXOzqyGFRjLRHhdww1Fw\n\
/1IC5gC3GQBM6LWww8bSrusgyX7RZL1pa0t5Jh8f9cRqrSwNzd+Qwp85SBSzc7EM\n\
dKQ4osTwR4E5iEX716whPrcffFpwEKdjTHt6qwIDAQABo1AwTjAdBgNVHQ4EFgQU\n\
FW6vX7AHQF9r5vXzZWQIYVBy2gIwHwYDVR0jBBgwFoAUFW6vX7AHQF9r5vXzZWQI\n\
YVBy2gIwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOBgQCZGw5RZcDyTZzu\n\
QjQoGknoXBOUlGfAXcJXwTI1tRTOwGnu38+BC7pIPTKgZxA3cxWaDMwAFZdqz7dZ\n\
mP4/jVJ1W3qOdq/U1WwBef0U5mzKF/9gRZ47Q8jvdhIiooz6LeqVqVN0IV6c7+ZX\n\
el8zPw/p8+AolrvgeNFY2zlIXuvSXg==\n\
-----END CERTIFICATE-----\
";

static const char privkey[] = "\
-----BEGIN PRIVATE KEY-----\n\
MIICdwIBADANBgkqhkiG9w0BAQEFAASCAmEwggJdAgEAAoGBANGsHBFidhI4xTSm\n\
e/NT3PMhuEEkafdrp05D1judzah2EQroxxw9c7OrIYVGMtEeF3DDUXD/UgLmALcZ\n\
AEzotbDDxtKu6yDJftFkvWlrS3kmHx/1xGqtLA3N35DCnzlIFLNzsQx0pDiixPBH\n\
gTmIRfvXrCE+tx98WnAQp2NMe3qrAgMBAAECgYEAsxFda09+7TME4WFqLLcr1Y2v\n\
0hqqUX/khEgVWBb+hGhJR9rj6e9luEVoPG1miwg7FrgI/wtfXFBYvgchJY39VK0g\n\
L0/+BDSDaLhtDr8gYWctllp1oLUt4+uZzFNiun1cwLwhuOdmJY4nz2LPV4nz5DcE\n\
TD/QqwM7mOGH1BT6gLkCQQDsOlTsbPfpsapEta3Jqa6G3nweCEEiNlqriAGe3WnC\n\
DF/P8FJ0GbLnvOTpDtoPsHPd9LEAcJrBi1wDnIdm9VNHAkEA4zjFPPybIBCjXSoi\n\
4SJO5ffQRuWLlBYZnfITePZLp+/dmFWlCQejiWGVSfibku6JVr/0Ifyl2oaAM86C\n\
be4nfQJBANpdLYWDUUPoJJHxM124CYnIfgkw7iyW/AeV6JGW8J0c3TCVYVLLMrK+\n\
zQeW4AIixWiGOVxDuvpwPTVla1DENB8CQFohTrueR7o0X1i5OhSrkzhNUGSO8QrQ\n\
qWCpgWfd6qy2zON8NXabfRclih5JawyhagDrK1+/49oGuBvUspAAg9ECQHU5scmj\n\
s7drmB4xazbRtRU0JbofhqaSJc9qr5t5G77JdhUgO4oUi2JK1ijZnJVU11BHqB/W\n\
LIxqdzilwnRkXVc=\n\
-----END PRIVATE KEY-----\
";


static struct rip_entry rip[] = {
  {
    .id = RIP_COMMON_SERIAL_NUMBER,
    .name = "SERIAL_NUMBER",
    .length = RIP_PRIV_SERIAL_NUMBER_SIZE,
    .data = DEFAULT_SN,
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "serial_number.txt",
  },
  {
    .id = RIP_COMMON_WAN_ADDR,
    .name = "WAN_ADDR",
    .length = RIP_COMMON_WAN_ADDR_SIZE,
    .data = DEFAULT_MAC,
    .flags = FLAG_MACADDRESS,
    .filename = "mac_address.txt",
  },
  {
    .id = RIP_COMMON_MANUFACTURER,
    .name = "MANUFACTURER",
    .length = RIP_PRIV_MANUFACTURER_SIZE,
    .data = "Kaonmedia Co., Ltd.",
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "manufacturer.txt",
  },
  {
    .id = RIP_COMMON_MANUFACTURER_OUI,
    .name = "MANUFACTURER_OUI",
    .length = RIP_PRIV_MANUFACTURER_OUI_SIZE,
    .data = "\x00\x00\x00",
    .flags = 0,
    .filename = "manufacturer_oui.bin",
  },
  {
    .id = RIP_COMMON_MANUFACTURER_URL,
    .name = "MANUFACTURER_URL",
    .length = RIP_PRIV_MANUFACTURER_URL_SIZE,
    .data = "http://www.kaonmedia.com",
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "manufacturer_url.txt",
  },
  {
    .id = RIP_COMMON_MODELNAME,
    .name = "MODELNAME",
    .length = RIP_PRIV_MODELNAME_SIZE,
#if defined(CONFIG_MODEL_AR2146)
    .data = "AR2146",
#elif defined(CONFIG_MODEL_AR2140)
    .data = "AR2140",
#elif defined(CONFIG_MODEL_AR1344)
#ifdef NON_BROWSER
    .data = "AR1344E",
#else // NON_BROWSER
    .data = "AR1344",
#endif // NON_BROWSER
#elif defined(CONFIG_MODEL_AR1340)
    .data = "AR1340",
#else
    .data = "",
#endif
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "model_name.txt",
  },
  {
    .id = RIP_COMMON_PRODUCT_CLASS,
    .name = "PRODUCT_CLASS",
    .length = RIP_PRIV_PRODUCT_CLASS_SIZE,
#if defined(CONFIG_MODEL_AR2146) || defined(CONFIG_MODEL_AR2140)
    .data = "APROUTER",
#elif defined(CONFIG_MODEL_AR1344) || defined(CONFIG_MODEL_AR1340)
    .data = "EXTENDER",
#else
    .data = "",
#endif
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "product_class.txt",
  },
  {
    .id = RIP_COMMON_HARDWARE_VERSION,
    .name = "HARDWARE_VERSION",
    .length = RIP_PRIV_HARDWARE_VERSION_SIZE,
    .data = "1.0",
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "hardware_version.txt",
  },
  {
    .id = RIP_COMMON_BOOTLOADER_VERSION,
    .name = "BOOTLOADER_VERSION",
    .length = RIP_COMMON_BOOTLOADER_VERSION_SIZE,
    .data = "1.0.38-163.231",
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "bootloader_version.txt",
  },
  {
    .id = RIP_COMMON_PUBLIC_DSA_NORM,
    .name = "PUBLIC_DSA_NORM",
    .length = RIP_PRIV_PUBLIC_DSA_NORM_SIZE,
    .data = norm_dsa1024_p,
    .flags = FLAG_ARRAY,
    .filename = "public_dsa_norm",
  },
  {
    .id = RIP_COMMON_PUBLIC_DSA_RESC,
    .name = "PUBLIC_DSA_RESC",
    .length = RIP_PRIV_PUBLIC_DSA_RESC_SIZE,
    .data = resc_dsa1024_p,
    .flags = FLAG_ARRAY,
    .filename = "public_dsa_resc",
  },
  {
    .id = RIP_COMMON_CLIENT_CERTIFICATE,
    .name = "CLIENT_CERTIFICATE",
    .length = RIP_PRIV_CLIENT_CERTIFICATE_SIZE,
    .data = cert,
    .flags = FLAG_STRING | FLAG_STRING_SIZE | FLAG_MULTILINE,
    .filename = "client_certificate",
  },
  {
    .id = RIP_COMMON_PRIVATE_KEY,
    .name = "PRIVATE_KEY",
    .length = RIP_PRIV_PRIVATE_KEY_SIZE,
    .data = privkey,
    .flags = FLAG_STRING | FLAG_STRING_SIZE | FLAG_MULTILINE,
    .filename = "private_key",
  },
  {
    .id = RIP_COMMON_WLAN_KEY,
    .name = "WLAN_KEY",
    .length = RIP_PRIV_WLAN_KEY_SIZE,
    .data = "41BAFE401160A731A1A91511B5",
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "wlan_key.txt",
  },
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP
  {
    .id = RIP_COMMON_WLAN_BRCM_SROM_MAP,
    .name = "WLAN_BRCM_SROM_MAP",
    .length = RIP_PRIV_WLAN_BRCM_SROM_MAP_SIZE,
    .data = wl_srom_map_1108,
    .flags = 0,
    .filename = "wlan_srom_1.bin",
  },
#endif
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP2
  {
    .id = RIP_COMMON_WLAN_BRCM_SROM_MAP2,
    .name = "WLAN_BRCM_SROM_MAP2",
    .length = RIP_PRIV_WLAN_BRCM_SROM_MAP2_SIZE,
    .data = wl_srom_map_4366,
    .flags = 0,
    .filename = "wlan_srom_2.bin",
  },
#endif
#ifdef RIP_PRIV_FACTORY
  {
    .id = RIP_COMMON_FACTORY,
    .name = "FACTORY",
    .length = RIP_PRIV_FACTORY_SIZE,
    .data = "0",
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = NULL,
  },
#endif
#ifdef RIP_PRIV_ADMIN_PWD
  {
    .id = RIP_COMMON_ADMIN_PWD,
    .name = "ADMIN_PWD",
    .length = RIP_PRIV_ADMIN_PWD_SIZE,
#if defined(CONFIG_CUSTOM_MAXIS) || defined(CONFIG_CUSTOM_CELCOM)
    .data = "password",
#else
    .data = "admin",
#endif
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "admin_pwd.txt",
  },
#endif
#ifdef RIP_PRIV_WLAN_SSID
  {
    .id = RIP_COMMON_WLAN_SSID,
    .name = "WLAN_SSID",
    .length = RIP_PRIV_WLAN_SSID_SIZE,
    .data = "",
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
	.filename = "ssid",
  },
#endif
#if defined(CONFIG_CUSTOM_MAXIS) || defined(CONFIG_CUSTOM_CELCOM)
#ifdef RIP_PRIV_SPT_PWD
  {
    .id = RIP_COMMON_SPT_PWD,
    .name = "SPT_PWD",
    .length = RIP_PRIV_SPT_PWD_SIZE,
    .data = "password",
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "spt_pwd.txt",
  },
#endif
#ifdef RIP_PRIV_USER_PWD
  {
    .id = RIP_COMMON_USER_PWD,
    .name = "USER_PWD",
    .length = RIP_PRIV_USER_PWD_SIZE,
    .data = "password",
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "user_pwd.txt",
  },
#endif
#ifdef RIP_PRIV_RIP_VERSION
  {
    .id = RIP_COMMON_RIP_VERSION,
    .name = "RIP_VERSION",
    .length = RIP_PRIV_RIP_VERSION_SIZE,
    .data = "1.0",
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "rip_version.txt",
  },
#endif
#endif
  {
    .id = RIP_COMMON_FIRSTUSE,
    .name = "RIP_COMMON_FIRSTUSE",
    .length = RIP_COMMON_FIRSTUSE_SIZE,
    .flags = FLAG_STRING | FLAG_STRING_SIZE | FLAG_RIP_RW,
    .filename = NULL,
  },
#if defined(CONFIG_CUSTOM_PLUME)
#ifdef RIP_PRIV_CA_PEM_KEY
  {
    .id = RIP_COMMON_CA_PEM_KEY,
    .name = "CA_PEM_KEY",
    .length = RIP_PRIV_CA_PEM_KEY_SIZE,
    .data = "",
    .flags = 0,
    .filename = "cer_ca.pem",
  },
#endif // RIP_PRIV_CA_PEM_KEY
#ifdef RIP_PRIV_CLIENT_PEM_KEY
  {
    .id = RIP_COMMON_CLIENT_PEM_KEY,
    .name = "CLIENT_PEM_KEY",
    .length = RIP_PRIV_CLIENT_PEM_KEY_SIZE,
    .data = "",
    .flags = 0,
    .filename = "cer_client.pem",
  },
#endif // RIP_PRIV_CLIENT_PEM_KEY
#ifdef RIP_PRIV_DEC_KEY
  {
    .id = RIP_COMMON_DEC_KEY,
    .name = "DEC_KEY",
    .length = RIP_PRIV_DEC_KEY_SIZE,
    .data = "",
    .flags = 0,
    .filename = "cer_client_dec.key",
  },
#endif // RIP_PRIV_DEC_KEY
#ifdef RIP_PRIV_SEED
  {
    .id = RIP_COMMON_SEED,
    .name = "SEED_VALUE",
    .length = RIP_PRIV_SEED_SIZE,
    .data = "D123asd14s",
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "key_seed_value.txt",
  },
#endif // RIP_PRIV_SEED
#ifdef RIP_PRIV_MFG_DATE
  {
    .id = RIP_COMMON_MFG_DATE,
    .name = "MFG_DATE",
    .length = RIP_PRIV_MFG_DATE_SIZE,
    .data = "20210730",
    .flags = FLAG_STRING | FLAG_STRING_SIZE,
    .filename = "mfg_date.txt",
  },
#endif // RIP_PRIV_MFG_DATE
#endif // CONFIG_CUSTOM_PLUME
};

#define CRC32_INIT_VALUE    0xffffffff

static unsigned int crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};


static char *
ether_etoa(const unsigned char *e, char *a)
{
        char *c = a;
        int i;

        for (i = 0; i < 6; i++) {
                if (i)
                        *c++ = ':';
                c += sprintf(c, "%02X", e[i] & 0xff);
        }
        return a;
}

/***************************************************************************
// Function Name: lib_get_crc32
// Description  : calculate the CRC 32 of the given data.
// Parameters   : pdata - array of data.
//                size - number of input data bytes.
//                crc - either CRC32_INIT_VALUE or previous return value.
// Returns      : crc.
****************************************************************************/
static uint32_t lib_get_crc32(unsigned char *pdata, uint32_t size, uint32_t crc)
{
    while (size-- > 0)
        crc = (crc >> 8) ^ crc32_table[(crc ^ *pdata++) & 0xff];

    return crc;
}

static int fill_rip_struct(PMW_RIP_RO rip_ro_p, const unsigned char *buf, unsigned short index)
{
    unsigned def=0;

    if(index >= sizeof(rip)/sizeof(rip[0]))
    {
        fprintf(stderr, "Index is too large %u (max: %u)\n", index, sizeof(rip)/sizeof(rip[0]));
        return -1;
    }

    if (NULL == buf)
    {
        DEBUG("Set the default settings for %s\n", rip[index].name);
        def=1;
        buf = rip[index].data;
    }

#ifdef DEBUG_WRITE_RIP
    if(rip[index].flags & FLAG_STRING ||
       (rip[index].flags & FLAG_MACADDRESS && !def))
        DEBUG("Set %s: %s (%d bytes)\n", rip[index].name,
              buf, buf ? strlen(buf) : 0);
    else if (rip[index].flags & FLAG_MACADDRESS)
        DEBUG("Set %s: %02X:%02X:%02X:%02X:%02X:%02X\n", rip[index].name,
              buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    else /* Don't print binary data */
        DEBUG("Set %s\n", rip[index].name);
#endif

    switch(rip[index].id)
    {
      case RIP_COMMON_SERIAL_NUMBER:
          memcpy(rip_ro_p->rip_ro_tlb.serial_number, buf, RIP_COMMON_SERIAL_NUMBER_SIZE);
          break;

      case RIP_COMMON_WAN_ADDR:
      {
          int i;
          if(def) /* Use default value => hex */
          {
              memcpy(rip_ro_p->rip_ro_tlb.mac_addr_base, buf, RIP_COMMON_WAN_ADDR_SIZE);
          }
          else /* String from file */
          {
              unsigned int mac_addr[RIP_COMMON_WAN_ADDR_SIZE]={0};
              int res = sscanf((const char *)buf, "%02x:%02x:%02x:%02x:%02x:%02x", &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]);
              if (RIP_COMMON_WAN_ADDR_SIZE != res)
              {
                  fprintf(stderr, "WAN MAC wrongly formatted %s (%s)\n", buf, strerror(errno));
              }
              for(i=0 ; i<RIP_COMMON_WAN_ADDR_SIZE ; ++i)
                  rip_ro_p->rip_ro_tlb.mac_addr_base[i] = (unsigned char)(mac_addr[i] & 0xff);
          }
          DEBUG("WAN MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                rip_ro_p->rip_ro_tlb.mac_addr_base[0],
                rip_ro_p->rip_ro_tlb.mac_addr_base[1],
                rip_ro_p->rip_ro_tlb.mac_addr_base[2],
                rip_ro_p->rip_ro_tlb.mac_addr_base[3],
                rip_ro_p->rip_ro_tlb.mac_addr_base[4],
                rip_ro_p->rip_ro_tlb.mac_addr_base[5]);
          break;
      }
      case RIP_COMMON_MANUFACTURER:
          memcpy(rip_ro_p->rip_ro_tlb.manufacturer, buf, RIP_COMMON_MANUFACTURER_SIZE);
          break;

      case RIP_COMMON_MANUFACTURER_OUI:
          memcpy(rip_ro_p->rip_ro_tlb.manufacturer_oui, buf, RIP_COMMON_MANUFACTURER_OUI_SIZE);
          break;

      case RIP_COMMON_MANUFACTURER_URL:
          memcpy(rip_ro_p->rip_ro_tlb.manufacturer_url, buf, RIP_COMMON_MANUFACTURER_URL_SIZE);
          break;

      case RIP_COMMON_MODELNAME:
          memcpy(rip_ro_p->rip_ro_tlb.model_name, buf, RIP_COMMON_MODELNAME_SIZE);
          break;

      case RIP_COMMON_PRODUCT_CLASS:
          memcpy(rip_ro_p->rip_ro_tlb.product_class, buf, RIP_COMMON_PRODUCT_CLASS_SIZE);
          break;

      case RIP_COMMON_HARDWARE_VERSION:
          memcpy(rip_ro_p->rip_ro_tlb.hardware_version, buf, RIP_COMMON_HARDWARE_VERSION_SIZE);
          break;

      case RIP_COMMON_BOOTLOADER_VERSION:
          memcpy(rip_ro_p->rip_ro_tlb.bootloader_version, buf, RIP_COMMON_BOOTLOADER_VERSION_SIZE);
          break;

      case RIP_COMMON_PUBLIC_DSA_NORM:
          memcpy(rip_ro_p->rip_ro_tlb.public_rsa_normal, buf, RIP_COMMON_PUBLIC_DSA_NORM_SIZE);
          break;

      case RIP_COMMON_PUBLIC_DSA_RESC:
          memcpy(rip_ro_p->rip_ro_tlb.public_rsa_rescue, buf, RIP_COMMON_PUBLIC_DSA_RESC_SIZE);
          break;

      case RIP_COMMON_CLIENT_CERTIFICATE:
          memcpy(rip_ro_p->rip_ro_tlb.client_certificate, buf, RIP_COMMON_CLIENT_CERTIFICATE_SIZE);
          break;

      case RIP_COMMON_PRIVATE_KEY:
          memcpy(rip_ro_p->rip_ro_tlb.private_key, buf, RIP_COMMON_PRIVATE_KEY_SIZE);
          break;

      case RIP_COMMON_WLAN_KEY:
          memcpy(rip_ro_p->rip_ro_tlb.wlan_key, buf, RIP_COMMON_WLAN_KEY_SIZE);
          break;

#if 0 /* NOT USED */
      case RIP_COMMON_WLAN_BSSID:
      {
          int i;
          if(def) /* Use default value => hex */
          {
            memcpy(rip_ro_p->rip_ro_tlb.wlan_bssid, buf, RIP_COMMON_WLAN_BSSID_SIZE);
          }
          else /* String from file */
          {
              unsigned int wlan_bssid[RIP_COMMON_WLAN_BSSID_SIZE]={0};
              int res = sscanf(buf, "%2x:%2x:%2x:%2x:%2x:%2x",
                               &wlan_bssid[0], &wlan_bssid[1], &wlan_bssid[2],
                               &wlan_bssid[3], &wlan_bssid[4], &wlan_bssid[5]);
              if (RIP_COMMON_WLAN_BSSID_SIZE != res)
              {
                  fprintf(stderr, "WLAN BSSID wrongly formatted %s (%s)\n",
                          buf, strerror(errno));
              }
              for(i=0 ; i<RIP_COMMON_WLAN_BSSID_SIZE ; ++i)
                rip_ro_p->rip_ro_tlb.wlan_bssid[i] = (unsigned char)(wlan_bssid[i] & 0xff);
          }
          DEBUG("WLAN BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
                rip_ro_p->rip_ro_tlb.wlan_bssid[0],
                rip_ro_p->rip_ro_tlb.wlan_bssid[1],
                rip_ro_p->rip_ro_tlb.wlan_bssid[2],
                rip_ro_p->rip_ro_tlb.wlan_bssid[3],
                rip_ro_p->rip_ro_tlb.wlan_bssid[4],
                rip_ro_p->rip_ro_tlb.wlan_bssid[5]);
          break;
      }
#endif

#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP
      case RIP_PRIV_WLAN_BRCM_SROM_MAP:
          if(buf)
              memcpy(rip_ro_p->rip_ro_tlb.wlan_brcm_srom_map1, buf, RIP_COMMON_WLAN_BRCM_SROM_MAP_SIZE);
          else
              memset(rip_ro_p->rip_ro_tlb.wlan_brcm_srom_map1, 0, RIP_COMMON_WLAN_BRCM_SROM_MAP_SIZE);
          break;
#endif

#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP2
      case RIP_PRIV_WLAN_BRCM_SROM_MAP2:
          if(buf)
              memcpy(rip_ro_p->rip_ro_tlb.wlan_brcm_srom_map2, buf, RIP_COMMON_WLAN_BRCM_SROM_MAP2_SIZE);
          else
              memset(rip_ro_p->rip_ro_tlb.wlan_brcm_srom_map2, 0, RIP_COMMON_WLAN_BRCM_SROM_MAP2_SIZE);
          break;
#endif

#ifdef RIP_PRIV_FACTORY
      case RIP_PRIV_FACTORY:
          memcpy(rip_ro_p->rip_ro_tlb.factory, buf, RIP_COMMON_FACTORY_SIZE);
          break;
#endif

#ifdef RIP_PRIV_ADMIN_PWD
      case RIP_PRIV_ADMIN_PWD:
          memcpy(rip_ro_p->rip_ro_tlb.admin_pwd, buf, RIP_PRIV_ADMIN_PWD_SIZE);
          break;
#endif

      case RIP_PRIV_WLAN_SSID:
            memcpy(rip_ro_p->rip_ro_tlb.wlan_ssid, buf, RIP_PRIV_WLAN_SSID_SIZE);
            break;

#if defined(CONFIG_CUSTOM_MAXIS) || defined(CONFIG_CUSTOM_CELCOM)
#ifdef RIP_PRIV_SPT_PWD
      case RIP_PRIV_SPT_PWD:
          memcpy(rip_ro_p->rip_ro_tlb.spt_pwd, buf, RIP_PRIV_SPT_PWD_SIZE);
          break;
#endif
#ifdef RIP_PRIV_USER_PWD
      case RIP_PRIV_USER_PWD:
          memcpy(rip_ro_p->rip_ro_tlb.user_pwd, buf, RIP_PRIV_USER_PWD_SIZE);
          break;
#endif
#ifdef RIP_PRIV_RIP_VERSION
      case RIP_PRIV_RIP_VERSION:
          memcpy(rip_ro_p->rip_ro_tlb.rip_ver, buf, RIP_PRIV_RIP_VERSION_SIZE);
          break;
#endif
#endif

      case RIP_COMMON_FIRSTUSE:
          /* Ignore that one as not stored in the RO RIP */
          break;

#if defined(CONFIG_CUSTOM_PLUME)
#ifdef RIP_PRIV_CA_PEM_KEY
      case RIP_PRIV_CA_PEM_KEY:
         if (buf) {
             memcpy(rip_ro_p->rip_ro_tlb.ca_pem, buf, rip[index].length);
             rip_ro_p->rip_ro_tlb.ca_pem_size = rip[index].length;
         }
         else
             memset(rip_ro_p->rip_ro_tlb.ca_pem, 0, RIP_PRIV_CA_PEM_KEY_SIZE);
         break;
#endif // RIP_PRIV_CA_PEM_KEY
#ifdef RIP_PRIV_CLIENT_PEM_KEY
      case RIP_PRIV_CLIENT_PEM_KEY:
         if (buf) {
             memcpy(rip_ro_p->rip_ro_tlb.client_pem, buf, rip[index].length);
             rip_ro_p->rip_ro_tlb.client_pem_size = rip[index].length;
         }
         else
             memset(rip_ro_p->rip_ro_tlb.client_pem, 0, RIP_PRIV_CLIENT_PEM_KEY_SIZE);
         break;
#endif // RIP_PRIV_CLIENT_PEM_KEY
#ifdef RIP_PRIV_DEC_KEY
      case RIP_PRIV_DEC_KEY:
         if (buf) {
             memcpy(rip_ro_p->rip_ro_tlb.dec_key, buf, rip[index].length);
             rip_ro_p->rip_ro_tlb.dec_key_size = rip[index].length;
         }
         else
             memset(rip_ro_p->rip_ro_tlb.dec_key, 0, RIP_PRIV_DEC_KEY_SIZE);
         break;
#endif // RIP_PRIV_DEC_KEY
#ifdef RIP_PRIV_SEED
      case RIP_PRIV_SEED:
         memcpy(rip_ro_p->rip_ro_tlb.seed_key, buf, rip[index].length);
         break;
#endif // RIP_PRIV_SEED
#ifdef RIP_PRIV_MFG_DATE
     case RIP_PRIV_MFG_DATE:
         memcpy(rip_ro_p->rip_ro_tlb.mfg_date, buf, rip[index].length);
         break;
#endif // RIP_PRIV_MFG_DATE
#endif // CONFIG_CUSTOM_PLUME

      default:
          fprintf(stderr,"ERROR: Unknown ID %d (%s)\n", rip[index].id, rip[index].name);
          return -1;
    }
    return 0;
}

static int erase_block(mtd_info_t mtd_info, int fd, unsigned int offset)
{
    erase_info_t erase_info;
    int ret=0;

    if (!fd)
    {
        fprintf(stderr, "%s: Provide a valid file descriptor\n", __FUNCTION__);
        return -1;
    }

    if (offset >= mtd_info.size)
    {
        fprintf(stderr, "%s: offset is too big (%u)\n", __FUNCTION__, offset);
        return -1;
    }

    erase_info.start = offset;
    erase_info.length = mtd_info.erasesize;

    if(-1 == ioctl(fd, MEMERASE, &erase_info))
    {
        fprintf(stderr, "ERROR: Failed erasing eraseblock at offset %u (%s)\n",
                erase_info.start, strerror(errno));
        if (EIO == errno) /* Low-level IO error => block is bad */
        {
            if (-1 == ioctl(fd, MEMSETBADBLOCK, &offset))
            {
                fprintf(stderr, "Failed marking eraseblock at offset %u as bad (%s)\n",
                        offset, strerror(errno));
            }
            else
            {
                fprintf(stdout, "Eraseblock at offset %u has been marked as bad\n", offset);
            }
        }
        ret=-1;
    }

    return ret;
}

static int first_good_EB(mtd_info_t mtd_info, int fd)
{
    int offset;

    if (!fd)
    {
        fprintf(stderr, "%s: Provide a valid file descriptor\n", __FUNCTION__);
        return -1;
    }

    /* Check eraseblocks and take the first good one */
    for(offset=0 ; offset < mtd_info.size ; offset+=mtd_info.erasesize)
    {
        int ret = ioctl(fd, MEMGETBADBLOCK, &offset);
        if (ret > 0)
        {
            fprintf(stdout, "Bad block on %s at offset 0x%06x\n", RIP_MTD, offset);
        }
        else if (ret < 0 && errno != EINVAL) /* EINVAL == MEMGETBADBLOCK not supported => let's try on the first EB */
        {
            offset = 0;
            fprintf(stderr, "Failed to get bad block info on %s at offset 0x%06x: %d %s\n",
                    RIP_MTD, offset, errno, strerror(errno));
            break;
        }
        else
            break;
    }

    return offset;
}

static int write_full_rip(void *data, unsigned length)
{
    mtd_info_t mtd_info;
    int fd, offset, ret=0;
    char *buf=NULL;
    unsigned bufsize;

    if (-1 == (fd = open(RIP_MTD, O_RDWR)))
    {
        fprintf(stderr, "ERROR: Could not open file %s (%s)\n", RIP_MTD, strerror(errno));
        return -1;
    }

    if (-1 == ioctl(fd, MEMGETINFO, &mtd_info))
    {
        fprintf(stderr, "ERROR: MEMGETINFO ioctl failed (%s)\n", strerror(errno));
        close(fd);
        ret=-1;
        goto end;
    }

    if (-1 == (offset = first_good_EB(mtd_info, fd)))
    {
        ret=-1;
        goto end;
    }

    if (-1 == erase_block(mtd_info, fd, offset))
    {
        ret=-1;
        goto end;
    }

    lseek(fd, offset, SEEK_SET);  /* go to the first good eraseblock */

    /* Allocate a buffer whose size is a multiple of the page size */
    bufsize = (length % mtd_info.writesize) ? (length / mtd_info.writesize + 1)*mtd_info.writesize : length;

    if(NULL == (buf = malloc(bufsize)))
    {
        fprintf(stderr, "ERROR: Failed allocating memory\n");
        ret=-1;
        goto end;
    }
    memset(buf, 0xff, bufsize);
    memcpy(buf, data, length);

    if(-1 == (ret = write(fd, buf, bufsize)))
    {
        fprintf(stderr, "MTD write failure (%s)\n", strerror(errno));
        if (EIO == errno) /* Low-level IO error => block might be becoming bad */
        {
            (void) erase_block(mtd_info, fd, offset);
        }
        ret = -1;
    }
    else if (ret != bufsize)
    {
        fprintf(stderr, "Incomplete write - Wrote %d bytes instead of %d bytes\n", ret, mtd_info.writesize);
        ret = -1;
    }

end:
    free(buf);
    close(fd);
    return ret;
}

int rip_to_certi_files(void)
{
    int fd, i, ret=1;
    FILE *pFile = NULL;
    char filename[1024];
    mtd_info_t mtd_info;
    unsigned int offset, bufsize;
    unsigned char *buf=NULL;
    PMW_RIP_RO rip_ro_p;
    unsigned long checksum;

    if (-1 == (fd = open(RIP_MTD, O_RDWR)))
    {
        fprintf(stderr, "ERROR: Could not open file %s (%s)\n", RIP_MTD, strerror(errno));
        return -1;
    }

    if (-1 == ioctl(fd, MEMGETINFO, &mtd_info))
    {
        fprintf(stderr, "ERROR: MEMGETINFO ioctl failed (%s)\n", strerror(errno));
        ret=-1;
        goto end;
    }

    if (-1 == (offset = first_good_EB(mtd_info, fd)))
    {
        ret=-1;
        goto end;
    }

    lseek(fd, offset, SEEK_SET);  /* go to the first good eraseblock */

    /* Allocate a buffer whose size is a multiple of the page size and contains enough space to store the full RIP and CRC */
    bufsize = (sizeof(MW_RIP_RO) % mtd_info.writesize) ?
      (sizeof(MW_RIP_RO) / mtd_info.writesize + 1)*mtd_info.writesize : sizeof(MW_RIP_RO);

    if(NULL == (buf = malloc(bufsize)))
    {
        fprintf(stderr, "%s: Failed to allocate memory\n", __FUNCTION__);
        ret = -1;
        goto end;
    }

    if(-1 == (ret = read(fd, buf, bufsize)))
    {
        fprintf(stderr, "MTD read failure (%s)\n", strerror(errno));
        goto end;
    }
    else if (ret < sizeof(MW_RIP_RO))
    {
        fprintf(stderr, "Incomplete read - Read %d bytes\n", ret);
        ret = -1;
        goto end;
    }

    rip_ro_p = (PMW_RIP_RO)buf;

    if(CRC32_INIT_VALUE == rip_ro_p->checksum) /* Empty RIP */
    {
        ret = 0;
        goto end;
    }

    /* Calculate CRC */
    checksum = lib_get_crc32(rip_ro_p->padding, sizeof(rip_ro_p->padding), CRC32_INIT_VALUE);

    if(checksum == rip_ro_p->checksum)
        ;
        //fprintf(stdout, "Checksum OK: 0x%lx - 0x%lx\n", checksum, rip_ro_p->checksum);
    else
    {
        fprintf(stderr, "Checksum NOK: 0x%lx - 0x%lx => Corrupted RIP!?!\n", checksum, rip_ro_p->checksum);
        ret = -1;
        goto end;
    }

    for(i=0 ; i < sizeof(rip)/sizeof(rip[0]) ; ++i)
    {
        if(NULL == rip[i].filename)
            continue;

	    if( (rip[i].id == RIP_COMMON_CA_PEM_KEY) || (rip[i].id == RIP_COMMON_CLIENT_PEM_KEY) || (rip[i].id == RIP_COMMON_DEC_KEY) )
	    {
        	snprintf(filename, sizeof(filename), "%s%s", "/data/", rip[i].filename);
        	if(NULL == (pFile = fopen(filename, "w")))
        	{
        	    fprintf(stderr, "Failed creating file %s with error %s (%d)\n",
        	            filename, strerror(errno), errno);
        	    ret = -1;
        	    goto end;
        	}

        	switch(rip[i].id)
        	{
        	    case RIP_COMMON_CA_PEM_KEY:
                    fwrite(rip_ro_p->rip_ro_tlb.ca_pem, rip_ro_p->rip_ro_tlb.ca_pem_size, 1, pFile);
        	        break;
        	    case RIP_COMMON_CLIENT_PEM_KEY:
        	        fwrite(rip_ro_p->rip_ro_tlb.client_pem, rip_ro_p->rip_ro_tlb.client_pem_size, 1, pFile);
        	        break;
        	    case RIP_COMMON_DEC_KEY:
        	        fwrite(rip_ro_p->rip_ro_tlb.dec_key, rip_ro_p->rip_ro_tlb.dec_key_size, 1, pFile);
        	        break;

        	    default:
        	        fprintf(stderr, "Unknown ID %s\n", rip[i].name);
        	        //ret = -1;
        	        //goto end;
			break;
        	}
        	fclose(pFile);
	    }
    }

end:
    close(fd);
    free(buf);
    return ret;
}

int dump_rip_to_files(char *path)
{
    int fd, i, ret=1;
    FILE *pFile = NULL;
    char filename[1024];
    mtd_info_t mtd_info;
    unsigned int offset, bufsize;
    unsigned char *buf=NULL;
    PMW_RIP_RO rip_ro_p;
    unsigned long checksum;

    if (-1 == (fd = open(RIP_MTD, O_RDWR)))
    {
        fprintf(stderr, "ERROR: Could not open file %s (%s)\n", RIP_MTD, strerror(errno));
        return -1;
    }

    if (-1 == ioctl(fd, MEMGETINFO, &mtd_info))
    {
        fprintf(stderr, "ERROR: MEMGETINFO ioctl failed (%s)\n", strerror(errno));
        ret=-1;
        goto end;
    }

    if (-1 == (offset = first_good_EB(mtd_info, fd)))
    {
        ret=-1;
        goto end;
    }

    lseek(fd, offset, SEEK_SET);  /* go to the first good eraseblock */

    /* Allocate a buffer whose size is a multiple of the page size and contains enough space to store the full RIP and CRC */
    bufsize = (sizeof(MW_RIP_RO) % mtd_info.writesize) ?
      (sizeof(MW_RIP_RO) / mtd_info.writesize + 1)*mtd_info.writesize : sizeof(MW_RIP_RO);

    if(NULL == (buf = malloc(bufsize)))
    {
        fprintf(stderr, "%s: Failed to allocate memory\n", __FUNCTION__);
        ret = -1;
        goto end;
    }

    if(-1 == (ret = read(fd, buf, bufsize)))
    {
        fprintf(stderr, "MTD read failure (%s)\n", strerror(errno));
        goto end;
    }
    else if (ret < sizeof(MW_RIP_RO))
    {
        fprintf(stderr, "Incomplete read - Read %d bytes\n", ret);
        ret = -1;
        goto end;
    }

    rip_ro_p = (PMW_RIP_RO)buf;

    if(CRC32_INIT_VALUE == rip_ro_p->checksum) /* Empty RIP */
    {
        ret = 0;
        goto end;
    }

    /* Calculate CRC */
    checksum = lib_get_crc32(rip_ro_p->padding, sizeof(rip_ro_p->padding), CRC32_INIT_VALUE);

    if(checksum == rip_ro_p->checksum)
        ;
        //fprintf(stdout, "Checksum OK: 0x%lx - 0x%lx\n", checksum, rip_ro_p->checksum);
    else
    {
        fprintf(stderr, "Checksum NOK: 0x%lx - 0x%lx => Corrupted RIP!?!\n", checksum, rip_ro_p->checksum);
        ret = -1;
        goto end;
    }

    for(i=0 ; i < sizeof(rip)/sizeof(rip[0]) ; ++i)
    {
        if(NULL == rip[i].filename)
            continue;

        snprintf(filename, sizeof(filename), "%s/%s", path, rip[i].filename);
        if(NULL == (pFile = fopen(filename, "w")))
        {
            fprintf(stderr, "Failed creating file %s with error %s (%d)\n",
                    filename, strerror(errno), errno);
            ret = -1;
            goto end;
        }

        switch(rip[i].id)
        {
            case RIP_COMMON_SERIAL_NUMBER:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.serial_number);
                break;
            case RIP_COMMON_WAN_ADDR:
                fprintf(pFile, "%02x:%02x:%02x:%02x:%02x:%02x",
                        rip_ro_p->rip_ro_tlb.mac_addr_base[0], rip_ro_p->rip_ro_tlb.mac_addr_base[1],
                        rip_ro_p->rip_ro_tlb.mac_addr_base[2], rip_ro_p->rip_ro_tlb.mac_addr_base[3],
                        rip_ro_p->rip_ro_tlb.mac_addr_base[4], rip_ro_p->rip_ro_tlb.mac_addr_base[5]);
                break;
            case RIP_COMMON_MANUFACTURER:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.manufacturer);
                break;
            case RIP_COMMON_MANUFACTURER_OUI:
                fwrite(rip_ro_p->rip_ro_tlb.manufacturer_oui, rip[i].length, 1, pFile);
                break;
            case RIP_COMMON_MANUFACTURER_URL:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.manufacturer_url);
                break;
            case RIP_COMMON_MODELNAME:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.model_name);
                break;
            case RIP_COMMON_PRODUCT_CLASS:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.product_class);
                break;
            case RIP_COMMON_HARDWARE_VERSION:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.hardware_version);
                break;
            case RIP_COMMON_BOOTLOADER_VERSION:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.bootloader_version);
                break;
            case RIP_COMMON_PUBLIC_DSA_NORM:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.public_rsa_normal);
                break;
            case RIP_COMMON_PUBLIC_DSA_RESC:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.public_rsa_rescue);
                break;
            case RIP_COMMON_CLIENT_CERTIFICATE:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.client_certificate);
                break;
            case RIP_COMMON_PRIVATE_KEY:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.private_key);
                break;
            case RIP_COMMON_WLAN_KEY:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.wlan_key);
                break;
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP
            case RIP_COMMON_WLAN_BRCM_SROM_MAP:
                fwrite(rip_ro_p->rip_ro_tlb.wlan_brcm_srom_map1, rip[i].length, 1, pFile);
                break;
#endif
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP2
            case RIP_COMMON_WLAN_BRCM_SROM_MAP2:
                fwrite(rip_ro_p->rip_ro_tlb.wlan_brcm_srom_map2, rip[i].length, 1, pFile);
                break;
#endif
#ifdef RIP_PRIV_FACTORY
            case RIP_COMMON_FACTORY:
                fwrite(rip_ro_p->rip_ro_tlb.factory, rip[i].length, 1, pFile);
                break;
#endif
            case RIP_COMMON_FIRSTUSE:
                /* Ignore that one as not stored in the RO RIP */
                break;

#ifdef RIP_PRIV_ADMIN_PWD
            case RIP_COMMON_ADMIN_PWD:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.admin_pwd);
                break;
#endif
            case RIP_PRIV_WLAN_SSID:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.wlan_ssid);
                break;

#if defined(CONFIG_CUSTOM_MAXIS) || defined(CONFIG_CUSTOM_CELCOM)
#ifdef RIP_PRIV_SPT_PWD
            case RIP_COMMON_SPT_PWD:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.spt_pwd);
                break;
#endif
#ifdef RIP_PRIV_USER_PWD
            case RIP_COMMON_USER_PWD:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.user_pwd);
                break;
#endif
#ifdef RIP_PRIV_RIP_VERSION
            case RIP_COMMON_RIP_VERSION:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.rip_ver);
                break;
#endif
#endif

#if defined(CONFIG_CUSTOM_PLUME)
#ifdef RIP_PRIV_CA_PEM_KEY
            case RIP_PRIV_CA_PEM_KEY:
                fprintf(stderr,"2 ca_pem_size: %d (%d)\n", sizeof(rip_ro_p->rip_ro_tlb.ca_pem), rip_ro_p->rip_ro_tlb.ca_pem_size);
                fwrite(rip_ro_p->rip_ro_tlb.ca_pem, rip_ro_p->rip_ro_tlb.ca_pem_size, 1, pFile);
                break;
#endif // RIP_PRIV_CA_PEM_KEY
#ifdef RIP_PRIV_CLIENT_PEM_KEY
            case RIP_PRIV_CLIENT_PEM_KEY:
                fprintf(stderr,"2 clien_pem_size: %d (%d)\n", sizeof(rip_ro_p->rip_ro_tlb.client_pem), rip_ro_p->rip_ro_tlb.client_pem_size);
                fwrite(rip_ro_p->rip_ro_tlb.client_pem, rip_ro_p->rip_ro_tlb.client_pem_size, 1, pFile);
                break;
#endif // RIP_PRIV_CLIENT_PEM_KEY
#ifdef RIP_PRIV_DEC_KEY
            case RIP_PRIV_DEC_KEY:
                fprintf(stderr,"2 clien_pem_size: %d (%d)\n", sizeof(rip_ro_p->rip_ro_tlb.dec_key), rip_ro_p->rip_ro_tlb.dec_key_size);
                fwrite(rip_ro_p->rip_ro_tlb.dec_key, rip_ro_p->rip_ro_tlb.dec_key_size, 1, pFile);
                break;
#endif // RIP_PRIV_DEC_KEY
#ifdef RIP_PRIV_SEED
            case RIP_PRIV_SEED:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.seed_key);
                break;
#endif // RIP_PRIV_SEED
#ifdef RIP_PRIV_MFG_DATE
           case RIP_PRIV_MFG_DATE:
                fprintf(pFile, "%s", rip_ro_p->rip_ro_tlb.mfg_date);
                break;
#endif // RIP_PRIV_MFG_DATE
#endif // CONFIG_CUSTOM_PLUME
            default:
                fprintf(stderr, "Unknown ID %s\n", rip[i].name);
                ret = -1;
                goto end;
        }

        fclose(pFile);
    }

end:
    close(fd);
    free(buf);
    return ret;
}

int write_rip_use_files(char *path)
{
    int i, res=0;
    FILE *fd;
    MW_RIP_RO rip_ro;
#if defined(CONFIG_CUSTOM_PLUME)
    unsigned char buf[1024*7];
#else
    unsigned char buf[2048];
#endif
    char fullPath[1024]={0};

    /* Start retrieving informations from files and write them to rip_ro */
    for (i=0 ; i<(sizeof(rip) / sizeof(rip[0])) ; ++i)
    {
        sprintf(fullPath, "%s%s", path, rip[i].filename);
        if(path && rip[i].filename && (fd = fopen(fullPath, "r")))
        {
            size_t read_bytes;
            int length;

            /* Handle the special case of the MAC address in file with ':' */
            length = (rip[i].flags == FLAG_MACADDRESS) ? rip[i].length*2+5 : rip[i].length;

            if (0 == (read_bytes=fread(buf, 1, length, fd)))
            {
                fprintf(stderr, "ERROR: Failed reading file %s\n", rip[i].filename);
                fclose(fd);
                return -1;
            }
            if(length > read_bytes) /* Set to 0 the remaining unused bytes */
                memset(&buf[read_bytes], 0, length-read_bytes);

            /* Remove trailing newlines that could have been left at the end of the text files */
            if(rip[i].flags & FLAG_STRING)
            {
                int j;
                for(j=read_bytes-1 ; j >= 0 && (buf[j] == '\r' || buf[j] == '\n') ; --j)
                {
                    buf[j] = '\0';
                }
            }
#if defined(CONFIG_CUSTOM_PLUME)
            rip[i].length = read_bytes;
#endif // CONFIG_CUSTOM_PLUME
            res = fill_rip_struct(&rip_ro, buf, i);
            fclose(fd);
        }
        else /* Use the default value */
        {
            res = fill_rip_struct(&rip_ro, NULL, i);
        }
        if(res)
        {
            fprintf(stderr, "ERROR: Filling RIP struct\n");
            return -1;
        }
    }

    /* Calculate CRC */
    rip_ro.checksum = lib_get_crc32(rip_ro.padding, sizeof(rip_ro.padding), CRC32_INIT_VALUE);

    /* Backup RIP to files */
    sprintf(fullPath, "%s%s", path, "backup");
    res = mkdir(fullPath, 0777);
    if(!dump_rip_to_files(fullPath) && !res) /* RIP is empty, the backup dir was created */
        rmdir(fullPath);

    /* Write the data to the RIP in flash */
    res = write_full_rip((void*)&rip_ro, sizeof(rip_ro));

    return res;
}
#if 1

int dump_rip(MW_RIP_RO *rip_ro_p)
{
    int fd, ret=1;
    mtd_info_t mtd_info;
    unsigned int offset, bufsize;
    unsigned char *buf=NULL;
    unsigned long checksum;

    if (-1 == (fd = open(RIP_MTD, O_RDWR)))
    {
        fprintf(stderr, "ERROR: Could not open file %s (%s)\n", RIP_MTD, strerror(errno));
        return -1;
    }

    if (-1 == ioctl(fd, MEMGETINFO, &mtd_info))
    {
        fprintf(stderr, "ERROR: MEMGETINFO ioctl failed (%s)\n", strerror(errno));
        ret=-1;
        goto end;
    }

    if (-1 == (offset = first_good_EB(mtd_info, fd)))
    {
        ret=-1;
        goto end;
    }

    lseek(fd, offset, SEEK_SET);  /* go to the first good eraseblock */

    /* Allocate a buffer whose size is a multiple of the page size and contains enough space to store the full RIP and CRC */
    bufsize = (sizeof(MW_RIP_RO) % mtd_info.writesize) ?
      (sizeof(MW_RIP_RO) / mtd_info.writesize + 1)*mtd_info.writesize : sizeof(MW_RIP_RO);

    if(NULL == (buf = malloc(bufsize)))
    {
        fprintf(stderr, "%s: Failed to allocate memory\n", __FUNCTION__);
        ret = -1;
        goto end;
    }

    if(-1 == (ret = read(fd, buf, bufsize)))
    {
        fprintf(stderr, "MTD read failure (%s)\n", strerror(errno));
        goto end;
    }
    else if (ret < sizeof(MW_RIP_RO))
    {
        fprintf(stderr, "Incomplete read - Read %d bytes\n", ret);
        ret = -1;
        goto end;
    }

    //rip_ro_p = (PMW_RIP_RO)buf;
    memcpy(rip_ro_p, buf,sizeof(MW_RIP_RO));

    if(CRC32_INIT_VALUE == rip_ro_p->checksum) /* Empty RIP */
    {
        fprintf(stdout, "RIP: Rip is emptry\n");
        ret = 0;
        goto end;
    }

    /* Calculate CRC */
    checksum = lib_get_crc32(rip_ro_p->padding, sizeof(rip_ro_p->padding), CRC32_INIT_VALUE);

    if(checksum == rip_ro_p->checksum){
        fprintf(stdout, "RIP: Checksum OK: 0x%lx - 0x%lx\n", checksum, rip_ro_p->checksum);
    }
    else
    {
        fprintf(stderr, "RIP: Checksum NOK: 0x%lx - 0x%lx => Corrupted RIP!?!\n", checksum, rip_ro_p->checksum);
        ret = -1;
        goto end;
    }

end:
    close(fd);
    free(buf);
    return ret;
}

int write_rip_field(char *field, char *value)
{
    int i, res=0;
    MW_RIP_RO rip_ro;

    /* Dump RIP */
    dump_rip(&rip_ro);

    if (!field || !value)
        return -1;

    /* Start retrieving informations from files and write them to rip_ro */
    for (i=0 ; i<(sizeof(rip) / sizeof(rip[0])) ; ++i)
    {
        if (strcasecmp(rip[i].name, field))
            continue;

        fill_rip_struct(&rip_ro, (const unsigned char *)value, i);
        break;
    }

    /* Calculate CRC */
    rip_ro.checksum = lib_get_crc32(rip_ro.padding, sizeof(rip_ro.padding), CRC32_INIT_VALUE);

    /* Write the data to the RIP in flash */
    res = write_full_rip((void*)&rip_ro, sizeof(rip_ro));

    return res;
}
#endif

int set_default_rip(void)
{
    return write_rip_use_files(NULL);
}

void rip_list_fields(void)
{
    int i, rip_size;

    rip_size = sizeof(rip) / sizeof(rip[0]);

    for(i = 0; i<rip_size; i++)
       printf("%s (%lX) size %lu\n", rip[i].name, (long unsigned int)rip[i].id, rip[i].length);
}

static int getIdFromName(char *field)
{
    int i, rip_size;

    if(NULL == field)
        return -1;

    rip_size = sizeof(rip) / sizeof(rip[0]);

    for(i = 0; i<rip_size; i++)
        if(!strcmp(field, rip[i].name))
        {
            return rip[i].id;
        }

    return -1;
}

int print_mtd_rip(char* field)
{
    int fd, i, ret=1;
    mtd_info_t mtd_info;
    unsigned int offset, bufsize;
    unsigned char *buf=NULL;
    PMW_RIP_RO rip_ro_p;
    unsigned long checksum;
    int field_id=-1;

    /* Check that field has valid name */
    if (field && -1 == (field_id = getIdFromName(field)))
    {
        fprintf(stderr, "ERROR: Not a valid field (%s)\n", field);
        return -1;
    }

    if (-1 == (fd = open(RIP_MTD, O_RDWR)))
    {
        fprintf(stderr, "ERROR: Could not open file %s (%s)\n", RIP_MTD, strerror(errno));
        return -1;
    }

    if (-1 == ioctl(fd, MEMGETINFO, &mtd_info))
    {
        fprintf(stderr, "ERROR: MEMGETINFO ioctl failed (%s)\n", strerror(errno));
        ret=-1;
        goto end;
    }

    if (-1 == (offset = first_good_EB(mtd_info, fd)))
    {
        ret=-1;
        goto end;
    }

    lseek(fd, offset, SEEK_SET);  /* go to the first good eraseblock */

    /* Allocate a buffer whose size is a multiple of the page size and contains enough space to store the full RIP and CRC */
    bufsize = (sizeof(MW_RIP_RO) % mtd_info.writesize) ?
      (sizeof(MW_RIP_RO) / mtd_info.writesize + 1)*mtd_info.writesize : sizeof(MW_RIP_RO);

    if(NULL == (buf = malloc(bufsize)))
    {
        fprintf(stderr, "%s: Failed to allocate memory\n", __FUNCTION__);
        ret = -1;
        goto end;
    }

    if(-1 == (ret = read(fd, buf, bufsize)))
    {
        fprintf(stderr, "MTD read failure (%s)\n", strerror(errno));
        goto end;
    }
    else if (ret < sizeof(MW_RIP_RO))
    {
        fprintf(stderr, "Incomplete read - Read %d bytes\n", ret);
        ret = -1;
        goto end;
    }

    rip_ro_p = (PMW_RIP_RO)buf;

    if(CRC32_INIT_VALUE == rip_ro_p->checksum) /* Empty RIP */
    {
        ret = 0;
        goto end;
    }

    /* Calculate CRC */
    checksum = lib_get_crc32(rip_ro_p->padding, sizeof(rip_ro_p->padding), CRC32_INIT_VALUE);

    if(checksum == rip_ro_p->checksum)
        ;
        //fprintf(stdout, "Checksum OK: 0x%lx - 0x%lx\n", checksum, rip_ro_p->checksum);
    else
        fprintf(stderr, "Checksum NOK: 0x%lx - 0x%lx => Corrupted RIP!?!\n", checksum, rip_ro_p->checksum);

    if(!field)
    {
        fprintf(stdout, "RIP data:\n");
        fprintf(stdout, "\t%s: %.*s\n", "Serial number", RIP_COMMON_SERIAL_NUMBER_SIZE,
                rip_ro_p->rip_ro_tlb.serial_number);
        fprintf(stdout, "\t%s: %02X:%02X:%02X:%02X:%02X:%02X\n", "WAN Mac address",
                rip_ro_p->rip_ro_tlb.mac_addr_base[0], rip_ro_p->rip_ro_tlb.mac_addr_base[1],
                rip_ro_p->rip_ro_tlb.mac_addr_base[2], rip_ro_p->rip_ro_tlb.mac_addr_base[3],
                rip_ro_p->rip_ro_tlb.mac_addr_base[4], rip_ro_p->rip_ro_tlb.mac_addr_base[5]);
        fprintf(stdout, "\t%s: %.*s\n", "Manufacturer", RIP_COMMON_MANUFACTURER_SIZE,
                rip_ro_p->rip_ro_tlb.manufacturer);
        fprintf(stdout, "\t%s: 0x%02X%02X%02X\n", "Manufacturer OUI",
                rip_ro_p->rip_ro_tlb.manufacturer_oui[0],
                rip_ro_p->rip_ro_tlb.manufacturer_oui[1],
                rip_ro_p->rip_ro_tlb.manufacturer_oui[2]);
        fprintf(stdout, "\t%s: %.*s\n", "Manufacturer URL", RIP_COMMON_MANUFACTURER_URL_SIZE,
                rip_ro_p->rip_ro_tlb.manufacturer_url);
        fprintf(stdout, "\t%s: %.*s\n", "Model name", RIP_COMMON_MODELNAME_SIZE,
                rip_ro_p->rip_ro_tlb.model_name);
        fprintf(stdout, "\t%s: %.*s\n", "Product class", RIP_COMMON_PRODUCT_CLASS_SIZE,
                rip_ro_p->rip_ro_tlb.product_class);
        fprintf(stdout, "\t%s: %.*s\n", "Hardware version", RIP_COMMON_HARDWARE_VERSION_SIZE,
                rip_ro_p->rip_ro_tlb.hardware_version);
        fprintf(stdout, "\t%s: %.*s\n", "Bootloader version", RIP_COMMON_BOOTLOADER_VERSION_SIZE,
                rip_ro_p->rip_ro_tlb.bootloader_version);
        fprintf(stdout, "\t%s: large binary data (use writerip -r PUBLIC_DSA_NORM)\n", "Public RSA key");
        fprintf(stdout, "\t%s: large binary data (use writerip -r PUBLIC_DSA_RESC)\n", "Public RSA rescue key");
        fprintf(stdout, "\t%s:\n%.*s\n", "Client certificate", RIP_COMMON_CLIENT_CERTIFICATE_SIZE,
                rip_ro_p->rip_ro_tlb.client_certificate);
        fprintf(stdout, "\t%s:\n%.*s\n", "Private key", RIP_COMMON_PRIVATE_KEY_SIZE,
                rip_ro_p->rip_ro_tlb.private_key);
        fprintf(stdout, "\t%s: %.*s\n", "WLAN key", RIP_COMMON_WLAN_KEY_SIZE,
                rip_ro_p->rip_ro_tlb.wlan_key);
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP
        fprintf(stdout, "\t%s: large binary data (use writerip -r WLAN_BRCM_SROM_MAP)\n", "WLAN SROM map");
#endif
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP2
        fprintf(stdout, "\t%s: large binary data (use writerip -r WLAN_BRCM_SROM_MAP2)\n", "WLAN SROM map 2");
#endif
#ifdef RIP_PRIV_ADMIN_PWD
        fprintf(stdout, "\t%s: %.*s\n", "Admin Pw", RIP_PRIV_ADMIN_PWD_SIZE, rip_ro_p->rip_ro_tlb.admin_pwd);
#endif
        fprintf(stdout, "\t%s: %.*s\n", "SSID", RIP_PRIV_WLAN_SSID_SIZE, rip_ro_p->rip_ro_tlb.wlan_ssid);

#if defined(CONFIG_CUSTOM_MAXIS) || defined(CONFIG_CUSTOM_CELCOM)
#ifdef RIP_PRIV_SPT_PWD
        fprintf(stdout, "\t%s: %.*s\n", "Spt Pw", RIP_PRIV_SPT_PWD_SIZE, rip_ro_p->rip_ro_tlb.spt_pwd);
#endif
#ifdef RIP_PRIV_USER_PWD
        fprintf(stdout, "\t%s: %.*s\n", "User Pw", RIP_PRIV_USER_PWD_SIZE, rip_ro_p->rip_ro_tlb.user_pwd);
#endif
#ifdef RIP_PRIV_RIP_VERSION
        fprintf(stdout, "\t%s: %.*s\n", "Rip Version", RIP_PRIV_RIP_VERSION_SIZE, rip_ro_p->rip_ro_tlb.rip_ver);
#endif
#endif

#if defined(CONFIG_CUSTOM_PLUME)
#ifdef RIP_PRIV_CA_PEM_KEY
       fprintf(stdout, "\t%s: large binary data (use writerip -r CA_PEM_KEY)\n", "CA PEM key");
#endif // RIP_PRIV_CA_PEM_KEY
#ifdef RIP_PRIV_CLIENT_PEM_KEY
       fprintf(stdout, "\t%s: large binary data (use writerip -r CLIENT_PEM_KEY)\n", "CLIENT PEM key");
#endif // RIP_PRIV_CLIENT_PEM_KEY
#ifdef RIP_PRIV_DEC_KEY
       fprintf(stdout, "\t%s: large binary data (use writerip -r DEC_KEY)\n", "DEC KEY key");
#endif // RIP_PRIV_DEC_KEY
#ifdef RIP_PRIV_SEED
       fprintf(stdout, "\t%s: %.*s\n", "SEED Value", strlen((char *)rip_ro_p->rip_ro_tlb.seed_key), rip_ro_p->rip_ro_tlb.seed_key);
#endif // RIP_PRIV_SEED
#ifdef RIP_PRIV_MFG_DATE
       fprintf(stdout, "\t%s: %.*s\n", "MFG Date", strlen((char *)rip_ro_p->rip_ro_tlb.mfg_date), rip_ro_p->rip_ro_tlb.mfg_date);
#endif // RIP_PRIV_MFG_DATE
#endif // CONFIG_CUSTOM_PLUME

#ifdef RIP_PRIV_FACTORY
        fprintf(stdout, "\t%s: %.*s\n", "Factory", RIP_PRIV_FACTORY_SIZE, rip_ro_p->rip_ro_tlb.factory);
#endif
    }
    else
    {
        switch(field_id)
        {
            case RIP_COMMON_SERIAL_NUMBER:
                fprintf(stdout, "%.*s\n",RIP_COMMON_SERIAL_NUMBER_SIZE, rip_ro_p->rip_ro_tlb.serial_number);
                break;
            case RIP_COMMON_WAN_ADDR:
                fprintf(stdout, "%02X:%02X:%02X:%02X:%02X:%02X\n",
                        rip_ro_p->rip_ro_tlb.mac_addr_base[0], rip_ro_p->rip_ro_tlb.mac_addr_base[1],
                        rip_ro_p->rip_ro_tlb.mac_addr_base[2], rip_ro_p->rip_ro_tlb.mac_addr_base[3],
                        rip_ro_p->rip_ro_tlb.mac_addr_base[4], rip_ro_p->rip_ro_tlb.mac_addr_base[5]);
                break;
            case RIP_COMMON_MANUFACTURER:
                fprintf(stdout, "%.*s\n", RIP_COMMON_MANUFACTURER_SIZE,
                        rip_ro_p->rip_ro_tlb.manufacturer);
                break;
            case RIP_COMMON_MANUFACTURER_OUI:
                fprintf(stdout, "0x%02X%02X%02X\n",
                        rip_ro_p->rip_ro_tlb.manufacturer_oui[0],
                        rip_ro_p->rip_ro_tlb.manufacturer_oui[1],
                        rip_ro_p->rip_ro_tlb.manufacturer_oui[2]);
                break;
            case RIP_COMMON_MANUFACTURER_URL:
                fprintf(stdout, "%.*s\n", RIP_COMMON_MANUFACTURER_URL_SIZE,
                        rip_ro_p->rip_ro_tlb.manufacturer_url);
                break;
            case RIP_COMMON_MODELNAME:
                fprintf(stdout, "%.*s\n", RIP_COMMON_MODELNAME_SIZE,
                        rip_ro_p->rip_ro_tlb.model_name);
                break;
            case RIP_COMMON_PRODUCT_CLASS:
                fprintf(stdout, "%.*s\n", RIP_COMMON_PRODUCT_CLASS_SIZE,
                        rip_ro_p->rip_ro_tlb.product_class);
                break;
            case RIP_COMMON_HARDWARE_VERSION:
                fprintf(stdout, "%.*s\n", RIP_COMMON_HARDWARE_VERSION_SIZE,
                        rip_ro_p->rip_ro_tlb.hardware_version);
                break;
            case RIP_COMMON_BOOTLOADER_VERSION:
                fprintf(stdout, "%.*s\n", RIP_COMMON_BOOTLOADER_VERSION_SIZE,
                        rip_ro_p->rip_ro_tlb.bootloader_version);
                break;
            case RIP_COMMON_PUBLIC_DSA_NORM:
                for(i=0 ; i<RIP_COMMON_PUBLIC_DSA_NORM_SIZE ; ++i)
                    fprintf(stdout, "0x%02X ", rip_ro_p->rip_ro_tlb.public_rsa_normal[i]);
                fprintf(stdout, "\n");
                break;
            case RIP_COMMON_PUBLIC_DSA_RESC:
                for(i=0 ; i<RIP_COMMON_PUBLIC_DSA_RESC_SIZE ; ++i)
                    fprintf(stdout, "0x%02X ", rip_ro_p->rip_ro_tlb.public_rsa_rescue[i]);
                fprintf(stdout, "\n");
                break;
            case RIP_COMMON_CLIENT_CERTIFICATE:
                fprintf(stdout, "%.*s\n", RIP_COMMON_CLIENT_CERTIFICATE_SIZE,
                        rip_ro_p->rip_ro_tlb.client_certificate);
                break;
            case RIP_COMMON_PRIVATE_KEY:
                fprintf(stdout, "%.*s\n", RIP_COMMON_PRIVATE_KEY_SIZE,
                        rip_ro_p->rip_ro_tlb.private_key);
                break;
            case RIP_COMMON_WLAN_KEY:
                fprintf(stdout, "%.*s\n", RIP_COMMON_WLAN_KEY_SIZE,
                        rip_ro_p->rip_ro_tlb.wlan_key);
                break;
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP
            case RIP_COMMON_WLAN_BRCM_SROM_MAP:
                for(i=0 ; i< RIP_PRIV_WLAN_BRCM_SROM_MAP_SIZE; i++)
                    fprintf(stdout, "[%d]0x%02X ", i, rip_ro_p->rip_ro_tlb.wlan_brcm_srom_map1[i]);
                fprintf(stdout, "\n");
                break;
#endif
#ifdef RIP_PRIV_WLAN_BRCM_SROM_MAP2
            case RIP_COMMON_WLAN_BRCM_SROM_MAP2:
                for(i=0 ; i< RIP_PRIV_WLAN_BRCM_SROM_MAP2_SIZE; i++)
                    fprintf(stdout, "[%d]0x%02X ",i, rip_ro_p->rip_ro_tlb.wlan_brcm_srom_map2[i]);
                fprintf(stdout, "\n");
                break;
#endif
#ifdef RIP_PRIV_ADMIN_PWD
            case RIP_COMMON_ADMIN_PWD:
                fprintf(stdout, "%.*s\n", RIP_PRIV_ADMIN_PWD_SIZE,
                        rip_ro_p->rip_ro_tlb.admin_pwd);
                break;
#endif
            case RIP_PRIV_WLAN_SSID:
                fprintf(stdout, "%.*s\n", RIP_PRIV_WLAN_SSID_SIZE,
                        rip_ro_p->rip_ro_tlb.wlan_ssid);
                break;                

#if defined(CONFIG_CUSTOM_MAXIS) || defined(CONFIG_CUSTOM_CELCOM)
#ifdef RIP_PRIV_SPT_PWD
            case RIP_COMMON_SPT_PWD:
                fprintf(stdout, "%.*s\n", RIP_PRIV_SPT_PWD_SIZE,
                        rip_ro_p->rip_ro_tlb.spt_pwd);
                break;
#endif
#ifdef RIP_PRIV_USER_PWD
            case RIP_COMMON_USER_PWD:
                fprintf(stdout, "%.*s\n", RIP_PRIV_USER_PWD_SIZE,
                        rip_ro_p->rip_ro_tlb.user_pwd);
                break;
#endif
#ifdef RIP_PRIV_RIP_VERSION
            case RIP_COMMON_RIP_VERSION:
                fprintf(stdout, "%.*s\n", RIP_PRIV_RIP_VERSION_SIZE,
                        rip_ro_p->rip_ro_tlb.rip_ver);
                break;
#endif
#endif

#ifdef RIP_PRIV_FACTORY
            case RIP_COMMON_FACTORY:
                fprintf(stdout, "%.*s\n", RIP_PRIV_FACTORY_SIZE,
                        rip_ro_p->rip_ro_tlb.factory);
                break;
#endif

#if defined(CONFIG_CUSTOM_PLUME)
#ifdef RIP_PRIV_CA_PEM_KEY
           case RIP_COMMON_CA_PEM_KEY:
                for(i=0 ; i < rip_ro_p->rip_ro_tlb.ca_pem_size ; ++i)
                    fprintf(stdout, "%x", rip_ro_p->rip_ro_tlb.ca_pem[i]);
                break;
#endif // RIP_PRIV_CA_PEM_KEY
#ifdef RIP_PRIV_CLIENT_PEM_KEY
           case RIP_COMMON_CLIENT_PEM_KEY:
                for(i=0 ; i < rip_ro_p->rip_ro_tlb.client_pem_size ; ++i)
                    fprintf(stdout, "%x", rip_ro_p->rip_ro_tlb.client_pem[i]);
                break;
#endif // RIP_PRIV_CLIENT_PEM_KEY
#ifdef RIP_PRIV_DEC_KEY
           case RIP_COMMON_DEC_KEY:
                for(i=0 ; i < rip_ro_p->rip_ro_tlb.dec_key_size ; ++i)
                    fprintf(stdout, "%x", rip_ro_p->rip_ro_tlb.dec_key[i]);
                break;
#endif // RIP_PRIV_DEC_KEY
#ifdef RIP_PRIV_SEED
           case RIP_COMMON_SEED:
                fprintf(stdout, "%.*s\n", strlen((char*)rip_ro_p->rip_ro_tlb.seed_key),
                        rip_ro_p->rip_ro_tlb.seed_key);
                break;
#endif // RIP_PRIV_SEED
#ifdef RIP_PRIV_MFG_DATE
           case RIP_PRIV_MFG_DATE:
                fprintf(stdout, "%.*s\n", strlen((char*)rip_ro_p->rip_ro_tlb.mfg_date),
                        rip_ro_p->rip_ro_tlb.mfg_date);
                break;
#endif // RIP_PRIV_MFG_DATE
#endif // CONFIG_CUSTOM_PLUME
            default:
                fprintf(stderr, "Unknown RIP field - ID: %d\n", field_id);
        }
    }
end:
    close(fd);
    free(buf);
    return ret;
}

int rip_init(void)
{
#if 0
    int ret = 0;
    MW_RIP_RO rip_ro;
	UINT8 macNum[MAC_ADDR_LEN];
	devCtl_getBaseMacAddress(macNum);

    ret = dump_rip(&rip_ro);
    if (ret != 1)  
    {
       set_default_rip();
       goto end;
    }

    if(memcmp(rip_ro.rip_ro_tlb.mac_addr_base, macNum, 
                RIP_COMMON_WAN_ADDR_SIZE))
    {
        fprintf(stdout, "RIP: ");
    }

    if (strcpy(rip_ro.rip_ro_tlb.factory, "1") == 0)
        goto end;

end:
#endif
    return 0;
}

const unsigned char wl_srom_map_1108[468] = {
// 4360 2.4G for kaon.
0x01,0x38,
0x00,0x30,
0xf1,0x06,
0xE4,0x14,
0x18,0x02,
0x7E,0x1B,
0x00,0x0A,
0xC4,0x2B,
0x64,0x2A,
0x64,0x29,
0x64,0x2C,
0xE7,0x3C,
0x0A,0x48,
0x3C,0x11,
0x32,0x21,
0x64,0x31,
0x52,0x18,
0x05,0x00,
0x2e,0x1f,
0xf7,0x4d,
0x80,0x80,
0x0b,0x00,
0x30,0x86,
0x90,0x01,
0x00,0x5f,
0xf6,0x41,
0x30,0x86,
0x91,0x01,
0x00,0x83,
0xeb,0x01,
0x01,0x9f,
0xf5,0x65,
0x00,0x82,
0x00,0xd8,
0x10,0x80,
0x7c,0x00,
0x00,0x1f,
0x04,0x00,
0x00,0x80,
0x0c,0x20,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0xA1,0x43,
0x00,0x80,
0x02,0x00,
0x00,0x00,
0xF3,0x3F,
0x00,0x18,
0x00,0x00,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0x34,0x06,
0x03,0x11,
0x00,0x10,
0x00,0x00,
0x02,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x90,0x00,
0x17,0x4C,
0x00,0x10,
0x30,0x58,
0x0F,0x00,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0x07,0x07,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x77,0x00,
0x51,0x30,
0x00,0x00,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x04,0x00,
0x58,0xFF,
0x35,0xFF,
0xF2,0x17,
0x08,0xFD,
0xFF,0xFF,
0xB3,0xBB,
0x48,0x54,
0x50,0x50,
0x5A,0xFF,
0x4B,0x17,
0x19,0xFD,
0x87,0xFF,
0x92,0x1A,
0xCE,0xFC,
0x58,0xFF,
0xE9,0x18,
0xEC,0xFC,
0x57,0xFF,
0xAB,0x19,
0xD6,0xFC,
0x5A,0xFF,
0x3A,0xFF,
0x35,0x18,
0x0A,0xFD,
0xFF,0xFF,
0xB3,0xBB,
0x48,0x54,
0x50,0x50,
0x53,0xFF,
0x26,0x17,
0x15,0xFD,
0x87,0xFF,
0x92,0x1A,
0xCE,0xFC,
0x6D,0xFF,
0x92,0x1A,
0xC3,0xFC,
0x53,0xFF,
0x84,0x19,
0xD4,0xFC,
0x5A,0xFF,
0x2F,0xFF,
0xD0,0x17,
0x06,0xFD,
0xFF,0xFF,
0xB3,0xBB,
0x48,0x54,
0x50,0x50,
0x5C,0xFF,
0x8F,0x17,
0x16,0xFD,
0x8F,0xFF,
0xE7,0x1A,
0xD4,0xFC,
0x5A,0xFF,
0x28,0x19,
0xE9,0xFC,
0x58,0xFF,
0xCF,0x19,
0xD5,0xFC,
0x00,0x00,
0x00,0x00,
0x22,0x42,
0x86,0xCA,
0x22,0x42,
0x86,0xCA,
0x20,0x64,
0x00,0x00,
0x00,0x00,
0x10,0x55,
0x00,0x00,
0x10,0x55,
0x00,0x00,
0x10,0x54,
0x00,0x00,
0x00,0x00,
0x00,0x20,
0x75,0xAA,
0x00,0x00,
0x74,0xBA,
0x00,0x00,
0x74,0xBA,
0x00,0x00,
0x00,0x00,
0x00,0x10,
0x53,0x99,
0x00,0x00,
0x63,0x99,
0x00,0x00,
0x64,0xBA,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0x00,0x00,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0xFF,0xFF,
0x0B,0x00
};

const unsigned char wl_srom_map_4366[1180] = {
// 4366, 5G for kaon.
/*SROM[0  ]*/ 0x01,0x38,
/*SROM[1  ]*/ 0x00,0x30,
/*SROM[2  ]*/ 0x7D,0x07,
/*SROM[3  ]*/ 0xE4,0x14,
/*SROM[4  ]*/ 0x18,0x02,
/*SROM[5  ]*/ 0x7E,0x1B,
/*SROM[6  ]*/ 0x00,0x0A,
/*SROM[7  ]*/ 0xC4,0x2B,
/*SROM[8  ]*/ 0x64,0x2A,
/*SROM[9  ]*/ 0x64,0x29,
/*SROM[10 ]*/ 0x64,0x2C,
/*SROM[11 ]*/ 0xE7,0x3C,
/*SROM[12 ]*/ 0x0A,0x48,
/*SROM[13 ]*/ 0x3C,0x11,
/*SROM[14 ]*/ 0x32,0x21,
/*SROM[15 ]*/ 0x64,0x31,
/*SROM[16 ]*/ 0x52,0x18,
/*SROM[17 ]*/ 0x05,0x00,
/*SROM[18 ]*/ 0x2E,0x1F,
/*SROM[19 ]*/ 0xF7,0x4D,
/*SROM[20 ]*/ 0x80,0x80,
/*SROM[21 ]*/ 0x0B,0x00,
/*SROM[22 ]*/ 0x30,0x86,
/*SROM[23 ]*/ 0x90,0x01,
/*SROM[24 ]*/ 0x00,0x5F,
/*SROM[25 ]*/ 0xF6,0x41,
/*SROM[26 ]*/ 0x30,0x86,
/*SROM[27 ]*/ 0x91,0x01,
/*SROM[28 ]*/ 0x00,0x83,
/*SROM[29 ]*/ 0xEB,0x01,
/*SROM[30 ]*/ 0x01,0x9F,
/*SROM[31 ]*/ 0xF5,0x65,
/*SROM[32 ]*/ 0x00,0x82,
/*SROM[33 ]*/ 0x00,0xD8,
/*SROM[34 ]*/ 0x10,0x80,
/*SROM[35 ]*/ 0x7C,0x00,
/*SROM[36 ]*/ 0x00,0x1F,
/*SROM[37 ]*/ 0x04,0x00,
/*SROM[38 ]*/ 0x00,0x80,
/*SROM[39 ]*/ 0x0C,0x20,
/*SROM[40 ]*/ 0x00,0x00,
/*SROM[41 ]*/ 0x00,0x00,
/*SROM[42 ]*/ 0x00,0x00,
/*SROM[43 ]*/ 0x00,0x00,
/*SROM[44 ]*/ 0x00,0x00,
/*SROM[45 ]*/ 0x00,0x00,
/*SROM[46 ]*/ 0x00,0x00,
/*SROM[47 ]*/ 0x00,0x00,
/*SROM[48 ]*/ 0xC5,0x43,
/*SROM[49 ]*/ 0x00,0x80,
/*SROM[50 ]*/ 0x02,0x00,
/*SROM[51 ]*/ 0x00,0x00,
/*SROM[52 ]*/ 0xF7,0x3F,
/*SROM[53 ]*/ 0x00,0x18,
/*SROM[54 ]*/ 0x05,0x00,
/*SROM[55 ]*/ 0xFF,0xFF,
/*SROM[56 ]*/ 0xFF,0xFF,
/*SROM[57 ]*/ 0xFF,0xFF,
/*SROM[58 ]*/ 0xFF,0xFF,
/*SROM[59 ]*/ 0xFF,0xFF,
/*SROM[60 ]*/ 0xFF,0xFF,
/*SROM[61 ]*/ 0xFF,0xFF,
/*SROM[62 ]*/ 0xFF,0xFF,
/*SROM[63 ]*/ 0xFF,0xFF,
/*SROM[64 ]*/ 0x55,0x4D,
/*SROM[65 ]*/ 0x45,0x11,
/*SROM[66 ]*/ 0x00,0x00,
/*SROM[67 ]*/ 0x00,0x10,
/*SROM[68 ]*/ 0x04,0x00,
/*SROM[69 ]*/ 0x00,0x00,
/*SROM[70 ]*/ 0x00,0x00,
/*SROM[71 ]*/ 0x00,0x00,
/*SROM[72 ]*/ 0x90,0x00,
/*SROM[73 ]*/ 0x1D,0x4C,
/*SROM[74 ]*/ 0x00,0xA0,
/*SROM[75 ]*/ 0x00,0x00,
/*SROM[76 ]*/ 0x00,0x00,
/*SROM[77 ]*/ 0xFF,0xFF,
/*SROM[78 ]*/ 0xFF,0xFF,
/*SROM[79 ]*/ 0xFF,0xFF,
/*SROM[80 ]*/ 0x0F,0x0F,
/*SROM[81 ]*/ 0x02,0x02,
/*SROM[82 ]*/ 0x02,0x02,
/*SROM[83 ]*/ 0x02,0x02,
/*SROM[84 ]*/ 0xFF,0x00,
/*SROM[85 ]*/ 0x01,0x10,
/*SROM[86 ]*/ 0x31,0x00,
/*SROM[87 ]*/ 0xFF,0x78,
/*SROM[88 ]*/ 0xFF,0xFF,
/*SROM[89 ]*/ 0xFF,0xFF,
/*SROM[90 ]*/ 0x40,0x9C,
/*SROM[91 ]*/ 0xFF,0xFF,
/*SROM[92 ]*/ 0x28,0x5A,
/*SROM[93 ]*/ 0x7F,0x7F,
/*SROM[94 ]*/ 0xFF,0xFF,
/*SROM[95 ]*/ 0xFF,0xFF,
/*SROM[96 ]*/ 0x00,0x00,
/*SROM[97 ]*/ 0xAB,0xAB,
/*SROM[98 ]*/ 0x00,0x00,
/*SROM[99 ]*/ 0x00,0x00,
/*SROM[100]*/ 0x02,0x02,
/*SROM[101]*/ 0x00,0x00,
/*SROM[102]*/ 0x00,0x00,
/*SROM[103]*/ 0x00,0x00,
/*SROM[104]*/ 0xFF,0xFF,
/*SROM[105]*/ 0x00,0x00,
/*SROM[106]*/ 0x00,0x00,
/*SROM[107]*/ 0x05,0x00,
/*SROM[108]*/ 0xFF,0xFF,
/*SROM[109]*/ 0x00,0x00,
/*SROM[110]*/ 0x00,0x00,
/*SROM[111]*/ 0x00,0x00,
/*SROM[112]*/ 0x00,0x00,
/*SROM[113]*/ 0x00,0x00,
/*SROM[114]*/ 0x00,0x00,
/*SROM[115]*/ 0x00,0x00,
/*SROM[116]*/ 0x00,0x00,
/*SROM[117]*/ 0x00,0x00,
/*SROM[118]*/ 0x00,0x00,
/*SROM[119]*/ 0x54,0x76,
/*SROM[120]*/ 0x98,0xDB,
/*SROM[121]*/ 0x54,0x76,
/*SROM[122]*/ 0x98,0xDB,
/*SROM[123]*/ 0x54,0x76,
/*SROM[124]*/ 0x98,0xDB,
/*SROM[125]*/ 0x54,0x76,
/*SROM[126]*/ 0x98,0xDB,
/*SROM[127]*/ 0x54,0x76,
/*SROM[128]*/ 0x98,0xDB,
/*SROM[129]*/ 0xFF,0xFF,
/*SROM[130]*/ 0xFF,0xFF,
/*SROM[131]*/ 0xFF,0xFF,
/*SROM[132]*/ 0xFF,0xFF,
/*SROM[133]*/ 0xFF,0xFF,
/*SROM[134]*/ 0xFF,0xFF,
/*SROM[135]*/ 0x00,0x00,
/*SROM[136]*/ 0x00,0x00,
/*SROM[137]*/ 0x00,0x00,
/*SROM[138]*/ 0x00,0x00,
/*SROM[139]*/ 0xFE,0xFE,
/*SROM[140]*/ 0xFE,0xFE,
/*SROM[141]*/ 0xFE,0xFE,
/*SROM[142]*/ 0xFE,0xFE,
/*SROM[143]*/ 0xFF,0xFF,
/*SROM[144]*/ 0xFF,0xFF,
/*SROM[145]*/ 0xFF,0xFF,
/*SROM[146]*/ 0xFF,0xFF,
/*SROM[147]*/ 0xFF,0xFF,
/*SROM[148]*/ 0xFF,0xFF,
/*SROM[149]*/ 0xFF,0xFF,
/*SROM[150]*/ 0xFF,0xFF,
/*SROM[151]*/ 0xFF,0xFF,
/*SROM[152]*/ 0xFF,0xFF,
/*SROM[153]*/ 0xFF,0xFF,
/*SROM[154]*/ 0xFF,0xFF,
/*SROM[155]*/ 0xFF,0xFF,
/*SROM[156]*/ 0xFF,0xFF,
/*SROM[157]*/ 0xFF,0xFF,
/*SROM[158]*/ 0xFF,0xFF,
/*SROM[159]*/ 0xFF,0xFF,
/*SROM[160]*/ 0xFF,0xFF,
/*SROM[161]*/ 0xFF,0xFF,
/*SROM[162]*/ 0xFF,0xFF,
/*SROM[163]*/ 0xFF,0xFF,
/*SROM[164]*/ 0xFF,0xFF,
/*SROM[165]*/ 0xFF,0xFF,
/*SROM[166]*/ 0xFF,0xFF,
/*SROM[167]*/ 0xFF,0xFF,
/*SROM[168]*/ 0x00,0x00,
/*SROM[169]*/ 0x00,0x00,
/*SROM[170]*/ 0x22,0x72,
/*SROM[171]*/ 0x77,0x99,
/*SROM[172]*/ 0x22,0x72,
/*SROM[173]*/ 0x77,0x99,
/*SROM[174]*/ 0x22,0x77,
/*SROM[175]*/ 0x22,0x00,
/*SROM[176]*/ 0x22,0x43,
/*SROM[177]*/ 0x65,0x87,
/*SROM[178]*/ 0x22,0x43,
/*SROM[179]*/ 0x55,0x76,
/*SROM[180]*/ 0x22,0x43,
/*SROM[181]*/ 0x65,0x87,
/*SROM[182]*/ 0x00,0x00,
/*SROM[183]*/ 0x00,0x00,
/*SROM[184]*/ 0x22,0x43,
/*SROM[185]*/ 0x55,0x66,
/*SROM[186]*/ 0x22,0x43,
/*SROM[187]*/ 0x55,0x66,
/*SROM[188]*/ 0x22,0x43,
/*SROM[189]*/ 0x55,0x66,
/*SROM[190]*/ 0x00,0x00,
/*SROM[191]*/ 0x00,0x00,
/*SROM[192]*/ 0x22,0x43,
/*SROM[193]*/ 0x55,0x66,
/*SROM[194]*/ 0x22,0x43,
/*SROM[195]*/ 0x55,0x66,
/*SROM[196]*/ 0x22,0x43,
/*SROM[197]*/ 0x55,0x66,
/*SROM[198]*/ 0x00,0x00,
/*SROM[199]*/ 0x00,0x00,
/*SROM[200]*/ 0x00,0x00,
/*SROM[201]*/ 0x00,0x00,
/*SROM[202]*/ 0x00,0x00,
/*SROM[203]*/ 0x00,0x00,
/*SROM[204]*/ 0x00,0x00,
/*SROM[205]*/ 0x00,0x00,
/*SROM[206]*/ 0x00,0x00,
/*SROM[207]*/ 0x00,0x00,
/*SROM[208]*/ 0x00,0x00,
/*SROM[209]*/ 0x00,0x00,
/*SROM[210]*/ 0x00,0x00,
/*SROM[211]*/ 0x00,0x00,
/*SROM[212]*/ 0x00,0x00,
/*SROM[213]*/ 0x00,0x00,
/*SROM[214]*/ 0x00,0x00,
/*SROM[215]*/ 0x00,0x00,
/*SROM[216]*/ 0x00,0x00,
/*SROM[217]*/ 0x00,0x00,
/*SROM[218]*/ 0x00,0x00,
/*SROM[219]*/ 0x00,0x00,
/*SROM[220]*/ 0xFF,0xFF,
/*SROM[221]*/ 0x12,0x0F,
/*SROM[222]*/ 0xFF,0xFF,
/*SROM[223]*/ 0xFF,0xFF,
/*SROM[224]*/ 0xFF,0xFF,
/*SROM[225]*/ 0xFF,0xFF,
/*SROM[226]*/ 0xFF,0xFF,
/*SROM[227]*/ 0xFF,0xFF,
/*SROM[228]*/ 0x04,0x00,
/*SROM[229]*/ 0x04,0x00,
/*SROM[230]*/ 0x08,0x00,
/*SROM[231]*/ 0x08,0x00,
/*SROM[232]*/ 0xFF,0xFF,
/*SROM[233]*/ 0xFF,0xFF,
/*SROM[234]*/ 0x22,0x43,
/*SROM[235]*/ 0x55,0x66,
/*SROM[236]*/ 0x22,0x43,
/*SROM[237]*/ 0x55,0x66,
/*SROM[238]*/ 0x22,0x43,
/*SROM[239]*/ 0x55,0x66,
/*SROM[240]*/ 0x00,0x00,
/*SROM[241]*/ 0x00,0x00,
/*SROM[242]*/ 0x00,0x00,
/*SROM[243]*/ 0x00,0x00,
/*SROM[244]*/ 0x00,0x00,
/*SROM[245]*/ 0x22,0x43,
/*SROM[246]*/ 0x55,0x66,
/*SROM[247]*/ 0x22,0x43,
/*SROM[248]*/ 0x55,0x66,
/*SROM[249]*/ 0x22,0x43,
/*SROM[250]*/ 0x55,0x66,
/*SROM[251]*/ 0x00,0x00,
/*SROM[252]*/ 0x00,0x00,
/*SROM[253]*/ 0x00,0x00,
/*SROM[254]*/ 0x00,0x00,
/*SROM[255]*/ 0x00,0x00,
/*SROM[256]*/ 0x5A,0x5A,
/*SROM[257]*/ 0xFF,0xFF,
/*SROM[258]*/ 0xFF,0xFF,
/*SROM[259]*/ 0xFF,0xFF,
/*SROM[260]*/ 0xFF,0xFF,
/*SROM[261]*/ 0xAB,0xAB,
/*SROM[262]*/ 0x5A,0x5A,
/*SROM[263]*/ 0x5A,0x5A,
/*SROM[264]*/ 0x27,0x19,
/*SROM[265]*/ 0x87,0xD0,
/*SROM[266]*/ 0xE7,0x12,
/*SROM[267]*/ 0xF6,0x21,
/*SROM[268]*/ 0xF1,0x18,
/*SROM[269]*/ 0xBB,0xD1,
/*SROM[270]*/ 0x0B,0x15,
/*SROM[271]*/ 0x8E,0x22,
/*SROM[272]*/ 0xF5,0x18,
/*SROM[273]*/ 0x82,0xD2,
/*SROM[274]*/ 0xA6,0x14,
/*SROM[275]*/ 0x5C,0x22,
/*SROM[276]*/ 0xF8,0x18,
/*SROM[277]*/ 0xE7,0xD4,
/*SROM[278]*/ 0x5F,0x18,
/*SROM[279]*/ 0x12,0x23,
/*SROM[280]*/ 0xF1,0x18,
/*SROM[281]*/ 0x65,0xD7,
/*SROM[282]*/ 0x19,0x1C,
/*SROM[283]*/ 0xC4,0x23,
/*SROM[284]*/ 0xFF,0xFF,
/*SROM[285]*/ 0xFF,0xFF,
/*SROM[286]*/ 0xFF,0xFF,
/*SROM[287]*/ 0xFF,0xFF,
/*SROM[288]*/ 0xBA,0x18,
/*SROM[289]*/ 0xE1,0xDB,
/*SROM[290]*/ 0x7B,0x20,
/*SROM[291]*/ 0xE3,0x23,
/*SROM[292]*/ 0xAE,0x18,
/*SROM[293]*/ 0x30,0xDD,
/*SROM[294]*/ 0xE3,0x22,
/*SROM[295]*/ 0x44,0x24,
/*SROM[296]*/ 0x6E,0x18,
/*SROM[297]*/ 0x61,0xE0,
/*SROM[298]*/ 0xB6,0x26,
/*SROM[299]*/ 0x9E,0x24,
/*SROM[300]*/ 0x80,0x18,
/*SROM[301]*/ 0x0F,0xE3,
/*SROM[302]*/ 0xFB,0x2B,
/*SROM[303]*/ 0x47,0x25,
/*SROM[304]*/ 0x57,0x18,
/*SROM[305]*/ 0x55,0xE6,
/*SROM[306]*/ 0x0D,0x30,
/*SROM[307]*/ 0xA1,0x25,
/*SROM[308]*/ 0xAE,0x19,
/*SROM[309]*/ 0x55,0xDE,
/*SROM[310]*/ 0x33,0x24,
/*SROM[311]*/ 0x37,0x24,
/*SROM[312]*/ 0x85,0x19,
/*SROM[313]*/ 0xB4,0xDD,
/*SROM[314]*/ 0x09,0x23,
/*SROM[315]*/ 0x0E,0x24,
/*SROM[316]*/ 0x6E,0x19,
/*SROM[317]*/ 0x6F,0xE0,
/*SROM[318]*/ 0x00,0x26,
/*SROM[319]*/ 0x6A,0x24,
/*SROM[320]*/ 0x16,0x19,
/*SROM[321]*/ 0x2E,0xE3,
/*SROM[322]*/ 0x2A,0x28,
/*SROM[323]*/ 0x97,0x24,
/*SROM[324]*/ 0x1B,0x19,
/*SROM[325]*/ 0xDD,0xE4,
/*SROM[326]*/ 0x2C,0x2A,
/*SROM[327]*/ 0xBA,0x24,
/*SROM[328]*/ 0x5A,0x5A,
/*SROM[329]*/ 0xFF,0xFF,
/*SROM[330]*/ 0xFF,0xFF,
/*SROM[331]*/ 0xFF,0xFF,
/*SROM[332]*/ 0xFF,0xFF,
/*SROM[333]*/ 0xAB,0xAB,
/*SROM[334]*/ 0x5A,0x5A,
/*SROM[335]*/ 0x5A,0x5A,
/*SROM[336]*/ 0xF6,0x18,
/*SROM[337]*/ 0x42,0xCF,
/*SROM[338]*/ 0x2C,0x10,
/*SROM[339]*/ 0x74,0x21,
/*SROM[340]*/ 0xFE,0x18,
/*SROM[341]*/ 0xDD,0xCE,
/*SROM[342]*/ 0x08,0x0F,
/*SROM[343]*/ 0x48,0x21,
/*SROM[344]*/ 0xFE,0x18,
/*SROM[345]*/ 0x30,0xD1,
/*SROM[346]*/ 0x98,0x12,
/*SROM[347]*/ 0x1E,0x22,
/*SROM[348]*/ 0xE8,0x18,
/*SROM[349]*/ 0x03,0xD3,
/*SROM[350]*/ 0xAE,0x15,
/*SROM[351]*/ 0xB9,0x22,
/*SROM[352]*/ 0x0F,0x19,
/*SROM[353]*/ 0x67,0xD4,
/*SROM[354]*/ 0x92,0x17,
/*SROM[355]*/ 0x20,0x23,
/*SROM[356]*/ 0xFF,0xFF,
/*SROM[357]*/ 0xFF,0xFF,
/*SROM[358]*/ 0xFF,0xFF,
/*SROM[359]*/ 0xFF,0xFF,
/*SROM[360]*/ 0xB8,0x18,
/*SROM[361]*/ 0x11,0xDA,
/*SROM[362]*/ 0xF3,0x1D,
/*SROM[363]*/ 0xB4,0x23,
/*SROM[364]*/ 0xC2,0x18,
/*SROM[365]*/ 0xD6,0xD9,
/*SROM[366]*/ 0xAB,0x1C,
/*SROM[367]*/ 0x70,0x23,
/*SROM[368]*/ 0xA1,0x18,
/*SROM[369]*/ 0xA0,0xDB,
/*SROM[370]*/ 0x16,0x1F,
/*SROM[371]*/ 0xD3,0x23,
/*SROM[372]*/ 0xA3,0x18,
/*SROM[373]*/ 0x94,0xDE,
/*SROM[374]*/ 0x2C,0x24,
/*SROM[375]*/ 0x7A,0x24,
/*SROM[376]*/ 0xA4,0x18,
/*SROM[377]*/ 0x56,0xDF,
/*SROM[378]*/ 0xAE,0x23,
/*SROM[379]*/ 0x48,0x24,
/*SROM[380]*/ 0x99,0x19,
/*SROM[381]*/ 0xB8,0xDA,
/*SROM[382]*/ 0x28,0x1E,
/*SROM[383]*/ 0x8D,0x23,
/*SROM[384]*/ 0x9F,0x19,
/*SROM[385]*/ 0x89,0xDA,
/*SROM[386]*/ 0xEF,0x1D,
/*SROM[387]*/ 0x88,0x23,
/*SROM[388]*/ 0x6D,0x19,
/*SROM[389]*/ 0x63,0xDD,
/*SROM[390]*/ 0x68,0x22,
/*SROM[391]*/ 0x24,0x24,
/*SROM[392]*/ 0x4D,0x19,
/*SROM[393]*/ 0x6E,0xDE,
/*SROM[394]*/ 0x36,0x22,
/*SROM[395]*/ 0x1B,0x24,
/*SROM[396]*/ 0x41,0x19,
/*SROM[397]*/ 0xB8,0xE1,
/*SROM[398]*/ 0xC0,0x27,
/*SROM[399]*/ 0xBA,0x24,
/*SROM[400]*/ 0x5A,0x5A,
/*SROM[401]*/ 0xFF,0xFF,
/*SROM[402]*/ 0xFF,0xFF,
/*SROM[403]*/ 0xFF,0xFF,
/*SROM[404]*/ 0xFF,0xFF,
/*SROM[405]*/ 0xAB,0xAB,
/*SROM[406]*/ 0x5A,0x5A,
/*SROM[407]*/ 0x5A,0x5A,
/*SROM[408]*/ 0x12,0x19,
/*SROM[409]*/ 0xE6,0xCF,
/*SROM[410]*/ 0x28,0x11,
/*SROM[411]*/ 0x9C,0x21,
/*SROM[412]*/ 0x31,0x19,
/*SROM[413]*/ 0x0D,0xD0,
/*SROM[414]*/ 0x17,0x11,
/*SROM[415]*/ 0xBA,0x21,
/*SROM[416]*/ 0x30,0x19,
/*SROM[417]*/ 0xAC,0xD2,
/*SROM[418]*/ 0xFD,0x15,
/*SROM[419]*/ 0xD3,0x22,
/*SROM[420]*/ 0x3C,0x19,
/*SROM[421]*/ 0x52,0xD4,
/*SROM[422]*/ 0xEF,0x17,
/*SROM[423]*/ 0x13,0x23,
/*SROM[424]*/ 0x35,0x19,
/*SROM[425]*/ 0x42,0xD5,
/*SROM[426]*/ 0x9B,0x18,
/*SROM[427]*/ 0x2B,0x23,
/*SROM[428]*/ 0xFF,0xFF,
/*SROM[429]*/ 0xFF,0xFF,
/*SROM[430]*/ 0xFF,0xFF,
/*SROM[431]*/ 0xFF,0xFF,
/*SROM[432]*/ 0xDC,0x18,
/*SROM[433]*/ 0x39,0xDB,
/*SROM[434]*/ 0x8C,0x1F,
/*SROM[435]*/ 0xD9,0x23,
/*SROM[436]*/ 0xE9,0x18,
/*SROM[437]*/ 0xB1,0xDA,
/*SROM[438]*/ 0x19,0x1E,
/*SROM[439]*/ 0xA7,0x23,
/*SROM[440]*/ 0xCF,0x18,
/*SROM[441]*/ 0x26,0xDE,
/*SROM[442]*/ 0x11,0x24,
/*SROM[443]*/ 0x71,0x24,
/*SROM[444]*/ 0xC2,0x18,
/*SROM[445]*/ 0x57,0xE0,
/*SROM[446]*/ 0x38,0x27,
/*SROM[447]*/ 0xCB,0x24,
/*SROM[448]*/ 0xB7,0x18,
/*SROM[449]*/ 0xE5,0xE2,
/*SROM[450]*/ 0x6C,0x2A,
/*SROM[451]*/ 0x1E,0x25,
/*SROM[452]*/ 0xDA,0x19,
/*SROM[453]*/ 0x6F,0xDC,
/*SROM[454]*/ 0xE9,0x21,
/*SROM[455]*/ 0x01,0x24,
/*SROM[456]*/ 0xC8,0x19,
/*SROM[457]*/ 0xFC,0xDB,
/*SROM[458]*/ 0xA0,0x20,
/*SROM[459]*/ 0xE0,0x23,
/*SROM[460]*/ 0xB4,0x19,
/*SROM[461]*/ 0x91,0xDE,
/*SROM[462]*/ 0x17,0x24,
/*SROM[463]*/ 0x4B,0x24,
/*SROM[464]*/ 0x95,0x19,
/*SROM[465]*/ 0x26,0xE1,
/*SROM[466]*/ 0xD9,0x26,
/*SROM[467]*/ 0x9C,0x24,
/*SROM[468]*/ 0x7C,0x19,
/*SROM[469]*/ 0x81,0xE2,
/*SROM[470]*/ 0x05,0x28,
/*SROM[471]*/ 0xA2,0x24,
/*SROM[472]*/ 0xFF,0xFF,
/*SROM[473]*/ 0x00,0x00,
/*SROM[474]*/ 0x00,0x00,
/*SROM[475]*/ 0x00,0x00,
/*SROM[476]*/ 0x00,0x00,
/*SROM[477]*/ 0x00,0x00,
/*SROM[478]*/ 0x00,0x00,
/*SROM[479]*/ 0x00,0x00,
/*SROM[480]*/ 0x00,0x00,
/*SROM[481]*/ 0x00,0x00,
/*SROM[482]*/ 0x00,0x00,
/*SROM[483]*/ 0xAB,0xAB,
/*SROM[484]*/ 0xAB,0xAB,
/*SROM[485]*/ 0xAB,0xAB,
/*SROM[486]*/ 0x0E,0x00,
/*SROM[487]*/ 0x00,0x00,
/*SROM[488]*/ 0x00,0x00,
/*SROM[489]*/ 0x00,0x00,
/*SROM[490]*/ 0x00,0x00,
/*SROM[491]*/ 0x00,0x00,
/*SROM[492]*/ 0x00,0x00,
/*SROM[493]*/ 0x01,0x00,
/*SROM[494]*/ 0x44,0x44,
/*SROM[495]*/ 0x33,0x33,
/*SROM[496]*/ 0x22,0x22,
/*SROM[497]*/ 0x00,0x00,
/*SROM[498]*/ 0x44,0x44,
/*SROM[499]*/ 0x33,0x33,
/*SROM[500]*/ 0x22,0x22,
/*SROM[501]*/ 0x00,0x00,
/*SROM[502]*/ 0x00,0x00,
/*SROM[503]*/ 0x00,0x00,
/*SROM[504]*/ 0x00,0x00,
/*SROM[505]*/ 0x00,0x00,
/*SROM[506]*/ 0x00,0x00,
/*SROM[507]*/ 0x00,0x00,
/*SROM[508]*/ 0x00,0x00,
/*SROM[509]*/ 0x00,0x00,
/*SROM[510]*/ 0x00,0x00,
/*SROM[511]*/ 0x00,0x00,
/*SROM[512]*/ 0x5A,0x5A,
/*SROM[513]*/ 0xFF,0xFF,
/*SROM[514]*/ 0xFF,0xFF,
/*SROM[515]*/ 0xFF,0xFF,
/*SROM[516]*/ 0xFF,0xFF,
/*SROM[517]*/ 0xAB,0xAB,
/*SROM[518]*/ 0x5A,0x5A,
/*SROM[519]*/ 0x5A,0x5A,
/*SROM[520]*/ 0xD4,0x18,
/*SROM[521]*/ 0xF7,0xCE,
/*SROM[522]*/ 0x91,0x0F,
/*SROM[523]*/ 0x63,0x21,
/*SROM[524]*/ 0xDA,0x18,
/*SROM[525]*/ 0xF1,0xCF,
/*SROM[526]*/ 0xD7,0x10,
/*SROM[527]*/ 0xB5,0x21,
/*SROM[528]*/ 0xD8,0x18,
/*SROM[529]*/ 0x40,0xD2,
/*SROM[530]*/ 0x3A,0x14,
/*SROM[531]*/ 0x76,0x22,
/*SROM[532]*/ 0xFE,0x18,
/*SROM[533]*/ 0x84,0xD4,
/*SROM[534]*/ 0x91,0x17,
/*SROM[535]*/ 0x0B,0x23,
/*SROM[536]*/ 0xE0,0x18,
/*SROM[537]*/ 0x71,0xD6,
/*SROM[538]*/ 0x48,0x1A,
/*SROM[539]*/ 0x91,0x23,
/*SROM[540]*/ 0xFF,0xFF,
/*SROM[541]*/ 0xFF,0xFF,
/*SROM[542]*/ 0xFF,0xFF,
/*SROM[543]*/ 0xFF,0xFF,
/*SROM[544]*/ 0x61,0x18,
/*SROM[545]*/ 0x15,0xDB,
/*SROM[546]*/ 0xE5,0x1E,
/*SROM[547]*/ 0xC9,0x23,
/*SROM[548]*/ 0x60,0x18,
/*SROM[549]*/ 0x33,0xDC,
/*SROM[550]*/ 0x9B,0x20,
/*SROM[551]*/ 0x0B,0x24,
/*SROM[552]*/ 0x56,0x18,
/*SROM[553]*/ 0xD2,0xDE,
/*SROM[554]*/ 0x57,0x23,
/*SROM[555]*/ 0x57,0x24,
/*SROM[556]*/ 0x7C,0x18,
/*SROM[557]*/ 0x2F,0xE2,
/*SROM[558]*/ 0x47,0x29,
/*SROM[559]*/ 0xF9,0x24,
/*SROM[560]*/ 0x3A,0x18,
/*SROM[561]*/ 0xFC,0xE5,
/*SROM[562]*/ 0x63,0x2F,
/*SROM[563]*/ 0xAE,0x25,
/*SROM[564]*/ 0x9B,0x19,
/*SROM[565]*/ 0x7C,0xDC,
/*SROM[566]*/ 0x3E,0x21,
/*SROM[567]*/ 0x02,0x24,
/*SROM[568]*/ 0x59,0x19,
/*SROM[569]*/ 0x30,0xDE,
/*SROM[570]*/ 0xDB,0x23,
/*SROM[571]*/ 0x50,0x24,
/*SROM[572]*/ 0x65,0x19,
/*SROM[573]*/ 0xF0,0xE0,
/*SROM[574]*/ 0x0A,0x27,
/*SROM[575]*/ 0xAD,0x24,
/*SROM[576]*/ 0x11,0x19,
/*SROM[577]*/ 0x38,0xE4,
/*SROM[578]*/ 0x6E,0x2B,
/*SROM[579]*/ 0x1C,0x25,
/*SROM[580]*/ 0xFD,0x18,
/*SROM[581]*/ 0x19,0xE6,
/*SROM[582]*/ 0x05,0x2D,
/*SROM[583]*/ 0x29,0x25,
/*SROM[584]*/ 0xFF,0xFF,
/*SROM[585]*/ 0xFF,0xFF,
/*SROM[586]*/ 0xFF,0xFF,
/*SROM[587]*/ 0xFF,0xFF,
/*SROM[588]*/ 0xFF,0xFF,
/*SROM[589]*/ 0x0D,0x00
};

/*
 * from int ssk(void) funtion
 * 0: Continue next (Nomal)
 * 1: Need a reboot (with restoredefault)
 */
#define CONFIG_MODEL_AR1344 1
#define SUPPORT_SSID_FORMAT "\"%s-%02x%02x\""
#define SUPPORT_KAON_CUSTOM_SSID_PREFIX	"KaonAP"
#define KAON_CUSTOM_MANUFACTURE	"Kaonmedia Co., Ltd."
int rip_to_nvram(void)
{
	int ret = 0;

	MW_RIP_RO rip_ro;
	UINT8 macNum[MAC_ADDR_LEN];
	char macStr[32]={0,};
	char snStr[32]={0,};
	char ssidStr[64]={0,};
	devCtl_getBaseMacAddress(macNum);

	snprintf(macStr, 32, "%02x:%02x:%02x:%02x:%02x:%02x",
		macNum[0],macNum[1],
		macNum[2],macNum[3],
		macNum[4],macNum[5]);

	ret = dump_rip(&rip_ro);
	if (ret <= 0 || strcmp(rip_ro.rip_ro_tlb.factory, "9") == 0 )
		goto write_default;

//	if(memcmp(macNum, rip_ro.rip_ro_tlb.mac_addr_base, MAC_ADDR_LEN))
//	{
		char cmdStr[128];
		uint8_t hwaddr[6];
		uint64_t intHwaddr;

		/*
		* by hogi
		* Change the CFE nvram area to be writable.
		*/
		devCtl_boardIoctl(BOARD_IOCTL_USER_FLASH_WR, 0, NULL, 0, 0, NULL);
		system("/usr/sbin/envrams");
		snprintf(macStr, 32, "%02x:%02x:%02x:%02x:%02x:%02x",
		rip_ro.rip_ro_tlb.mac_addr_base[0],
		rip_ro.rip_ro_tlb.mac_addr_base[1],
		rip_ro.rip_ro_tlb.mac_addr_base[2],
		rip_ro.rip_ro_tlb.mac_addr_base[3],
		rip_ro.rip_ro_tlb.mac_addr_base[4],
		rip_ro.rip_ro_tlb.mac_addr_base[5]);
		snprintf(cmdStr, 128, "echo %s > /proc/nvram/BaseMacAddr", macStr);
		system(cmdStr);

		/*
		* AR1344
		* et0macaddr=90:f8:91:f0:bf:ae     //br0
		* sb/0/macaddr=90:f8:91:f0:bf:af   //wl1
		* 1:macaddr=90:f8:91:f0:bf:b0      //wl0
		*/

		/* set base address */
		snprintf(cmdStr, 128, "/usr/sbin/envram set et0macaddr=%s", macStr);
		system(cmdStr);

		/* set wl0 mac address */
		intHwaddr = mac2int(rip_ro.rip_ro_tlb.mac_addr_base);
		int2mac(intHwaddr+2, hwaddr);
		ether_etoa((unsigned char *)hwaddr, macStr);
		snprintf(cmdStr, 128, "/usr/sbin/envram set sb/0/macaddr=%s", macStr);
		system(cmdStr);

		/* set wl1 mac address */
		int2mac(intHwaddr+3, hwaddr);
		ether_etoa((unsigned char *)hwaddr, macStr);

        /* set AWLAN_Node info */

		snprintf(cmdStr, 128, "/usr/sbin/envram set board_id=%02X%02X%02X%02X%02X%02X",
		rip_ro.rip_ro_tlb.mac_addr_base[0],
		rip_ro.rip_ro_tlb.mac_addr_base[1],
		rip_ro.rip_ro_tlb.mac_addr_base[2],
		rip_ro.rip_ro_tlb.mac_addr_base[3],
		rip_ro.rip_ro_tlb.mac_addr_base[4],
		rip_ro.rip_ro_tlb.mac_addr_base[5]);
        
		system(cmdStr);

        snprintf(cmdStr, 128, "/usr/sbin/envram set board_serialnumber=%02X%02X%02X%02X%02X%02X",
		rip_ro.rip_ro_tlb.mac_addr_base[0],
		rip_ro.rip_ro_tlb.mac_addr_base[1],
		rip_ro.rip_ro_tlb.mac_addr_base[2],
		rip_ro.rip_ro_tlb.mac_addr_base[3],
		rip_ro.rip_ro_tlb.mac_addr_base[4],
		rip_ro.rip_ro_tlb.mac_addr_base[5]);
        
		system(cmdStr);

        snprintf(cmdStr, 128, "/usr/sbin/envram set board_revision=%s", "Rev1.0");
		system(cmdStr);

        snprintf(cmdStr, 128, "/usr/sbin/envram set vendor_factory=%s", "KaonBroadband");
		system(cmdStr);

        snprintf(cmdStr, 128, "/usr/sbin/envram set vendor_manufacturer=%s", "KaonBroadband");
		system(cmdStr);

        snprintf(cmdStr, 128, "/usr/sbin/envram set vendor_mfg_date=%s", rip_ro.rip_ro_tlb.mfg_date);
		system(cmdStr);

        snprintf(cmdStr, 128, "/usr/sbin/envram set vendor_name=%s", "KaonBroadband");
		system(cmdStr);

        snprintf(cmdStr, 128, "/usr/sbin/envram set vendor_part_number=%s", "kaonAR1344E");
		system(cmdStr);

        snprintf(cmdStr, 128, "/usr/sbin/envram set sku_number=%s", "AR1344E");
		system(cmdStr);

        snprintf(cmdStr, 128, "/usr/sbin/envram set target_model=%s", "AR1344E");
		system(cmdStr);

        snprintf(cmdStr, 128, "/usr/sbin/envram set platform_version=%s", "AR1344E_osync");
		system(cmdStr);

		system("/usr/sbin/envram commit");
		unlink("/data/.kernel_nvram.setting");

		rip_to_certi_files();

		sleep(1);
		system("/bin/sync");
		return 1;
//	}


	return 0;

write_default:
	/*
	* ret: 0 Rip is empty
	* ret: -1 Checksum Nok
	*/
	fprintf(stdout, "RIP: Initialized the RIP with default values\n");
	set_default_rip();
	snprintf(snStr, 32, "BS10065102%02X%02X%02X",macNum[3],macNum[4],macNum[5]);
	snprintf(ssidStr, 64, SUPPORT_SSID_FORMAT, SUPPORT_KAON_CUSTOM_SSID_PREFIX, macNum[4],macNum[5]);

	write_rip_field("WAN_ADDR", macStr);
	write_rip_field("SERIAL_NUMBER", snStr);
	write_rip_field("MODELNAME", "AR1344P");
	write_rip_field("MANUFACTURER", KAON_CUSTOM_MANUFACTURE);
	write_rip_field("FACTORY", "0");
	write_rip_field("WLAN_KEY", "1234567890");
	write_rip_field("WLAN_SSID", ssidStr);

	return 1;
}

int decrypt_certi_file(void)
{
	int ret = 0;
	MW_RIP_RO rip_ro;
	char key_buff[128] = {0,};
    char sn_buff[32] = {0,};
    char mac_buff[8] = {0,};
    char seed_buff[32] = {0,};
	/* Dump RIP */
	dump_rip(&rip_ro);

    memcpy(sn_buff,rip_ro.rip_ro_tlb.serial_number,RIP_COMMON_SERIAL_NUMBER_SIZE);
    memcpy(mac_buff,rip_ro.rip_ro_tlb.mac_addr_base,RIP_COMMON_WAN_ADDR_SIZE);
    memcpy(seed_buff,rip_ro.rip_ro_tlb.seed_key,RIP_PRIV_SEED_SIZE);
    
    memcpy(key_buff,key_generation(sn_buff,mac_buff,seed_buff),64);
    
	if(aes_decrypt_file_key("/data/cer_ca.pem", "/var/certs/ca.pem",key_buff) != 0)
	{
		ret = -1;
	}
	if(aes_decrypt_file_key("/data/cer_client.pem", "/var/certs/client.pem",key_buff) != 0)
	{
		ret = -1;
	}
	if(aes_decrypt_file_key("/data/cer_client_dec.key", "/var/certs/client_dec.key",key_buff) != 0)
	{
		ret = 1;
	}

	return ret;
}

int change_ssh_pw(void)
{
	int ret = 0;
	MW_RIP_RO rip_ro;
	char time_buff[255] = {0,};
	char pw_buff[128] = {0,};
	char pw_cmd[1024] = {0,};
	char macStr[32]={0,};
	time_t the_time;
	long min_buff = 0;
	

	/* Dump RIP */
	dump_rip(&rip_ro);

	time(&the_time);

	min_buff = the_time / 60;

	printf("time_buff : %ld\n",min_buff);

	sprintf(time_buff,"%ld", min_buff);

	snprintf(macStr, 32, "%02x:%02x:%02x:%02x:%02x:%02x",
                rip_ro.rip_ro_tlb.mac_addr_base[0],
                rip_ro.rip_ro_tlb.mac_addr_base[1],
                rip_ro.rip_ro_tlb.mac_addr_base[2],
                rip_ro.rip_ro_tlb.mac_addr_base[3],
                rip_ro.rip_ro_tlb.mac_addr_base[4],
                rip_ro.rip_ro_tlb.mac_addr_base[5]);

	printf("sn:%s , mac:%s \n",rip_ro.rip_ro_tlb.serial_number,macStr);

	memcpy(pw_buff,pw_generation(rip_ro.rip_ro_tlb.serial_number,macStr,rip_ro.rip_ro_tlb.model_name,time_buff),64);

	sprintf(pw_cmd,"/bin/echo 'osync:%s' | chpasswd",pw_buff);

	printf("%s\n",pw_cmd);

	system(pw_cmd);

	return ret;
}

int change_ssh_pw2(void)
{
	int ret = 0;
	MW_RIP_RO rip_ro;
	char time_buff[255] = {0,};
	char pw_buff[128] = {0,};
	char pw_cmd[1024] = {0,};
	time_t the_time;
	long min_buff = 0;
	

	/* Dump RIP */
	dump_rip(&rip_ro);

	time(&the_time);

	min_buff = the_time / 60;

	printf("time_buff : %ld\n",min_buff);

	sprintf(time_buff,"%ld", min_buff);

	memcpy(pw_buff,pw_generation2(rip_ro.rip_ro_tlb.seed_key,time_buff),64);

	sprintf(pw_cmd,"/bin/echo 'kaon_user:%s' | chpasswd",pw_buff);

	printf("%s\n",pw_cmd);

	system(pw_cmd);

	return ret;
}

int encrypt_certi_file(char *path)
{
	int i, res=0;
    FILE *fd;
    MW_RIP_RO rip_ro;
#if defined(CONFIG_CUSTOM_PLUME)
    unsigned char buf[1024*7];
#else
    unsigned char buf[2048];
#endif
    char fullPath[1024]={0};
	char encPath[1024]={0};
    char sn_buff[32] = {0,};
    char mac_buff[8] = {0,};
    char seed_buff[32] = {0,};
    char key_buff[128] = {0,};

	for (i=0 ; i<(sizeof(rip) / sizeof(rip[0])) ; ++i)
    {
		if( (rip[i].id == RIP_COMMON_SERIAL_NUMBER) || (rip[i].id == RIP_COMMON_WAN_ADDR) || (rip[i].id == RIP_COMMON_SEED) )
		{
			sprintf(fullPath, "%s%s", path, rip[i].filename);
			if(path && rip[i].filename && (fd = fopen(fullPath, "r")))
			{
				size_t read_bytes;
				int length;

				/* Handle the special case of the MAC address in file with ':' */
				length = (rip[i].flags == FLAG_MACADDRESS) ? rip[i].length*2+5 : rip[i].length;

				if (0 == (read_bytes=fread(buf, 1, length, fd)))
				{
					fprintf(stderr, "ERROR: Failed reading file %s\n", rip[i].filename);
					fclose(fd);
					return -1;
				}
				if(length > read_bytes) /* Set to 0 the remaining unused bytes */
					memset(&buf[read_bytes], 0, length-read_bytes);

				/* Remove trailing newlines that could have been left at the end of the text files */
				if(rip[i].flags & FLAG_STRING)
				{
					int j;
					for(j=read_bytes-1 ; j >= 0 && (buf[j] == '\r' || buf[j] == '\n') ; --j)
					{
						buf[j] = '\0';
					}
				}
				res = fill_rip_struct(&rip_ro, buf, i);
				fclose(fd);
			}
			else /* Use the default value */
			{
				res = fill_rip_struct(&rip_ro, NULL, i);
			}
			if(res)
			{
				fprintf(stderr, "ERROR: Filling RIP struct\n");
				return -1;
			}
		}
	}
	
	for (i=0 ; i<(sizeof(rip) / sizeof(rip[0])) ; ++i)
    {
        if(path)
        {
			switch(rip[i].id)
			{
                case RIP_COMMON_CA_PEM_KEY:				
                case RIP_COMMON_CLIENT_PEM_KEY:
                case RIP_COMMON_DEC_KEY:					
                    memcpy(sn_buff,rip_ro.rip_ro_tlb.serial_number,RIP_COMMON_SERIAL_NUMBER_SIZE);
                    memcpy(mac_buff,rip_ro.rip_ro_tlb.mac_addr_base,RIP_COMMON_WAN_ADDR_SIZE);
                    memcpy(seed_buff,rip_ro.rip_ro_tlb.seed_key,RIP_PRIV_SEED_SIZE);

                    memcpy(key_buff,key_generation(sn_buff,mac_buff,seed_buff),64);
                    
                    sprintf(encPath,"%s%s", path, rip[i].filename);
                    sprintf(fullPath, "%s%s", path, rip[i].filename+4);

                    if(aes_encrypt_file_key(fullPath,encPath,key_buff) == 0)
                    {
                        //memcpy(fullPath,encPath,sizeof(encPath));
						fprintf(stdout, "Success Encryption File %s\n", rip[i].filename);
                    }
                    else
                    {
                        fprintf(stderr, "ERROR: Failed Encryption File %s\n", rip[i].filename);
                    }
                    break;

                    default:

                    break;
            }
        }
	}
    return 0;
}
