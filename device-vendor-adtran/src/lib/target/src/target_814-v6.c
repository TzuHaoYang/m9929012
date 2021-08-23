#include <stdio.h>
#include <stdbool.h>
#include <wait.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "cms_main.h"
#include "cms_entity.h"

#include "os_types.h"
#include "os_util.h"
#include "os_nif.h"
#include "util.h"
#include "os.h"
#include "target.h"
#include "log.h"
#include "const.h"
#include "kconfig.h"

#define CHIP_CMDK       "find"
#define CHIP_DIR        "\"./usr/opensync/\""
#define CHIP_EXCLUDE    "\"./usr/opensync/.certs/*\""
#define CHIP_SHR1       "xargs -r0 openssl dgst -sha512"
#define CHIP_SHR2       "openssl dgst -sha256"
#define CHIP_FINALK     "awk '{print $2}'"
#define CHIP_CMDD       "openssl enc -d -aes-256-cbc -pass pass"
#define CHIP_CONFIG     "-md sha256 -pbkdf2 -iter 9595 -nosalt"

const char *target_tls_upload_cert_filename(void);

static int target_get_ckey(char* res, int max_size)
{
    char kcmd[256] = {0};

    snprintf(kcmd, sizeof(kcmd), "cd / && %s %s -type f ! -path %s -print0 | sort -z | %s | %s | %s", \
                                CHIP_CMDK, CHIP_DIR, CHIP_EXCLUDE, CHIP_SHR1, CHIP_SHR2, CHIP_FINALK);

    FILE* fd = popen(kcmd, "r");
    if (!fd) { return -1; }

    while (fgets(&res[strlen(res)], max_size - strlen(res), fd) != NULL);

    pclose(fd);

    if(strlen(res))
    {
        while((res[strlen(res)-1] == '\n') || (res[strlen(res)-1] == '\r'))
        {
            res[strlen(res)-1] = 0;
            if(!strlen(res)){ break; }
        }
    }

    return 0;
}

static int target_put_cpush(char* k, const char* i, const char* o)
{
    char dcmd[256] = {0};
    snprintf(dcmd, sizeof(dcmd), "%s:%s %s -in %s -out %s", \
                               CHIP_CMDD, k, CHIP_CONFIG, i, o);
    int rc = system(dcmd);

    return (!(WIFEXITED(rc) && WEXITSTATUS(rc) == 0)) ? -1 : 0;
}

bool target_init(target_init_opt_t opt, struct ev_loop *loop)
{
    char sk[128] = {0};
    if ((opt == TARGET_INIT_MGR_CM) || (opt ==TARGET_INIT_MGR_QM))
    {
        if(access(target_tls_privkey_filename(), F_OK) != 0 )
        {
            target_get_ckey(sk, sizeof(sk));
            target_put_cpush(sk, "/usr/opensync/.certs/ca", target_tls_cacert_filename());
            target_put_cpush(sk, "/usr/opensync/.certs/cl", target_tls_mycert_filename());
            target_put_cpush(sk, "/usr/opensync/.certs/dk", target_tls_privkey_filename());
            target_put_cpush(sk, "/usr/opensync/.certs/up", target_tls_upload_cert_filename());
        }
    }

    return true;
}

const char *target_tls_cacert_filename(void)
{
    // Return path/filename to CA Certificate used to validate cloud
    return TARGET_CERT_PATH "/.ca";
}

const char *target_tls_mycert_filename(void)
{
    // Return path/filename to MY Certificate used to authenticate with cloud
    return TARGET_CERT_PATH "/.cl";
}

const char *target_tls_privkey_filename(void)
{
    // Return path/filename to MY Private Key used to authenticate with cloud
    return TARGET_CERT_PATH "/.dk";
}

const char *target_tls_upload_cert_filename(void)
{
    // Return path/filename to MY Private Key used to authenticate with cloud
    return TARGET_CERT_PATH "/.up";
}

bool target_serial_get(void *buff, size_t buffsz)
{
    char sn[32] = {0};
    memset(buff, 0, buffsz);

    if( cms_serial_get(sn) == -1 )
    {
        LOGE("sn: unable to read serial no");
        return false;
    }

    if(strlen(sn) == 0)
    {
        LOGE("sn: empty serial number");
        return false;
    }

    if (strlen(sn) >= buffsz)
    {
        LOGE("sn: buffer not large enough");
        return false;
    }

    strscpy(buff, sn, buffsz);
    return true;
}

bool target_model_get(void *buff, size_t buffsz)
{
    cms_entity_t tgt;
    if (cms_entity_get(&tgt) == -1) { return false; }

    LOGI("Got Model: %s (%d)", tgt.modelName, buffsz);
    strscpy(buff, tgt.modelName, buffsz);
    return true;
}

bool target_hw_revision_get(void *buff, size_t buffsz)
{
    cms_entity_t tgt;
    if (cms_entity_get(&tgt) == -1) { return false; }

    LOGI("Got HW rev: %s (%d)", tgt.hardwareVersion, buffsz);
    strscpy(buff, tgt.hardwareVersion, buffsz);
    return true;
}

bool target_platform_version_get(void *buff, size_t buffsz)
{
    cms_entity_t tgt;
    if (cms_entity_get(&tgt) == -1) { return false; }

    LOGI("Got SW rev: %s (%d)", tgt.softwareVersion, buffsz);
    strscpy(buff, tgt.softwareVersion, buffsz);
    return true;
}
