#ifndef TARGET_834_5_H_INCLUDED
#define TARGET_834_5_H_INCLUDED

#include "dpp_client.h"
#include "dpp_survey.h"

#define TARGET_CERT_PATH            "/var/run/openvswitch/certs"
#define TARGET_MANAGERS_PID_PATH    "/tmp/dmpid"
#define TARGET_OVSDB_SOCK_PATH      "/var/run/openvswitch/db.sock"
#define TARGET_LOGREAD_FILENAME     "messages"

typedef struct
{
    DPP_TARGET_CLIENT_RECORD_COMMON_STRUCT;
    dpp_client_stats_t  stats;
} target_client_record_t;

typedef struct
{
    DPP_TARGET_SURVEY_RECORD_COMMON_STRUCT;
} target_survey_record_t;

typedef void target_capacity_data_t;


/******************************************************************************
 *  MANAGERS definitions
 *****************************************************************************/


#include "target_common.h"

#endif /* TARGET_834_5_INCLUDED */
