#include <string.h>

#include <microhttpd.h>

#include "mhd.h"
#include "beep.h"
#include "cms_params_modsw.h"

#define EE_INFO   BEEP_PERSISTENT_STORAGE_DIR"/ee_info"

static int glb_reqId = 0;

/* ---- Private Constants and Types --------------------------------------- */


extern const char *errorpage;

const char *redirect_page =
"<html>\
  <head>\
    <meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>\
    <script language='javascript'>\
      function frmLoad() {\
        window.location = '%s';\
      }\
    </script>\
  </head>\
  <body onLoad='frmLoad()'>\
  </body>\
</html>";


/* ---- Private Function Prototypes --------------------------------------- */


static int ee_view(struct MHD_Connection *connection);

static int ee_install(struct MHD_Connection *connection);

static int ee_start(struct MHD_Connection *connection);

static int ee_stop(struct MHD_Connection *connection);

static int ee_uninstall(struct MHD_Connection *connection);

static int cont_detail(struct MHD_Connection *connection);


/* ---- Public Variables -------------------------------------------------- */

COMMAND_INFO mhd_cmds[] =
{
   {
      "", ee_view
   },
   {
      "ee_view", ee_view
   },
   {
      "ee_install", ee_install
   },
   {
      "ee_start", ee_start
   },
   {
      "ee_stop", ee_stop
   },
   {
      "ee_uninstall", ee_uninstall
   },
   {
      "cont_detail", cont_detail
   }
};


static int container_get_info(const char *container_name,
                              PCONTAINER_INFO info)
{
   int ret = MHD_NO;
   char cmd[MHD_BUFLEN_256], buf[MHD_BUFLEN_256];
   char name[MHD_BUFLEN_64], val1[MHD_BUFLEN_64];
   char val2[MHD_BUFLEN_64], val3[MHD_BUFLEN_64];
   FILE *fp = NULL;

   snprintf(cmd, sizeof(cmd), "lxc-info -n %s", container_name);

   if ((fp = popen(cmd, "r")) == NULL)
   {
      printf("Error opening pipe! cmd=%s \n", cmd);
      return ret;
   }

   while (fgets(buf, MHD_BUFLEN_256, fp) != NULL)
   {
      memset(name, 0, MHD_BUFLEN_64);
      memset(val1, 0, MHD_BUFLEN_64);
      memset(val2, 0, MHD_BUFLEN_64);
      memset(val3, 0, MHD_BUFLEN_64);

      sscanf(buf, "%s %s %s %s", name, val1, val2, val3);

      if (val2[0] != '\0')
      {
         if (strcmp(name, "CPU") == 0)
         {
	        snprintf(info->cpu_use, MHD_BUFLEN_32, "%s %s", val2, val3);
         }
         else if (strcmp(name, "Memory") == 0)
         {
	        snprintf(info->mem_use, MHD_BUFLEN_32, "%s %s", val2, val3);
         }
         else if (strcmp(name, "TX") == 0)
         {
	        snprintf(info->byte_sent, MHD_BUFLEN_32, "%s %s", val2, val3);
         }
         else if (strcmp(name, "RX") == 0)
         {
	        snprintf(info->byte_received, MHD_BUFLEN_32, "%s %s",
                     val2, val3);
         }
      }
      else
      {
         if (strcmp(name, "Name:") == 0)
         {
            strncpy(info->name, val1, MHD_BUFLEN_64);
         }
         else if (strcmp(name, "State:") == 0)
         {
            strncpy(info->status, val1, MHD_BUFLEN_32);
         }
         else if (strcmp(name, "PID:") == 0)
         {
            strncpy(info->pid, val1, MHD_BUFLEN_32);
         }
         else if (strcmp(name, "Link:") == 0)
         {
            strncpy(info->interface, val1, MHD_BUFLEN_32);
         }
         else if (strcmp(name, "IP:") == 0)
         {
	        if (info->ipv4_addrs[0] == '\0')
            {
               strncpy(info->ipv4_addrs, val1, MHD_BUFLEN_32);
            }
	        else
            {
               strncat(info->ipv4_addrs, ", ",
                       MHD_BUFLEN_256+MHD_BUFLEN_4);
               strncat(info->ipv4_addrs, val1,
                       MHD_BUFLEN_256+MHD_BUFLEN_4);
            }
	     }
      }
   }

   if (pclose(fp))
   {
      printf("Command not found or exited with error status");
   }
   else
      ret = MHD_YES;

   return ret;
}


static int get_state_by_name(const char *container_name)
{
   char cmd[MHD_BUFLEN_128];
   char line[MHD_BUFLEN_128];
   char str1[MHD_BUFLEN_16], str2[MHD_BUFLEN_16];
   FILE *fp = NULL;
   int state = 0;

   snprintf(cmd, sizeof(cmd), "lxc-info -n %s -s 2> /dev/null",
            container_name);

   if ((fp = popen(cmd, "r")) == NULL)
   {
      printf("Error opening pipe! cmd=%s \n", cmd);
      return state;
   }

   memset(str1, 0, MHD_BUFLEN_16);
   memset(str2, 0, MHD_BUFLEN_16);

   if (fgets(line, MHD_BUFLEN_128, fp) != NULL)
   {
      sscanf(line, "%s %s", str1, str2);

      if (*(str2+strlen(str2)-1) == '\n')
      {
         *(str2+strlen(str2)-1) = '\0';
      }

      /* Return up if the state is not STOPPED */
      if (strcmp(str2, "STOPPED") != 0)
      {
         state = 1;
      }
   }

   pclose(fp);

   return state;
}


static int ee_get_info(BeepEeInfo_t *info)
{
   int i = 0, buf_len = 0;
   int ret = MHD_NO;
   char line[MHD_BUFLEN_1024*10] = {0};
   char *hex_str = NULL;
   const char *p1 = NULL, *p2 = NULL;
   unsigned char *buf = NULL;
   const unsigned char *ptr = NULL;
   FILE *fp = NULL;

#if 0
// FORMAT of EE_INFO
method return time=1554504980.918587 sender=:1.0 -> destination=:1.6 serial=39 reply_serial=2
   /* result code */  uint32 0
   /* requestId */    int32 1
   /* entry number */ uint32 1   
   /* BeepEeInfo_t */ string "687474703a2f2f3139322e3136382e312e3130322f706b675f626565705f4578616d706c6545455f312e312e7461722e677a0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004578616d706c654545000000000000000000000000000000000000000000000042726f6164636f6d204c74642e00000000000000000000000000000000000000312e310000000000000000000000000042726f6164636f6d204578616d706c6520457865637574696f6e20456e7669726f6e6d656e740000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000006578616d706c6545450000000000000000000000000000000000000000000000000000000000000000000000000000006b3666633039663733643533343662323266333939333234646463653539643700424545505f4578616d706c654545000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000ffffffff0a00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002d31010000000000640000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff000000000000000000000000000000000000000000000000ffffffffffffffff00000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000ffffffff000000000000000000000000000000000000000000000000000000000000000000000000"
   /* BeepDuInfo_t */ string "687474703a2f2f3139322e3136382e312e3130322f706b675f626565705f4578616d706c6545455f312e312e7461722e677a00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004578616d706c654545000000000000000000000000000000000000000000000042726f6164636f6d204c74642e00000000000000000000000000000000000000312e310000000000000000000000000042726f6164636f6d204578616d706c6520457865637574696f6e20456e7669726f6e6d656e74000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010000000100000001000000"
#endif

   if ((fp = fopen(EE_INFO, "r")) == NULL)
   {
      //printf("Error opening file %s\n", EE_INFO);
      return ret;
   }

   /* read pass the first 4 lines until reach the 5 one */
   for (i = 0; i < 5; i++)
   {
      if (fgets(line, sizeof(line), fp) == NULL)
      {
         fclose(fp);
         return ret;
      }
   }

   fclose(fp);

   /* parse 5th line to get ee info */
   p1 = strchr(line, '"');
   if (p1 == NULL)
      return ret;
   p2 = strrchr(line, '"');
   if (p2 == NULL)
      return ret;

   buf_len = p2 - p1;
   /* return if 5th line has empty string "" */
   if (buf_len < 2)
      return ret;

   hex_str = (char *)malloc(buf_len);
   memset(hex_str, 0, buf_len);
   strncpy(hex_str, ++p1, buf_len-1);

   buf_len = strlen(hex_str) / 2;
   ptr = (unsigned char *)hex_str;

   buf = (unsigned char *)malloc(buf_len);
   memset(buf, 0, buf_len);
   for (i = 0; i < buf_len; i++)
   {
      sscanf((char *)ptr, "%02hhx", &buf[i]);
      ptr += 2;
   }

   memcpy(info, buf, sizeof(BeepEeInfo_t));

   free(hex_str);
   free(buf);

#if 0
   printf("%s/%s/%s/%s/%s/%s/%s/%d/%llu/%llu/%llu/%d\n",
          info->url, info->name, info->version, info->vendor,
          info->description, info->mngrAppName, info->containerName, 
          info->privileged, info->resource.cpu, info->resource.memory,
          info->resource.flash, info->runLevel);
#endif

   return MHD_YES;
}


static int ee_view(struct MHD_Connection *connection)
{
   int ret = MHD_NO;
   int state = 0;
   char page[MHD_BUFLEN_1024*10] = {0};
   char *cur = page;
   const char *end = page + sizeof(page);
   BeepEeInfo_t info;

   memset(&info, 0, sizeof(BeepEeInfo_t));
   ret = ee_get_info(&info);

   if (ret == MHD_YES)
   {
      state = get_state_by_name(info.containerName);
   }

   memset(page, 0, sizeof(page));

   cur += snprintf(cur, end-cur, "<html>\n");   
   cur += snprintf(cur, end-cur, "  <head>\n");
   cur += snprintf(cur, end-cur, "    <meta http-equiv='Pragma' content='no-cache'>\n");
   cur += snprintf(cur, end-cur, "    <meta http-equiv='refresh' content='30'>\n");
   cur += snprintf(cur, end-cur, "    <link rel='stylesheet' href='tabletab.css' type='text/css'>\n");
   cur += snprintf(cur, end-cur, "    <script src='jquery.js'></script>\n");
   cur += snprintf(cur, end-cur, "    <script src='util.js'></script>\n");
   cur += snprintf(cur, end-cur, "    <title>Execution Environment Table</title>\n");
   cur += snprintf(cur, end-cur, "    <style>\n");
   cur += snprintf(cur, end-cur, "      table#tblContainer tr:nth-child(even) {\n");
   cur += snprintf(cur, end-cur, "        background-color: #f1f1c1\n");
   cur += snprintf(cur, end-cur, "      }\n");
   cur += snprintf(cur, end-cur, "      table#tblContainer tr:nth-child(odd) {\n");
   cur += snprintf(cur, end-cur, "        background-color: #f2f2f2\n");
   cur += snprintf(cur, end-cur, "      }\n");
   cur += snprintf(cur, end-cur, "    </style>\n");
   cur += snprintf(cur, end-cur, "    <script language='javascript'>\n");
   cur += snprintf(cur, end-cur, "      function btnInstall() {\n");
   cur += snprintf(cur, end-cur, "        var loc = 'ee_install.html';\n\n");
   cur += snprintf(cur, end-cur, "        var code = 'location=\"' + loc + '\"';\n\n");
   cur += snprintf(cur, end-cur, "        eval(code);\n");
   cur += snprintf(cur, end-cur, "      }\n\n");
   cur += snprintf(cur, end-cur, "      function btnStart() {\n");
   cur += snprintf(cur, end-cur, "        var loc = 'ee_start?name=%s&vendor=%s&version=%s';\n\n",
                   info.name, info.vendor, info.version);
   cur += snprintf(cur, end-cur, "        var code = 'location=\"' + loc + '\"';\n\n");
   cur += snprintf(cur, end-cur, "        eval(code);\n");
   cur += snprintf(cur, end-cur, "      }\n\n");
   cur += snprintf(cur, end-cur, "      function btnStop() {\n");
   cur += snprintf(cur, end-cur, "        var loc = 'ee_stop?name=%s&vendor=%s&version=%s';\n\n",
                   info.name, info.vendor, info.version);
   cur += snprintf(cur, end-cur, "        var code = 'location=\"' + loc + '\"';\n\n");
   cur += snprintf(cur, end-cur, "        eval(code);\n");
   cur += snprintf(cur, end-cur, "      }\n\n");
   cur += snprintf(cur, end-cur, "      function btnUninstall() {\n");
   cur += snprintf(cur, end-cur, "        var loc = 'ee_uninstall?name=%s&vendor=%s&version=%s';\n\n",
                   info.name, info.vendor, info.version);
   cur += snprintf(cur, end-cur, "        var code = 'location=\"' + loc + '\"';\n\n");
   cur += snprintf(cur, end-cur, "        eval(code);\n");
   cur += snprintf(cur, end-cur, "      }\n");
   cur += snprintf(cur, end-cur, "    </script>\n");
   cur += snprintf(cur, end-cur, "  </head>\n");
   cur += snprintf(cur, end-cur, "  <body>\n");
   cur += snprintf(cur, end-cur, "    <blockquote>\n");
   cur += snprintf(cur, end-cur, "    <form>\n");
   cur += snprintf(cur, end-cur, "      <center><b>Execution Environment Table</b><br><br>\n");
   cur += snprintf(cur, end-cur, "      Click on button to perform life cycle operation of the EE.<br><br>\n");
   cur += snprintf(cur, end-cur, "      <table id='tblContainer' border='1'cellpadding='4' cellspacing='0'>\n");
   cur += snprintf(cur, end-cur, "        <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "          <td class='hd'>Name</td>\n");
   cur += snprintf(cur, end-cur, "          <td class='hd'>Vendor</td>\n");
   cur += snprintf(cur, end-cur, "          <td class='hd'>Version</td>\n");
   cur += snprintf(cur, end-cur, "          <td class='hd'>Description</td>\n");
   cur += snprintf(cur, end-cur, "          <td class='hd'>Status</td>\n");
   cur += snprintf(cur, end-cur, "        </tr>\n");
   cur += snprintf(cur, end-cur, "        <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "          <td>%s</td>\n", info.name);
   cur += snprintf(cur, end-cur, "          <td>%s</td>\n", info.vendor);
   cur += snprintf(cur, end-cur, "          <td>%s</td>\n", info.version);
   cur += snprintf(cur, end-cur, "          <td>%s</td>\n", info.description);
   if (info.name[0] != '\0')
   {
      if (state == 1)
         cur += snprintf(cur, end-cur, "          <td>Active</td>\n");
      else
         cur += snprintf(cur, end-cur, "          <td>Idle</td>\n");
   }
   else
   {
      cur += snprintf(cur, end-cur, "          <td></td>\n");
   }
   cur += snprintf(cur, end-cur, "        </tr>\n");
   cur += snprintf(cur, end-cur, "      </table>\n");
   cur += snprintf(cur, end-cur, "      <br><br>\n");
   cur += snprintf(cur, end-cur, "      <center>\n");
   cur += snprintf(cur, end-cur, "        <input type='button' onClick='btnInstall()' value='Install'>\n");
   cur += snprintf(cur, end-cur, "        <input type='button' onClick='btnStart()' value='Start' %s>\n",
      ((info.name[0] == '\0') || (state == 1)) ? "disabled='1'" : "");
   cur += snprintf(cur, end-cur, "        <input type='button' onClick='btnStop()' value='Stop' %s>\n",
      (state == 0) ? "disabled='1'" : "");
   cur += snprintf(cur, end-cur, "        <input type='button' onClick='btnUninstall()' value='Uninstall' %s>\n",
      (info.name[0] == '\0') ?  "disabled='1'" : "");
   cur += snprintf(cur, end-cur, "      </center>\n");
   cur += snprintf(cur, end-cur, "    </form>\n");
   cur += snprintf(cur, end-cur, "    </blockquote>\n");
   cur += snprintf(cur, end-cur, "  </body>\n");
   cur += snprintf(cur, end-cur, "</html>\n");

   ret = send_page(connection, page);

   return ret;
}


static int ee_install(struct MHD_Connection *connection)
{
   int ret = MHD_YES;
   char cmd[BEEP_PKG_URL_MAX+BEEP_PKG_USERNAME_MAX+BEEP_PKG_PASSWORD_MAX+MHD_BUFLEN_512] = {0};
   char page[MHD_BUFLEN_1024*10] = {0};
   const char *url = NULL;
   const char *user = NULL;
   const char *pwd = NULL;

   url = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "url");
   user = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "user");
   pwd = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "pwd");

   snprintf(cmd, sizeof(cmd), 
"dbus-send --system --dest=com.broadcom.spd \
--type=method_call --print-reply \
/com/broadcom/spd com.broadcom.spd.ChangeDuState \
int32:%d string:'%s' string:'%s' string:'%s' string:'%s' \
string:'%s' string:'%s' string:'%s' > %s\n",
            ++glb_reqId, "install", url, user, pwd,
            "", "", "", EE_INFO);

   if (system(cmd) == -1)
   {
      snprintf(page, sizeof(page),
               "<html><body><h1>Fail to execute command: %s</h1></body></html>",
               cmd);

      ret =  send_page(connection, page);
   }
   else
   {
      snprintf(page, sizeof(page), redirect_page, "/");

      ret =  send_page(connection, page);
   }

   return ret;
}


static int ee_start(struct MHD_Connection *connection)
{
   int ret = MHD_YES;
   char cmd[BEEP_PKG_NAME_LEN_MAX+BEEP_PKG_VENDOR_LEN_MAX+BEEP_PKG_BEEP_VER_LEN_MAX+MHD_BUFLEN_256] = {0};
   char page[MHD_BUFLEN_1024*10] = {0};
   const char *name = NULL;
   const char *vendor = NULL;
   const char *version = NULL;

   name = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "name");
   vendor = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "vendor");
   version = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "version");

   if (strstr(name, "CMS") != NULL)
   {
      snprintf(cmd, sizeof(cmd), "ifconfig br0 down;brctl delbr br0");

      if (system(cmd) == -1)
      {
         snprintf(page, sizeof(page),
                  "<html><body><h1>Fail to execute command: %s</h1></body></html>",
                  cmd);

         ret =  send_page(connection, page);

         goto out;
      }
   }

   snprintf(cmd, sizeof(cmd), 
"dbus-send --system --dest=com.broadcom.spd \
--type=method_call --print-reply \
/com/broadcom/spd com.broadcom.spd.StartExecEnv \
int32:%d string:'%s' string:'%s' string:'%s' > /dev/null\n",
            ++glb_reqId, name, vendor, version);

   if (system(cmd) == -1)
   {
      snprintf(page, sizeof(page),
               "<html><body><h1>Fail to execute command: %s</h1></body></html>",
               cmd);

      ret =  send_page(connection, page);

      goto out;
   }
   else
   {
      snprintf(page, sizeof(page), redirect_page, "/");

      ret =  send_page(connection, page);
   }

out:

   return ret;
}


static int ee_stop(struct MHD_Connection *connection)
{
   int ret = MHD_YES;
   char cmd[BEEP_PKG_NAME_LEN_MAX+BEEP_PKG_VENDOR_LEN_MAX+BEEP_PKG_BEEP_VER_LEN_MAX+MHD_BUFLEN_256] = {0};
   char page[MHD_BUFLEN_1024*10] = {0};
   const char *name = NULL;
   const char *vendor = NULL;
   const char *version = NULL;

   name = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "name");
   vendor = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "vendor");
   version = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "version");

   snprintf(cmd, sizeof(cmd), 
"dbus-send --system --dest=com.broadcom.spd \
--type=method_call --print-reply \
/com/broadcom/spd com.broadcom.spd.StopExecEnv \
int32:%d string:'%s' string:'%s' string:'%s' > /dev/null\n",
            ++glb_reqId, name, vendor, version);

   if (system(cmd) == -1)
   {
      snprintf(page, sizeof(page),
               "<html><body><h1>Fail to execute command: %s</h1></body></html>",
               cmd);

      ret =  send_page(connection, page);
   }
   else
   {
      snprintf(page, sizeof(page), redirect_page, "/");

      ret =  send_page(connection, page);
   }

   return ret;
}


static int ee_uninstall(struct MHD_Connection *connection)
{
   int ret = MHD_YES;
   char cmd[BEEP_PKG_NAME_LEN_MAX+BEEP_PKG_VENDOR_LEN_MAX+BEEP_PKG_BEEP_VER_LEN_MAX+MHD_BUFLEN_512] = {0};
   char page[MHD_BUFLEN_1024*10] = {0};
   const char *name = NULL;
   const char *vendor = NULL;
   const char *version = NULL;

   name = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "name");
   vendor = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "vendor");
   version = MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "version");

   snprintf(cmd, sizeof(cmd), 
"dbus-send --system --dest=com.broadcom.spd \
--type=method_call --print-reply \
/com/broadcom/spd com.broadcom.spd.ChangeDuState \
int32:%d string:'%s' string:'%s' string:'%s' string:'%s' \
string:'%s' string:'%s' string:'%s' > %s\n",
            ++glb_reqId, "uninstall", "", "", "",
            name, vendor, version, EE_INFO);

   if (system(cmd) == -1)
   {
      snprintf(page, sizeof(page),
               "<html><body><h1>Fail to execute command: %s</h1></body></html>",
               cmd);

      ret =  send_page(connection, page);
   }
   else
   {
      /* rm -f /data/ee_info */
      snprintf(cmd, sizeof(cmd)-1, "rm -f %s", EE_INFO);
      system(cmd);
      sleep(2); /* wait for EE is removed */

      snprintf(page, sizeof(page), redirect_page, "/");

      ret =  send_page(connection, page);
   }

   return ret;
}


static int cont_detail(struct MHD_Connection *connection)
{
   int ret = MHD_NO;
   char page[MHD_BUFLEN_1024*10] = {0};
   char *cur = page;
   const char *end = page + sizeof(page);
   BeepEeInfo_t info;
   CONTAINER_INFO cont_info;
   
   memset(&info, 0, sizeof(BeepEeInfo_t));
   ret = ee_get_info(&info);

   memset(&cont_info, 0, sizeof(CONTAINER_INFO));
   if (ret == MHD_YES)
   {
      get_state_by_name(info.containerName);
      container_get_info(info.containerName, &cont_info);
   }

   memset(page, 0, sizeof(page));

   cur += snprintf(cur, end-cur, "<html>\n");   
   cur += snprintf(cur, end-cur, "  <head>\n");
   cur += snprintf(cur, end-cur, "    <meta http-equiv='Pragma' content='no-cache'>\n");
   cur += snprintf(cur, end-cur, "    <link rel='stylesheet' href='tabletab.css' type='text/css'>\n");
   cur += snprintf(cur, end-cur, "    <script src='jquery.js'></script>\n");
   cur += snprintf(cur, end-cur, "    <title>Container Detail Information</title>\n");
   cur += snprintf(cur, end-cur, "    <script language='javascript'>\n");
   cur += snprintf(cur, end-cur, "      $(document).ready(function() {\n");
   cur += snprintf(cur, end-cur, "        // Get the element with id='defaultTab' and click on it\n");
   cur += snprintf(cur, end-cur, "        document.getElementById('defaultTab').click();\n");
   cur += snprintf(cur, end-cur, "      });\n\n");
   cur += snprintf(cur, end-cur, "      function openTab(evt, tabName) {\n");
   cur += snprintf(cur, end-cur, "        var i, tabcontent, tablinks;\n\n");
   cur += snprintf(cur, end-cur, "        tabcontent = document.getElementsByClassName('tabcontent');\n\n");
   cur += snprintf(cur, end-cur, "        for (i = 0; i < tabcontent.length; i++) {\n");
   cur += snprintf(cur, end-cur, "          tabcontent[i].style.display = 'none';\n");
   cur += snprintf(cur, end-cur, "        }\n\n");
   cur += snprintf(cur, end-cur, "        tablinks = document.getElementsByClassName('tablinks');\n\n");
   cur += snprintf(cur, end-cur, "        for (i = 0; i < tablinks.length; i++) {\n");
   cur += snprintf(cur, end-cur, "          tablinks[i].className = tablinks[i].className.replace(' active', '');\n");
   cur += snprintf(cur, end-cur, "        }\n\n");
   cur += snprintf(cur, end-cur, "        document.getElementById(tabName).style.display = 'block';\n");
   cur += snprintf(cur, end-cur, "        evt.currentTarget.className += ' active';\n");
   cur += snprintf(cur, end-cur, "      }\n\n");
   cur += snprintf(cur, end-cur, "      function defaultOpen() {\n");
   cur += snprintf(cur, end-cur, "        // Get the element with id='defaultTab' and click on it\n");
   cur += snprintf(cur, end-cur, "        document.getElementById('defaultTab').click();\n");
   cur += snprintf(cur, end-cur, "      }\n");
   cur += snprintf(cur, end-cur, "    </script>\n");
   cur += snprintf(cur, end-cur, "  </head>\n");
   cur += snprintf(cur, end-cur, "  <body onload='defaultOpen()'>\n");
   cur += snprintf(cur, end-cur, "    <form>\n");
   cur += snprintf(cur, end-cur, "      <ul class='tab'>\n");
   cur += snprintf(cur, end-cur, "        <li><a href='#' class='tablinks' onclick=\"openTab(event, 'state')\" id='defaultTab'>State</a></li>\n");
   cur += snprintf(cur, end-cur, "        <li><a href='#' class='tablinks' onclick=\"openTab(event, 'network')\">Network</a></li>\n");
   cur += snprintf(cur, end-cur, "        <li><a href='#' class='tablinks' onclick=\"openTab(event, 'stats')\">Statistics</a></li>\n");
   cur += snprintf(cur, end-cur, "      </ul>\n");
   cur += snprintf(cur, end-cur, "    </form>\n");
   cur += snprintf(cur, end-cur, "  </body>\n");
   cur += snprintf(cur, end-cur, "</html>\n");

   cur += snprintf(cur, end-cur, "      <div id='state' class='tabcontent'><center>\n");
   cur += snprintf(cur, end-cur, "        <table border='1' cellpadding='4' cellspacing='0'>\n");
   cur += snprintf(cur, end-cur, "          <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "            <td class='hd' align='center' width='150'>Name</td>\n");
   cur += snprintf(cur, end-cur, "            <td align='center' width='200'>%s</td>\n", (info.containerName? info.containerName:""));
   cur += snprintf(cur, end-cur, "          </tr>\n");
   cur += snprintf(cur, end-cur, "          <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "            <td class='hd' align='center'>ID</td>\n");
   cur += snprintf(cur, end-cur, "            <td align='center'>%s</td>\n", (cont_info.id? cont_info.id:""));
   cur += snprintf(cur, end-cur, "          </tr>\n");
   cur += snprintf(cur, end-cur, "          <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "            <td class='hd' align='center'>Status</td>\n");
   cur += snprintf(cur, end-cur, "            <td align='center'>%s</td>\n", (cont_info.status? cont_info.status:""));
   cur += snprintf(cur, end-cur, "          </tr>\n");
   cur += snprintf(cur, end-cur, "          <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "            <td class='hd' align='center'>PID</td>\n");
   cur += snprintf(cur, end-cur, "            <td align='center'>%s</td>\n", (cont_info.pid? cont_info.pid:""));
   cur += snprintf(cur, end-cur, "          </tr>\n");
   cur += snprintf(cur, end-cur, "          <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "            <td class='hd' align='center'>CPU Use</td>\n");
   cur += snprintf(cur, end-cur, "            <td align='center'>%s</td>\n", (cont_info.cpu_use? cont_info.cpu_use:""));
   cur += snprintf(cur, end-cur, "          </tr>\n");
   cur += snprintf(cur, end-cur, "          <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "            <td class='hd' align='center'>Memory Use</td>\n");
   cur += snprintf(cur, end-cur, "            <td align='center'>%s</td>\n", (cont_info.mem_use? cont_info.mem_use:""));
   cur += snprintf(cur, end-cur, "          </tr>\n");
   cur += snprintf(cur, end-cur, "        </table>\n");
   cur += snprintf(cur, end-cur, "      </div>\n");

   cur += snprintf(cur, end-cur, "      <div id='network' class='tabcontent'><center>\n");
   cur += snprintf(cur, end-cur, "        <table border='1' cellpadding='4' cellspacing='0'>\n");
   cur += snprintf(cur, end-cur, "          <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "            <td class='hd' align='center' width='150'>Interface</td>\n");
   cur += snprintf(cur, end-cur, "            <td align='center' width='200'>%s</td>\n", (cont_info.interface? cont_info.interface:""));
   cur += snprintf(cur, end-cur, "          </tr>\n");
   cur += snprintf(cur, end-cur, "          <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "            <td class='hd' align='center'>IPv4 Addresses</td>\n");
   cur += snprintf(cur, end-cur, "            <td align='center'>%s</td>\n", (cont_info.ipv4_addrs? cont_info.ipv4_addrs:""));
   cur += snprintf(cur, end-cur, "          </tr>\n");
   cur += snprintf(cur, end-cur, "          <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "            <td class='hd' align='center'>Ports</td>\n");
   cur += snprintf(cur, end-cur, "            <td align='center'>%s</td>\n", (cont_info.ports? cont_info.ports:""));
   cur += snprintf(cur, end-cur, "          </tr>\n");
   cur += snprintf(cur, end-cur, "        </table>\n");
   cur += snprintf(cur, end-cur, "      </div>\n");

   cur += snprintf(cur, end-cur, "      <div id='stats' class='tabcontent'><center>\n");
   cur += snprintf(cur, end-cur, "        <table border='1' cellpadding='4' cellspacing='0'>\n");
   cur += snprintf(cur, end-cur, "          <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "            <td class='hd' align='center' width='150'>Transmitted Bytes</td>\n");
   cur += snprintf(cur, end-cur, "            <td align='center' width='200'>%s</td>\n", (cont_info.byte_sent? cont_info.byte_sent:""));
   cur += snprintf(cur, end-cur, "          </tr>\n");
   cur += snprintf(cur, end-cur, "          <tr align='center'>\n");
   cur += snprintf(cur, end-cur, "            <td class='hd' align='center'>Received Bytes</td>\n");
   cur += snprintf(cur, end-cur, "            <td align='center'>%s</td>\n", (cont_info.byte_received? cont_info.byte_received:""));
   cur += snprintf(cur, end-cur, "          </tr>\n");
   cur += snprintf(cur, end-cur, "        </table>\n");
   cur += snprintf(cur, end-cur, "      </div>\n");

   ret = send_page(connection, page);

   return ret;
}


int handle_command(struct MHD_Connection *connection, const char *cmd)
{
   int ret = MHD_YES;
   int i = 0;
   int max =  sizeof(mhd_cmds) / sizeof (COMMAND_INFO);

   for (i = 0; i < max; i++)
   {
      if (strcmp(cmd, mhd_cmds[i].cmd_name) == 0)
      {
         ret = (*mhd_cmds[i].pfn_cmd_handler)(connection);
         break;
      }
   }

   return ret;
}
