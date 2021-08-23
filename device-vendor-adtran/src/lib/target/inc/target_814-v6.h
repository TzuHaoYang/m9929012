#ifndef TARGET_OS_GATEWAY_BCM52_H_INCLUDED
#define TARGET_OS_GATEWAY_BCM52_H_INCLUDED

#include "schema.h"

#include "wl80211.h"
#include "wl80211_client.h"
#include "wl80211_scan.h"
#include "wl80211_survey.h"

#define TARGET_CERT_PATH            "/var/.certs"
#define TARGET_MANAGERS_PID_PATH    "/var/run/opensync"
#define TARGET_OVSDB_SOCK_PATH      "/var/run/db.sock"
#define TARGET_LOGREAD_FILENAME     "/var/log/messages"

//device specific configs
#define TARGET_WIFI_2G_HOME        "wl1.2"
#define TARGET_WIFI_2G_BHAUL       "wl1.1"
#define TARGET_WIFI_2G_ONBOARD     "wl1.3"

#define TARGET_WIFI_5G_HOME        "wl0.2"
#define TARGET_WIFI_5G_BHAUL       "wl0.1"
#define TARGET_WIFI_5G_ONBOARD     "wl0.3"

//overrides
#define TARGET_ID_SZ                32
#define TARGET_L2UF_IFNAME          "br0.l2uf"

#define TARGET_DEFAULT_NTP_SERVER   "time.nist.gov"

/******************************************************************************
 *  MANAGERS definitions
 *****************************************************************************/
#if !defined(CONFIG_TARGET_MANAGER)
#define TARGET_MANAGER_PATH(X)      CONFIG_INSTALL_PREFIX"/bin/"X
#endif

/******************************************************************************
 *  CLIENT definitions
 *****************************************************************************/

/******************************************************************************
 *  RADIO definitions
 *****************************************************************************/

/******************************************************************************
 *  VIF definitions
 *****************************************************************************/

/******************************************************************************
 *  SURVEY definitions
 *****************************************************************************/

/******************************************************************************
 *  NEIGHBOR definitions
 *****************************************************************************/

/******************************************************************************
 *  DEVICE definitions
 *****************************************************************************/

/******************************************************************************
 *  CAPACITY definitions
 *****************************************************************************/


typedef wl80211_client_record_t target_client_record_t;

typedef wl80211_survey_record_t target_survey_record_t;

typedef void target_capacity_data_t;

/******************************************************************************
 *  VENDOR definitions
 *****************************************************************************/

#include "target_common.h"

#endif /* TARGET_OS_GATEWAY_BCM52_H_INCLUDED */
