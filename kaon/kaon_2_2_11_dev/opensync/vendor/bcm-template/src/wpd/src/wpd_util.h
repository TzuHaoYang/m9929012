#ifndef WPD_UTIL_H_INCLUDED
#define WPD_UTIL_H_INCLUDED


#include <unistd.h>
#include <sys/stat.h>

/**
 * Write the PID of the current process to the given file
 */
int pid_write(const char *pidfile);

/**
 * Get the PID from the given file, or 0 on error
 */
pid_t pid_get(const char *pidfile);

/**
 * Read the pid using read_pid and looks up the pid in the process
 * table (using /proc) to determine if the process already exists. If
 * so 1 is returned, otherwise 0.
 */
int pid_check(const char *pidfile);

/**
 * Remove the pidfile.
 */
int pid_remove(const char *pidfile);

/**
 * fork() the program.
 */
int daemonize(const char *pidfile);


#endif /* WPD_UTIL_H_INCLUDED */
