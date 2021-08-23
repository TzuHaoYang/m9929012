/*
 * DHD Monitor Daemon
 *
 * Copyright (C) 2019, Broadcom. All Rights Reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: dhd_monitor.c 735326 2017-12-08 07:56:41Z $
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <shutils.h>
#include <confmtd_utils.h>
#if defined(BCA_HNDROUTER) || defined(DSLCPE)
#include <sys/utsname.h> /* for uname */
#endif /* BCA_HNDROUTER */

/* defines */
#ifndef RAMFS_CONFMTD_DIR
#define RAMFS_CONFMTD_DIR	"/tmp/"
#endif // endif

/* Base Path /tmp/confmtd/crash_logs - or - /tmp/media/nand/crash_logs */
#define	LOG_BASE_PATH			RAMFS_CONFMTD_DIR"/crash_logs"
#define	LOG_UNAME_FILENAME		"uname.txt"
#define	LOG_DMESG_FILENAME		"dmesg.txt"
#define	LOG_DONGLE_MEM_FILENAME		"dongle_mem_dump.bin"
#define IF_IDX_FILE_PATH		"/tmp/failed_if.txt"
#define LOG_D11REGDUMP_FILENAME		"d11reg_dump.txt"
#define LOG_D11REGXDUMP_FILENAME	"d11regx_dump.txt"
#define LOG_SOCRAM_DUMP			"socram_dump.txt"
#define LOG_SCTPLDUMP_FILENAME		"sctpl_dump.txt"
#define LOG_SCTPLXDUMP_FILENAME		"sctplx_dump.txt"
#define LOG_VASIPDUMP_FILENAME		"vasip_dump.txt"

#if defined(DSLCPE)
#define rc_stop()		//Or should we stop WLMNGR ?
#define rc_start()		system("nvram restart")
#define rc_restart()		system("reboot")
#define rc_reboot()		system("reboot")
#else
#define rc_stop()		kill(1, SIGINT)
#define rc_start()		kill(1, SIGUSR2)
#define rc_restart()		kill(1, SIGHUP)
#define rc_reboot()		kill(1, SIGTERM)
#endif

/* Number of crash logs to retain */
#define MAX_CRASH_LOGS   3

char *_mod_name = NULL;

static void get_timestamp(char *buffer, int max_len)
{
	time_t rawtime;
	struct tm *info;

	time(&rawtime);
	info = localtime(&rawtime);
	strftime(buffer, max_len, "%F_%T", info);
	return;
}

static void handle_recovery(void)
{
	char *val = (char *) nvram_get("fast_restart");
#if defined(BCA_HNDROUTER) || defined(DSLCPE)
	struct utsname name;
	char buf[PATH_MAX];
#endif /* BCA_HNDROUTER */

	if (val && atoi(val)) {
		/* Fast Restart - rc_restart */
		/* stop all services */
		printf("%s: stop services\n", _mod_name);
		rc_stop();

		/* rc_stop is a non-blocking call. So, add sufficient sleep
		 * to make sure all services are stopped before proceeding.
		 */
		sleep(3);

		/* unload dhd */
		printf("%s: unload dhd\n", _mod_name);
		eval("rmmod", "dhd");

		/* reload dhd */
		printf("%s: reload dhd\n", _mod_name);
#if defined(BCA_HNDROUTER)
		uname(&name);
		snprintf(buf, sizeof(buf), "/lib/modules/%s/extra/dhd.ko", name.release);
		eval("insmod", buf);
#elif defined(DSLCPE)
		uname(&name);
		snprintf(buf, sizeof(buf), "insmod /lib/modules/%s/extra/dhd.ko firmware_path=/etc/wlan/dhd mfg_firmware_path=/etc/wlan/dhd/mfg  dhd_dpc_prio=5  iface_name=wl  dhd_console_ms=0"
		, name.release);
		system(buf);
#else
		eval("insmod", "dhd.ko");
#endif /* ! BCA_HNDROUTER */

		/* start all services */
		printf("%s: restart services\n", _mod_name);
		rc_start();
	} else {
		/* Full restart - reboot */
		val = (char *) nvram_get("watchdog");
		if (val && atoi(val)) {
			/* full reboot */
			printf("%s: rebooting system\n", _mod_name);
			rc_reboot();
		} else {
			printf("%s: watchdog disabled, ignoring dongle trap\n", _mod_name);
		}
	}
}

/* Filter out "." & ".." directory entries */
static int dir_filter(const struct dirent *dptr)
{
	if ((strcmp(dptr->d_name, ".") == 0) || (strcmp(dptr->d_name, "..") == 0))
		return 0;

	return 1;
}

/* Remove any extra crash logs that might have accumulated over time */
/* Prune the oldest crash logs */
static int delete_extra_crash_files()
{
	struct dirent **fnamelist;
	int n;
	char filepath[256];

	n = scandir(LOG_BASE_PATH, &fnamelist, dir_filter, alphasort);
	if (n < 0) {
		perror("scandir");
		printf("%s: could not scan %s folder \n", _mod_name, LOG_BASE_PATH);
	} else {
		n = n - MAX_CRASH_LOGS;
		while (n-- > 0) {
			printf("%s: pruning old crash log - %s\n", _mod_name, fnamelist[n]->d_name);
			snprintf(filepath, sizeof(filepath), "%s/%s",
				LOG_BASE_PATH, fnamelist[n]->d_name);
			unlink(filepath);
			free(fnamelist[n]);
		}
		free(fnamelist);
	}

	return 0;
}

static int backup_logs(void)
{
	if (confmtd_backup() == 0) {
		return 0;
	} else {
		printf("%s:%s: failed\n", _mod_name, __FUNCTION__);
		return -1;
	}
}

static void capture_logs(int noclk)
{
	char timestamp[64];
	char basepath[128];
	char filepath[256];
	char command[256];
	struct stat file_stat;
	FILE *fp;
	char if_name[32];
	int ch, nch = 0;
	int error = 0;
	char *tmp;

	/* Get the interface name */
	fp = fopen(IF_IDX_FILE_PATH, "r");
	if (!fp) {
		printf("%s: cannot open file %s\n", __FUNCTION__, IF_IDX_FILE_PATH);
		error = -1;
	} else {
		while ((ch = fgetc(fp)) != '\n' && ch != EOF) {
			if_name[nch++] = ch;
		}
		if_name[nch] = '\0';
		fclose(fp);
	}

	if (!error) {
		printf("%s: Trap/Assert on interface %s!!, noclk %d\n",
			_mod_name, if_name, noclk);
	}

	/* get current timestamp */
	get_timestamp(timestamp, sizeof(timestamp));

	/* reformatting time stamp string */
	tmp = timestamp;
	while (*tmp) {
		if (*tmp == '-' || *tmp == ':') {
			*tmp = '_';
		}
		tmp++;
	}

	/* create the output directory for the logs */
	snprintf(basepath, sizeof(basepath), "%s/%s", LOG_BASE_PATH, timestamp);
	if (mkdir(RAMFS_CONFMTD_DIR, 0777) < 0 && errno != EEXIST) {
		perror("could not create dhd log folder");
		printf("%s: could not create %s folder\n", _mod_name, RAMFS_CONFMTD_DIR);
		return;
	}
	if (mkdir(LOG_BASE_PATH, 0777) < 0 && errno != EEXIST) {
		perror("could not create dhd log folder");
		printf("%s: could not create %s folder\n", _mod_name, LOG_BASE_PATH);
		return;
	}
	if (mkdir(basepath, 0777) < 0 && errno != EEXIST) {
		perror("could not create dhd log folder");
		printf("%s: could not create %s folder\n", _mod_name, basepath);
		return;
	}

	printf("Logging timestamp: %s\n", timestamp);

	/* dump version */
	snprintf(command, sizeof(command), "uname -a > %s/%s", basepath, LOG_UNAME_FILENAME);
	system(command);

	/* copy the dongle mem dump if available */
	if (stat("/tmp/mem_dump", &file_stat) == 0) {
		snprintf(filepath, sizeof(filepath), "%s/%s", basepath, LOG_DONGLE_MEM_FILENAME);
		eval("mv", "/tmp/mem_dump", filepath);
	} else {
		printf("%s: dongle memory dump is not available\n", _mod_name);
	}

	/* dump dmesg to command */
	snprintf(command, sizeof(command), "dmesg -c > %s/%s", basepath, LOG_DMESG_FILENAME);
	system(command);

	if (!error) {
		/* Make sure dongle is dead */
		snprintf(command, sizeof(command), "dhd -i %s pcie_device_trap",
			if_name);
		system(command);

		if (noclk)
			goto bypass_dump;

		/* dump PSMr regs */
		snprintf(command, sizeof(command), "dhd -i %s dump_mac > %s/%s",
			if_name, basepath, LOG_D11REGDUMP_FILENAME);
		system(command);

		/* dump PSMx regs */
		snprintf(command, sizeof(command), "dhd -i %s dump_mac -x > %s/%s",
			if_name, basepath, LOG_D11REGXDUMP_FILENAME);
		system(command);

		/* dump sample capture log */
		snprintf(command, sizeof(command), "dhd -i %s dump_sctpl > %s/%s",
			if_name, basepath, LOG_SCTPLDUMP_FILENAME);
		system(command);

		/* dump SHMx sample capture log */
		snprintf(command, sizeof(command), "dhd -i %s dump_sctpl -u shmx > %s/%s",
			if_name, basepath, LOG_SCTPLXDUMP_FILENAME);
		system(command);

		/* dump vasip dump log */
		snprintf(command, sizeof(command), "dhd -i %s dump_svmp > %s/%s",
			if_name, basepath, LOG_VASIPDUMP_FILENAME);
		system(command);

bypass_dump:
		/* SOCRAM dump to socram_dump.txt */
		snprintf(command, sizeof(command), "dhd -i %s upload %s/%s",
			if_name, basepath, LOG_SOCRAM_DUMP);
		system(command);
	}

	/* tar/zip the logs and memory dump */
	snprintf(command, sizeof(command), "tar cf - %s -C %s | gzip -c > %s.tgz",
		timestamp, LOG_BASE_PATH, basepath);
	system(command);

	/* delete the raw folder */
	snprintf(command, sizeof(command), "rm -rf %s", basepath);
	system(command);

	snprintf(filepath, sizeof(filepath), "%s.tgz", basepath);
	if ((stat(filepath, &file_stat) == 0) && file_stat.st_size) {
		printf("%s: log (%s) is collected, file size %d\n", _mod_name,
			filepath, file_stat.st_size);
	} else {
		printf("%s: log (%s) is not available or an empty file\n", _mod_name, filepath);
	}
}

static void sig_handler(int signo)
{
	printf("%s: Detected firmware trap/assert !!\n", _mod_name);

	/* capture all relevant logs */
	capture_logs((signo == SIGUSR2 ? 1 : 0));

	/* retain latest crash logs */
	delete_extra_crash_files();

	/* back up the logs to persistent store */
	backup_logs();

	/* reboot or rc_restart based on configuration */
	handle_recovery();
}

int main(int argc, char **argv)
{
	_mod_name = argv[0];

#if defined(DSLCPE)    
	/* Create necessary direction for log */
	eval("mkdir", "-p",LOG_BASE_PATH);
#endif

	if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
		printf("%s: error setting up signal1 handler.\n", _mod_name);
		return -1;
	}
	if (signal(SIGUSR2, sig_handler) == SIG_ERR) {
		printf("%s: error setting up signal2 handler.\n", _mod_name);
		return -1;
	}

	/* Delete any old crash files that may have not got deleted */
	delete_extra_crash_files();

	if (daemon(1, 1) == -1) {
		printf("%s: error daemonizing.\n", _mod_name);
	} else {
		while (1) {
			sleep(1);
		}
	}
	return 0;
}
