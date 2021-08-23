#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "log.h"


int pid_write(const char *pidfile)
{
    int fd = -1;
    int rv = -1;
    int len;
    char buf[100];
    pid_t pid = getpid();
    struct flock fl;

    fd = open(pidfile, O_CREAT | O_WRONLY,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        LOGE("Could not create PID file.");
        goto fail;
    }
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    if (fcntl(fd, F_SETLK, &fl) < 0) {
        LOGE("Another instance of this program is already running.");
        goto fail;
    }

    rv = ftruncate(fd, 0);
    if (rv != 0) {
        LOGE("Could not truncate PID file.");
        goto fail;
    }

    len = snprintf(buf, sizeof(buf), "%d", pid);

    rv = write(fd, buf, len+1);
    if (rv != (len+1)) {
        LOGE("Could not write to PID file.");
        goto fail;
    }

    rv = fsync(fd);
    if (rv != 0) {
        LOGE("Could not sync PID file.");
        goto fail;
    }

    rv = 0;
fail:
    if (fd >= 0) {
        close(fd);
    }

    return rv;
}


pid_t pid_get(const char *pidfile)
{
    int fd = -1;
    int rv = -1;
    pid_t pid = 0;
    char buf[100];

    fd = open(pidfile, O_RDONLY);
    if (fd < 0) {
        /*LOGE("Could not open PID file.");*/
        goto fail;
    }

    rv = read(fd, buf, sizeof(buf)-1);
    if (rv < 0) {
        /*LOGE("Could not read from PID file.");*/
        goto fail;
    }
    buf[rv] = '\0';

    sscanf(buf, "%d", &pid);
fail:
    if (fd >= 0) {
        close(fd);
    }
    return pid;
}


int pid_check(const char *pidfile)
{
    pid_t pid;

    pid = pid_get(pidfile);

    /* Amazing ! _I_ am already holding the pid file... */
    if ((pid == 0) || (pid == getpid())) {
        return 0;
    }

    /*
    * The 'standard' method of doing this is to try and do a 'fake' kill
    * of the process.  If an ESRCH error is returned the process cannot
    * be found -- GW
    */
    /* But... errno is usually changed only on error.. */
    if ((kill(pid, 0)) && (errno == ESRCH)) {
        return 0;
    }

    return pid;
}

int pid_remove(const char *pidfile)
{
    return unlink(pidfile);
}


int daemonize(const char *pidfile)
{
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        LOGE("Could not fork().");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* Get a new process group */
    if (setsid() < 0) {
        LOGE("Could not setsid().");
        exit(EXIT_FAILURE);
    }

    /* Set file permissions 750 */
    umask(027);

    chdir("/");

    if (pid_check(pidfile) != 0) {
        LOGE("Daemon already running");
        exit(EXIT_FAILURE);
    }

    /* write our own PID to the file */
    if (pid_write(pidfile) != 0) {
        LOGE("Could not write PID file.");
        exit(EXIT_FAILURE);
    }

    return 0;
}
