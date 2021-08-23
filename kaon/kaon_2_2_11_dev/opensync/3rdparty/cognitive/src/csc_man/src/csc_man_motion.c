#include <ev.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "log.h"

#include "csc_man.h"
#include "csc_man_uci.h"

#define MODULE_ID LOG_MODULE_ID_EVENT

int motion_stack_ready = 0;
int motion_stack_started = 0;
#ifdef CONFIG_CSC_LIBUCI
int driver_enabled_boot = 0;
int driver_enabled_platform = 0;
#else
int driver_enabled_boot = 1;
int driver_enabled_platform = 1;
#endif

#ifdef CONFIG_CSC_LIBUCI
static ev_stat uci_change_watcher;

static int read_uci(void) {
    struct uci_context *ctx = NULL;
    struct uci_package *pkg = NULL;
    struct uci_section *sec = NULL;
    const char *opt = NULL;
    char *str = NULL;

    LOGI("%s", __func__);

    if ((ctx = uci_alloc_context()) == NULL) {
        uci_get_errorstr(ctx, &str, NULL);
        LOGE("%s: uci_alloc_context failed: %s", __func__, str);
        return 1;
    }

    if (uci_load(ctx, MOTION_UCI_PACKAGE, &pkg) || !pkg) {
        uci_get_errorstr(ctx, &str, NULL);
        LOGE("%s: uci_load "MOTION_UCI_PACKAGE" failed: %s", __func__, str);
        driver_enabled_platform = 1;
        goto read_uci_free;
    }

    driver_enabled_platform = !(
        (sec = uci_lookup_section(ctx, pkg, MOTION_UCI_SECTION)) &&
        (opt = uci_lookup_option_string(ctx, sec, MOTION_UCI_OPT_DRIVER_EN)) && (
            !strcmp(opt, "0") ||
            !strcmp(opt, "off") ||
            !strcmp(opt, "false") ||
            !strcmp(opt, "no") ||
            !strcmp(opt, "disabled")
        )
    );

    uci_unload(ctx, pkg);
read_uci_free:
    uci_free_context(ctx);
    LOGI("%s "MOTION_UCI_OPT_DRIVER_EN_STR"=%d", __func__, driver_enabled_platform);
    return 0;
}

static void uci_change_cb(struct ev_loop *loop, ev_stat *w, int revent) {
    read_uci();
    write_ovsdb_state();
}

void write_uci(int driver_en) {
    struct uci_context *ctx = NULL;
    char *str = NULL;
    const char val[] = {'0' + driver_en, '\0'};
    struct uci_ptr ptr = {
        .target  = UCI_TYPE_SECTION,
        .package = MOTION_UCI_PACKAGE,
        .section = MOTION_UCI_SECTION,
        .value   = MOTION_UCI_SECTION,
    };

    LOGI("%s "MOTION_UCI_OPT_DRIVER_EN_STR"=%d" , __func__, driver_en);

    if ((ctx = uci_alloc_context()) == NULL) {
        uci_get_errorstr(ctx, &str, NULL);
        LOGE("%s: uci_alloc_context failed: %s", __func__, str);
        return;
    }

    if (uci_load(ctx, ptr.package, &ptr.p) || !ptr.p) {
        uci_get_errorstr(ctx, &str, NULL);
        LOGE("%s: uci_load "MOTION_UCI_PACKAGE" failed: %s", __func__, str);
        goto write_uci_free;
    }

    // set section name
    if (uci_set(ctx, &ptr) != UCI_OK) {
        uci_get_errorstr(ctx, &str, NULL);
        LOGE("%s: uci_set failed: %s", __func__, str);
        goto write_uci_unload;
    }

    // set option value
    ptr.target = UCI_TYPE_OPTION;
    ptr.option = MOTION_UCI_OPT_DRIVER_EN;
    ptr.value = val;
    if (uci_set(ctx, &ptr) != UCI_OK) {
        uci_get_errorstr(ctx, &str, NULL);
        LOGE("%s: uci_set failed: %s", __func__, str);
        goto write_uci_unload;
    }

    if (uci_commit(ctx, &ptr.p, 0) != UCI_OK) {
        uci_get_errorstr(ctx, &str, NULL);
        LOGE("%s: uci_commit failed: %s", __func__, str);
        goto write_uci_unload;
    }

    driver_enabled_platform = driver_en;

write_uci_unload:
    if (ptr.p)
        uci_unload(ctx, ptr.p);
write_uci_free:
    uci_free_context(ctx);
}
#endif /* CONFIG_CSC_LIBUCI */

static void gen_mosquitto_conf(void) {
    LOGI("Generating mosquitto config");

    FILE *fp = fopen(MOTION_MOSQUITTO_CONF, "w+");
    if (fp == NULL) {
        LOGE("%s fopen %s: %s", __func__, MOTION_MOSQUITTO_CONF, strerror(errno));
        return;
    }

    fprintf(fp,
        "log_dest syslog\n"
        "connection_messages false\n"
        "\n"
        "port 8883\n"
        "cafile %s\n"
        "certfile %s\n"
        "keyfile "MOTION_KEY_OID_FILE"\n"
        "tls_version tlsv1.2\n"
        "require_certificate true\n"
#ifdef CSC_MAN_AUTOSTART_MOTION
        "\n"
        "listener 1883\n"
#endif
        , ca_file, cert_file);

    fclose(fp);
}

void start_motion(void) {
    motion_stack_ready = 1;
    if (!driver_enabled_boot || !driver_enabled_platform || !cert_ready) {
        return;
    }
    if (motion_stack_started) {
        stop_motion(); // Force restart if there is an existing process
    }

    LOGI(__func__);
    gen_mosquitto_conf();
    system(MOTION_RX_MOTION_START);
    system(MOTION_MOSQUITTO_START);
    system(MOTION_BORG_START);
    motion_stack_started = 1;
}

void stop_motion(void) {
    motion_stack_ready = 0;
    if (!motion_stack_started)
        return;
    LOGI(__func__);
    system(MOTION_BORG_STOP);
    system(MOTION_MOSQUITTO_STOP);
    system(MOTION_RX_MOTION_STOP);
    motion_stack_started = 0;
}

#ifdef CONFIG_CSC_LIBUCI
static int read_driver_param(void) {
    static const char path[] = MOTIOIN_DRIVER_DISABLED_PATH;
    char buf[32];
    unsigned module_disabled;
    int len;

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOGE("%s open %s: %s", __func__, path, strerror(errno));
        return 1;
    }
    len = read(fd, buf, sizeof(buf)-1);
    close(fd);
    if (len < 0) {
        LOGE("%s %s: %s", __func__, path, strerror(errno));
        return 1;
    }

    buf[len] = '\0';
    if (sscanf(buf, "%u", &module_disabled) != 1) {
        LOGE("%s %s: invalid param: %s", __func__, path, buf);
        return 1;
    }

    driver_enabled_boot = !module_disabled;
    LOGI("%s driver enabled: %d", __func__, driver_enabled_boot);
    return 0;
}
#endif

static int check_pid_file(const char *path) {
    char buf[32] = "/proc/";
    const int pid_off = sizeof("/proc/")-1;
    int i;

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return 0;

    i = read(fd, &buf[pid_off], sizeof(buf)-pid_off-1);
    close(fd);
    if (i < 1)
        return 0;

    for(i = pid_off; buf[i] >= '0' && buf[i] <= '9'; i++);
    if (i == pid_off)
        return 0;
    buf[i] = '\0';

    return !access(buf, F_OK);
}

int csc_man_motion_init(struct ev_loop *loop) {

#ifdef CONFIG_CSC_LIBUCI
    if (read_driver_param() || read_uci())
        return 1;
#endif

    motion_stack_started =
        check_pid_file(MOTION_RX_MOTION_PID_FILE) &&
        check_pid_file(MOTION_MOSQUITTO_PID_FILE) &&
        check_pid_file(MOTION_BORG_PID_FILE);
    LOGI("motion_stack_started: %d", motion_stack_started);

#ifdef CONFIG_CSC_LIBUCI
    ev_stat_init(&uci_change_watcher, uci_change_cb, MOTION_UCI_CONFIG_PATH, 0.0);
    ev_stat_start(loop, &uci_change_watcher);
#endif

    return 0;
}
