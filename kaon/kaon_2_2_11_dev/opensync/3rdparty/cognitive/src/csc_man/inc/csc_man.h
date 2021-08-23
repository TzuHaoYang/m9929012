#ifndef CSC_MAN_H_INCLUDED
#define CSC_MAN_H_INCLUDED

#include <limits.h>

#include "kconfig.h"
#include "csc_man_uci.h"

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

#define MOTIOIN_DRIVER_DISABLED_PATH "/sys/module/motion80211/parameters/module_disabled"

#define MOTION_OVSDB_MODULE          "motion80211"
#define MOTION_OVSDB_DRIVER_EN_KEY   "motion_driver_en"

#define MOTION_RX_MOTION_START       "/etc/init.d/rx_motion start"
#define MOTION_MOSQUITTO_START       "/etc/init.d/mosquitto start"
#define MOTION_BORG_START            "/etc/init.d/borg start"

#define MOTION_RX_MOTION_STOP        "/etc/init.d/rx_motion stop"
#define MOTION_MOSQUITTO_STOP        "/etc/init.d/mosquitto stop"
#define MOTION_BORG_STOP             "/etc/init.d/borg stop"

#define MOTION_RX_MOTION_PID_FILE    "/var/run/rx_motion.pid"
#define MOTION_MOSQUITTO_PID_FILE    "/var/run/mosquitto.pid"
#define MOTION_BORG_PID_FILE         "/var/run/borgmon.pid"

#define MOTION_CA_FILE_DEFAULT       "/var/certs/ca.pem"
#define MOTION_CERT_FILE_DEFAULT     "/var/certs/client.pem"
#define MOTION_KEY_FILE_DEFAULT      "/var/certs/client_dec.key"
#define MOTION_KEY_OID_FILE          "/tmp/client_dec_oid.key"

#define MOTION_MOSQUITTO_CONF        "/tmp/mosquitto.conf"

extern int motion_stack_started;
extern int motion_stack_ready;
extern int driver_enabled_boot;
extern int driver_enabled_platform;

extern char ca_file[PATH_MAX];
extern char cert_file[PATH_MAX];
extern char key_file[PATH_MAX];
extern int cert_ready;

void start_motion(void);
void stop_motion(void);
int csc_man_motion_init(struct ev_loop *loop);

void read_ovsdb_config(void);
void write_ovsdb_state(void);
int csc_man_ovsdb_init(void);
#ifdef CSC_MAN_AUTOSTART_MOTION
void wait_mqtt_headers(void);
#endif

int csc_man_cert_init(void);

#endif /* CSC_MAN_H_INCLUDED */
