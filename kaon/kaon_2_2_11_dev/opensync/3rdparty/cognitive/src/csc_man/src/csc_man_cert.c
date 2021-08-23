#include <unistd.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "log.h"
#include "csc_man.h"

char ca_file[PATH_MAX] = MOTION_CA_FILE_DEFAULT;
char cert_file[PATH_MAX] = MOTION_CERT_FILE_DEFAULT;
char key_file[PATH_MAX] = MOTION_KEY_FILE_DEFAULT;
int cert_ready = 0;

static int key_to_named_curve(char *in_file, char *out_file) {
    FILE *fp = NULL;
    EVP_PKEY *pkey = NULL;
    EC_KEY *eckey = NULL;
    EC_GROUP *group = NULL;
    char buf[256];
    int err = 1;

    if ((fp = fopen(in_file, "r")) == NULL) {
        LOGE("%s fopen %s: %s", __func__, in_file, strerror(errno));
        goto key_to_named_curve_end;
    }

    if ((pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL)) == NULL) {
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        LOGE("%s PEM_read_PrivateKey %s: %s", __func__, in_file, buf);
        goto key_to_named_curve_end;
    }

    fclose(fp);

    if ((eckey = EVP_PKEY_get1_EC_KEY(pkey)) != NULL &&
        (group = (EC_GROUP *)EC_KEY_get0_group(eckey)) != NULL &&
        EC_GROUP_get_degree(group) == 384)
    {
        if (EC_GROUP_get_curve_name(group) == NID_undef) {
            EC_GROUP_set_curve_name(group, NID_secp384r1);
        }
        EC_GROUP_set_asn1_flag(group, EC_GROUP_get_asn1_flag(group) | OPENSSL_EC_NAMED_CURVE);
    }

    if ((fp = fopen(out_file, "w+")) == NULL) {
        LOGE("%s fopen %s: %s", __func__, out_file, strerror(errno));
        goto key_to_named_curve_end;
    }

    if (!PEM_write_PrivateKey(fp, pkey, NULL, NULL, 0, NULL, NULL)) {
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        LOGE("%s PEM_write_PrivateKey %s: %s", __func__, out_file, buf);
        goto key_to_named_curve_end;
    }

    err = 0;

key_to_named_curve_end:
    if (fp)
        fclose(fp);
    if (eckey)
        EC_KEY_free(eckey);
    if (pkey)
        EVP_PKEY_free(pkey);
    return err;
}

int csc_man_cert_init(void) {
    int err;

    if (cert_ready)
        return 0;

    if (access(ca_file, F_OK) ||
        access(cert_file, F_OK) ||
        access(key_file, F_OK))
    {
        LOGI("Cert files missing, waiting for new paths from ovsdb ...");
        return 1;
    }

    if (!access(MOTION_KEY_OID_FILE, F_OK)) {
        LOGI("MQTT server cert key already generated");
        cert_ready = 1;
        return 0;
    }

    LOGI("Generating MQTT server cert key");
#ifdef CSC_MAN_AUTOSTART_MOTION
    SSL_load_error_strings();
#endif
    err = key_to_named_curve(key_file, MOTION_KEY_OID_FILE);
#ifdef CSC_MAN_AUTOSTART_MOTION
    ERR_free_strings();
#endif

    cert_ready = !err;
    return err;
}
