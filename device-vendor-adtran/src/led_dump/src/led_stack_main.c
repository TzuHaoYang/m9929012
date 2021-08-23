#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <jansson.h>
#include <ev.h>
#include <syslog.h>
#include <getopt.h>

#include "log.h"
#include "target.h"

/*****************************************************************************
 *  API definitions
 *****************************************************************************/

int main(int argc, char ** argv)
{

    target_log_open("led_stack ", 0);
    LOGN("Invoke led stack dump - led_stack");
    dump_state_stack_bin();

    return 0;
}
