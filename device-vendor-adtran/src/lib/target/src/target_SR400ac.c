#include <stdio.h>
#include <stdbool.h>
#include <wait.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "os_types.h"
#include "os_util.h"
#include "os_nif.h"
#include "util.h"
#include "os.h"
#include "target.h"
#include "log.h"
#include "const.h"
#include "kconfig.h"

/* 
 * Password used to encrypt and decrypt the certificate is $Key4PlumeCertificates
 * This password is copied to a text file (pass.txt) and hashed using the command
 * "openssl dgst -sha256 pass.txt". The hash output is
 * "d7a7404a8e7162cf648c7de5dbb296125815dc38c031c2daadf1416e5720b84b".
 * Using this hash as the password for encryption/decryption of certificates.
 *
 */
#define PASS "d7a7404a8e7162cf648c7de5dbb296125815dc38c031c2daadf1416e5720b84b"
#define CHIP_CMDD       "openssl enc -d -aes-256-cbc -pass pass"
#define CHIP_CONFIG     "-md sha256 -pbkdf2 -iter 9595 -nosalt"

#define SR400AC_MFG_MODEL_GET  "echo -n \"$(strings /dev/mtd6|grep MFG_MODEL|cut -d \"=\" -f2|cut -c1-5)\"AC\"\""
#define SR400AC_MFG_SERIAL_GET "echo -n \"$(strings /dev/mtd6|grep MFG_SERIAL|cut -d \"=\" -f2)\""
#define SR400AC_MFG_HWREV_GET  "echo -n \"$(strings /dev/mtd6|grep MFG_HWREV|cut -d \"=\" -f2)\""
#define PLATFORM_VERSION "echo -n \"$(cat /usr/opensync/.versions|grep OPENSYNC|cut -d \":\" -f2)\""
#define SR400_TARGET_CERT_PATH "/var/certs"

const char *target_tls_upload_cert_filename(void);

static int target_put_cpush(char* k, const char* i, const char* o)
{
    char dcmd[256] = {0};
    snprintf(dcmd, sizeof(dcmd), "%s:%s %s -in %s -out %s", \
                               CHIP_CMDD, k, CHIP_CONFIG, i, o);
    int rc = system(dcmd);

    return (!(WIFEXITED(rc) && WEXITSTATUS(rc) == 0)) ? -1 : 0;
}

bool target_init_vendor(target_init_opt_t opt, struct ev_loop *loop)
{
    char sk[128] = {0};
    char cmd[128] = {0};
    int rc;
    strcpy (cmd, "rm -rf /var/certs");
    rc = system(cmd);
    strcpy (cmd, "mkdir -p /var/certs");
    rc = system(cmd);
    if ((opt == TARGET_INIT_MGR_CM) || (opt ==TARGET_INIT_MGR_QM))
    {
            strcpy(sk, PASS);
            target_put_cpush(sk, "/usr/opensync/certs/ca", target_tls_cacert_filename());
            target_put_cpush(sk, "/usr/opensync/certs/cl", target_tls_mycert_filename());
            target_put_cpush(sk, "/usr/opensync/certs/dk", target_tls_privkey_filename());
            target_put_cpush(sk, "/usr/opensync/certs/up", target_tls_upload_cert_filename());
    }
    memset (cmd,0,128);
    strcpy (cmd, "touch /var/certs/*");
    rc = system(cmd);

    return true;
}

bool getCmdOutput(char *cmdBuf, char *str, size_t sz)
{
   memset(str,'\0',sz);
   FILE *cmd = popen(cmdBuf, "r");
   fscanf(cmd, "%s", str);
   pclose(cmd);
   if (str[0] != '\0')
   {
      return true;
   }
   return false;
}

bool target_serial_get(void *buff, size_t buffsz)
{
    bool n;
    memset(buff,'\0', buffsz);
    n = getCmdOutput(SR400AC_MFG_SERIAL_GET, buff, buffsz);
    if (n == true)
    {
       LOG(DEBUG,"target_serial_get: serial_number=%s", buff);
       return true;
    }
    else
    {
       LOG(ERR, "target_serial_get: serial number not found.");
       return false;
    }
}

bool target_hw_revision_get(void *buff, size_t buffsz)
{
    bool n;
    memset(buff,'\0', buffsz);
    n = getCmdOutput(SR400AC_MFG_HWREV_GET, buff, buffsz);
    if (n == true)
    {
       LOG(DEBUG,"target_hw_get: hw_rev=%s", buff);
       return true;
    }
    else
    {
       LOG(ERR, "target_hw_get: HW revision not found.");
       return false;
    }
}

bool target_model_get(void *buff, size_t buffsz)
{
    bool n;
    memset(buff,'\0', buffsz);
    n = getCmdOutput(SR400AC_MFG_MODEL_GET, buff, buffsz);
    if (n == true)
    {
       LOG(DEBUG,"target_model_get: Model=%s", buff);
       return true;
    }
    else
    {
       LOG(ERR, "target_model_get: Model name not found.");
       return false;
    }
}

bool target_platform_version_get(void *buff, size_t buffsz)
{
    bool n;
    memset(buff,'\0', buffsz);
    n = getCmdOutput(PLATFORM_VERSION, buff, buffsz);
    if (n == true)
    {
       LOG(DEBUG,"target_platform_version_get: PLATFORM_VERSION=%s", buff);
       return true;
    }
    else
    {
       LOG(ERR, "target_platform_version_get: platform version not found.");
       return false;
    }
}

const char *target_wan_interface_name()
{
    const char *iface_name = "eth0.2";
    return iface_name;
}

const char *target_tls_cacert_filename(void)
{
    return SR400_TARGET_CERT_PATH "/ca.pem";
}

const char *target_tls_mycert_filename(void)
{
    return SR400_TARGET_CERT_PATH "/client.pem";
}

const char *target_tls_privkey_filename(void)
{
    return SR400_TARGET_CERT_PATH "/client_dec.key";
}

const char *target_tls_upload_cert_filename(void)
{
    return SR400_TARGET_CERT_PATH "/upload.pem";
}
