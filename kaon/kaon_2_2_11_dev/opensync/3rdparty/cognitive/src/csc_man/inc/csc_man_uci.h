#ifndef CSC_MAN_UCI_H_INCLUDED
#define CSC_MAN_UCI_H_INCLUDED

#include "kconfig.h"
#include "target.h"

#ifdef CONFIG_CSC_LIBUCI

#include <uci.h>

#define MOTION_UCI_PACKAGE           "wireless"
#define MOTION_UCI_SECTION           MOTION_OVSDB_MODULE
#define MOTION_UCI_OPT_DRIVER_EN     MOTION_OVSDB_DRIVER_EN_KEY
#define MOTION_UCI_OPT_DRIVER_EN_STR MOTION_UCI_PACKAGE"."MOTION_UCI_SECTION"."MOTION_UCI_OPT_DRIVER_EN
#define MOTION_UCI_CONFIG_PATH       UCI_CONFDIR"/"MOTION_UCI_PACKAGE

void write_uci(int driver_en);

#endif /* CONFIG_CSC_LIBUCI */

#endif /* CSC_MAN_UCI_H_INCLUDED */
