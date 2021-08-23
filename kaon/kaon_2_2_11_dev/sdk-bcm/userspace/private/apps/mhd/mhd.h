#ifndef __MHD_H__
#define __MHD_H__

/* ---- Include Files ----------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#define MHD_BUFLEN_4         4
#define MHD_BUFLEN_16       16
#define MHD_BUFLEN_32       32
#define MHD_BUFLEN_64       64
#define MHD_BUFLEN_128     128
#define MHD_BUFLEN_256     256
#define MHD_BUFLEN_512     512
#define MHD_BUFLEN_1024   1024

#define CMD_NAME_LEN        64


typedef int (*FN_COMMAND_HANDLER)(struct MHD_Connection *connection);

typedef struct
{
   char cmd_name[CMD_NAME_LEN];
   FN_COMMAND_HANDLER pfn_cmd_handler;
} COMMAND_INFO, *PCOMMAND_INFO;

typedef struct {
   char id[MHD_BUFLEN_64];
   char name[MHD_BUFLEN_64];
   char status[MHD_BUFLEN_32];
   char pid[MHD_BUFLEN_32];
   char cpu_use[MHD_BUFLEN_32];
   char mem_use[MHD_BUFLEN_32];
   char interface[MHD_BUFLEN_32];
   char ipv4_addrs[MHD_BUFLEN_256+MHD_BUFLEN_4];
   char ports[MHD_BUFLEN_64];
   char byte_sent[MHD_BUFLEN_32];
   char byte_received[MHD_BUFLEN_32];
} CONTAINER_INFO, *PCONTAINER_INFO;


int send_page(struct MHD_Connection *connection, const char *page);

int handle_command(struct MHD_Connection *connection, const char *cmd);

#endif /* __MHD_H__ */
