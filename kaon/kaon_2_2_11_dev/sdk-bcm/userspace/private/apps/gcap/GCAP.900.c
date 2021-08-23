#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#include <mocalib.h>
#include <mocaint.h>

GCAP_GEN static void showUsage()
{
    printf("Usage: GCAP.900 [enable|disable]\n\
Disable or enable use of short (6us) IFG\n\
\n\
Options:\n\
 <ifg_disable>  \"disable\" short IFG, or \"enable\" short IFG\n\
  -M   Make configuration changes permanent\n\
  -r   Reset SoC to make configuration changes effective\n\
  -h   Display this help and exit\n");
}

GCAP_GEN int GCAP_900_main(int argc, char **argv)
{
    int ret = MOCA_API_SUCCESS;
    void *ctx;
    int disable = 0;
    int i;
    char *chipId = NULL;
    int persistent = 0;
    int reset = 0;

    // ----------- Parse parameters
 
   
    if (argc < 2)
    {
        printf( "Error!  Missing parameter - ifg_disabled\n");
        return(-2);
    }

    disable = atoi(argv[1]);
    if (strcmp(argv[1], "disable") == 0)
       disable = 1;
    else if (strcmp(argv[1], "enable") == 0)
       disable = 0;
    else
    {
       printf("Unrecognized disable/enable parameter\n");
       return -3;
    }


    for (i=1; i < argc; i++)
    {
       if (strcmp(argv[i], "-i") == 0)
       {
          i++;
          if (i < argc)
             chipId = argv[i];
       } 
       else if (strcmp(argv[i], "-M") == 0)
       {
          persistent = 1;
       }
       else if (strcmp(argv[i], "-r") == 0)
       {
          reset = 1;
       }
       else if (strcmp(argv[i], "-h") == 0)
       {
          showUsage();
          return(0); 
       }
    }

    // ----------- Initialize

    ctx = moca_open(chipId);

    if (!ctx)
    {
        printf( "Error!  Unable to connect to moca instance\n");
        moca_close(ctx);
        return(-2);
    }
        
    // ----------- Save Settings 

    ret = moca_set_no_ifg6(ctx, disable);

    if (ret != MOCA_API_SUCCESS)
    {
       moca_close(ctx);
       printf( "Error!  moca_set_no_ifg6\n");
       return(-4);
    }

    // ----------- Activate Settings   
    if (reset)
    {
        ret = moca_set_restart(ctx);
    
        if (ret != MOCA_API_SUCCESS)
        {
           moca_close(ctx);
           printf( "Error!  Invalid parameter - OF\n");
           return(-9);
        }
    }

    if (persistent)
    {
       ret = moca_set_persistent(ctx);
       if (ret != MOCA_API_SUCCESS)
       {
          moca_close(ctx);
          printf( "Error!  Unable to save persistent parameters\n");
          return(-10);
       }
    }

    // ----------- Finish
    moca_close(ctx);
    return(0);
}
