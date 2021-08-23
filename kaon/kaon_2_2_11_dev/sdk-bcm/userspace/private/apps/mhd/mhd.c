#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>

#include <microhttpd.h>

#include "mhd.h"
#include "beep_common.h"


#define POSTBUFFERSIZE  MHD_BUFLEN_512
#define MAXNAMESIZE     20
#define MAXANSWERSIZE   MHD_BUFLEN_512

#define GET             0
#define POST            1

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },  
	{"jpg", "image/jpeg"}, 
	{"jpeg","image/jpeg"},
	{"png", "image/png" },  
	{"zip", "image/zip" },  
	{"gz",  "image/gz"  },  
	{"tar", "image/tar" },  
	{"htm", "text/html" },  
	{"html","text/html" },  
	{"css", "text/css" },  
	{"js",  "application/javascript" },  
	{0,0} };

struct connection_info_struct
{
  int connectiontype;
  char *answerstring;
  struct MHD_PostProcessor *postprocessor;
};

char web_root[MHD_BUFLEN_256] = {0};

const char *greetingpage =
  "<html><body><h1>Welcome, %s!</center></h1></body></html>";

const char *errorpage =
  "<html><body>This doesn't seem to be right.</body></html>";


static int
send_page_from_fd(struct MHD_Connection *connection,
                  const char *type,
                  size_t size,
                  int fd)
{
  int ret;
  struct MHD_Response *response;

  response =
    MHD_create_response_from_fd_at_offset64(size, fd, 0);

  if (!response)
    return MHD_NO;

  MHD_add_response_header (response, "Content-Type", type);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);

  return ret;
}


static int
send_page_error(struct MHD_Connection *connection, const char *page)
{
  int ret;
  struct MHD_Response *response;

  response =
	MHD_create_response_from_buffer(strlen(page),
					                (void *) page,
					                MHD_RESPMEM_PERSISTENT);

   if (!response)
      return MHD_NO;

   ret = MHD_queue_response (connection,
                             MHD_HTTP_INTERNAL_SERVER_ERROR,
                             response);
   MHD_destroy_response (response);

  return ret;
}


int send_page(struct MHD_Connection *connection, const char *page)
{
  int ret;
  struct MHD_Response *response;


  response =
    MHD_create_response_from_buffer(strlen(page),
                                    (void *) page,
				                    MHD_RESPMEM_PERSISTENT);
  if (!response)
    return MHD_NO;

  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);

  MHD_destroy_response (response);

  return ret;
}


static int
iterate_post (void *coninfo_cls, enum MHD_ValueKind kind __attribute__((unused)), const char *key,
              const char *filename __attribute__((unused)), const char *content_type __attribute__((unused)),
              const char *transfer_encoding __attribute__((unused)), const char *data, uint64_t off __attribute__((unused)),
              size_t size)
{
  struct connection_info_struct *con_info = coninfo_cls;

  if (0 == strcmp (key, "name"))
    {
      if ((size > 0) && (size <= MAXNAMESIZE))
        {
          char *answerstring;
          answerstring = malloc (MAXANSWERSIZE);
          if (!answerstring)
            return MHD_NO;

          snprintf (answerstring, MAXANSWERSIZE, greetingpage, data);
          con_info->answerstring = answerstring;
        }
      else
        con_info->answerstring = NULL;

      return MHD_NO;
    }

  return MHD_YES;
}

static void
request_completed (void *cls __attribute__((unused)), struct MHD_Connection *connection __attribute__((unused)),
                   void **con_cls, enum MHD_RequestTerminationCode toe __attribute__((unused)))
{
  struct connection_info_struct *con_info = *con_cls;

  if (NULL == con_info)
    return;

  if (con_info->connectiontype == POST)
    {
      MHD_destroy_post_processor (con_info->postprocessor);
      if (con_info->answerstring)
        free (con_info->answerstring);
    }

  free (con_info);
  *con_cls = NULL;
}


#ifndef BRCM_CMS_BUILD

#define PASSWD_FILE  "/var/passwd"
#define GROUP_FILE   "/var/group"


static int mhd_busgate_init(void)
{
   int ret = 0;
   char cmd[MHD_BUFLEN_128];
   char logLine[MHD_BUFLEN_256]={0};
   FILE *fp = NULL;

   if ((fp = fopen(BEEP_PASSWD_FILE, "r")) == NULL)
   {
      /* BEEP_PASSWD_FILE not exists */
      if ((fp = fopen(PASSWD_FILE, "r")) == NULL)
      {
         /* PASSWD_FILE not exists so create empty BEEP_PASSWD_FILE */
         if ((fp = fopen(BEEP_PASSWD_FILE, "w+")) == NULL)
         {
            printf("Failed to create %s\n", BEEP_PASSWD_FILE);
            return -1;
         }
         fclose(fp);
      }
      else
      {
         /* PASSWD_FILE exists so copy it to BEEP_PASSWD_FILE */
         fclose(fp);
         sprintf(cmd, "cp -f %s %s", PASSWD_FILE, BEEP_PASSWD_FILE);
         ret = system(cmd);
         if (ret)
         {
            snprintf(logLine, sizeof(logLine)-1,
                     "Command: %s failed. rc=%d. Possibly out of disk space\n",
                     cmd, ret);
            printf(logLine);
            return -1;
         }
      }
   }
   else
   {
      /* BEEP_PASSWD_FILE exists */
      fclose(fp);
   }

   if ((fp = fopen(BEEP_GROUP_FILE, "r")) == NULL)
   {
      /* BEEP_GROUP_FILE not exists */
      if ((fp = fopen(GROUP_FILE, "r")) == NULL)
      {
         /* GROUP_FILE not exists so create empty BEEP_GROUP_FILE */
         if ((fp = fopen(BEEP_GROUP_FILE, "w+")) == NULL)
         {
            printf("Failed to create %s\n", BEEP_GROUP_FILE);
            return -1;
         }
         fclose(fp);
      }
      else
      {
         /* GROUP_FILE exists so copy it to BEEP_GROUP_FILE */
         fclose(fp);
         sprintf(cmd, "cp -f %s %s", GROUP_FILE, BEEP_GROUP_FILE);
         ret = system(cmd);
         if (ret)
         {
            snprintf(logLine, sizeof(logLine)-1,
                     "Command: %s failed. rc=%d. Possibly out of disk space\n",
                     cmd, ret);
            printf(logLine);
            return -1;
         }
      }
   }
   else
   {
      /* BEEP_GROUP_FILE exists */
      fclose(fp);
   }

   /* 
    * set symbolic link: /var/passwd -> PASSWD_FILE; /var/group -> GROUP_FILE
    * to save both files to flash.
    */
   unlink(PASSWD_FILE);
   unlink(GROUP_FILE);
   sprintf(cmd, "ln -s %s %s; ln -s %s %s", BEEP_PASSWD_FILE, PASSWD_FILE,
           BEEP_GROUP_FILE, GROUP_FILE);
   ret = system(cmd);
   if (ret == -1)
   {
      snprintf(logLine, sizeof(logLine), "Fail to execute command: %s\n", cmd);
      printf(logLine);
   }

   return ret;
}

#endif   /* BRCM_CMS_BUILD */


static int
answer_to_connection (void *cls __attribute__((unused)), struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version __attribute__((unused)), const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  if (NULL == *con_cls)
    {
      struct connection_info_struct *con_info;

      con_info = malloc (sizeof (struct connection_info_struct));
      if (NULL == con_info)
        return MHD_NO;
      con_info->answerstring = NULL;

      if (0 == strcmp (method, "POST"))
        {
          con_info->postprocessor =
            MHD_create_post_processor (connection, POSTBUFFERSIZE,
                                       iterate_post, (void *) con_info);

          if (NULL == con_info->postprocessor)
            {
              free (con_info);
              return MHD_NO;
            }

          con_info->connectiontype = POST;
        }
      else
        con_info->connectiontype = GET;

      *con_cls = (void *) con_info;

      return MHD_YES;
    }

   if (0 == strcmp (method, "GET"))
   {
      int i = 0;
      int len = 0;
      int fd = 0;
      char page[MHD_BUFLEN_512] = {0};
      char path[MHD_BUFLEN_256] = {0};
      char url_local[MHD_BUFLEN_1024*2] = {0};
      char *ptr = NULL;
      struct stat sbuf;

      strncpy(url_local, url, sizeof(url_local));

      /* redirect default page to main.html */
      if (url_local[0] == '/' && url_local[1] == '\0')
      {
         strcpy(url_local, "/mhd-main.html");
      }

      /* look for extension */
      ptr = strchr(url_local, '.');

      /* handle commands if no extension */
      if (ptr == NULL)
      {
         return handle_command(connection, &url_local[1]);
      }

      ptr++;

      for (i = 0; extensions[i].ext != 0; i++)
      {
         len = strlen(extensions[i].ext);
         if (!strncmp(ptr, extensions[i].ext, len))
         {
            break;
         }
      }

      if (extensions[i].ext != 0)
      {
         strcpy(path, web_root);
         strcat(path, url_local);

         fd = open (path, O_RDONLY);

         if (fd < 0)
         {
            snprintf(page, sizeof(page), 
              "<html><body>Cannot open %s file!</body></html>", path);
            return send_page_error(connection, page);
         }

         if (fstat(fd, &sbuf) != 0)
         {
            close (fd);
            snprintf(page, sizeof(page), 
              "<html><body>Cannot fstat %s file!</body></html>", path);
            return send_page_error(connection, page);
         }

         return send_page_from_fd(connection,
                                  extensions[i].filetype,
                                  sbuf.st_size,
                                  fd);
      }

      return send_page (connection, errorpage);
   }

  if (0 == strcmp (method, "POST"))
    {
      struct connection_info_struct *con_info = *con_cls;

      if (*upload_data_size != 0)
        {
          MHD_post_process (con_info->postprocessor, upload_data,
                            *upload_data_size);
          *upload_data_size = 0;

          return MHD_YES;
        }
      else if (NULL != con_info->answerstring)
        return send_page (connection, con_info->answerstring);
    }

  return send_page (connection, errorpage);
}

/**
 * Call with the port number as the only argument.
 * Never terminates (other than by signals, such as CTRL-C).
 */
int
main (int argc, char *const *argv)
{
   struct MHD_Daemon *d;
   struct timeval tv;
   struct timeval *tvp;
   fd_set rs;
   fd_set ws;
   fd_set es;
   MHD_socket max;
   MHD_UNSIGNED_LONG_LONG mhd_timeout;

   if (argc < 2)
   {
      printf ("%s PORT\n", argv[0]);
      return 1;
   }

   if (argc > 2)
   {
      strcpy(web_root, argv[2]);
   }
   else
   {
      strcpy(web_root, "/webs");
   }

#ifndef BRCM_CMS_BUILD
   /* only initialize busgate when build without CMS */
   mhd_busgate_init();
#endif  /* BRCM_CMS_BUILD */

   /* initialize PRNG */
   srand ((unsigned int) time (NULL));

   d = MHD_start_daemon (MHD_USE_DEBUG,
                         atoi (argv[1]),
                         NULL, NULL,
                         &answer_to_connection, NULL,
                         MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 60 * 5,
                         MHD_OPTION_NOTIFY_COMPLETED, &request_completed,
                         NULL, MHD_OPTION_END);
   if (NULL == d)
      return 1;

   while (1)
   {
      max = 0;
      FD_ZERO (&rs);
      FD_ZERO (&ws);
      FD_ZERO (&es);

      if (MHD_YES != MHD_get_fdset (d, &rs, &ws, &es, &max))
         break; /* fatal internal error */

      if (MHD_get_timeout (d, &mhd_timeout) == MHD_YES)
      {
         tv.tv_sec = mhd_timeout / 1000;
         tv.tv_usec = (mhd_timeout - (tv.tv_sec * 1000)) * 1000;
         tvp = &tv;
      }
      else
         tvp = NULL;

      if (-1 == select (max + 1, &rs, &ws, &es, tvp))
      {
         if (EINTR != errno)
            fprintf (stderr,
                     "Aborting due to error during select: %s\n",
                     strerror (errno));
         break;
      }

      MHD_run (d);
   }

   MHD_stop_daemon (d);

   return 0;
}
