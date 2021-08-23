/*
 * ntpc adjtime
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
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: ntpclient_adjtime.c  620256 2016-09-13 09:57:09Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/timex.h>
#include <time.h>
#include <ntpc_adjtime.h>
#include <sys/types.h>
#include <signal.h>

void
ntpc_get_curr_status(ntpc_status_t *status, int *timer,
	ntpc_reason_t *reason, struct timeval *tv)
{
	FILE *fp = NULL;

	*status = NTPC_STATUS_INIT;
	*reason = NTPC_REASON_NONE;
	*timer = 0;
	if ((fp = fopen(FILE_NTPC_STATUS, "r")) != NULL) {
		fscanf(fp, "%d %d %d %lu.%lu\n", (int *)status, timer,
			(int *)reason, &tv->tv_sec, &tv->tv_usec);
		fclose(fp);
	}
	return;
}

void
ntpc_set_next_status(ntpc_status_t status, int timer,
	ntpc_reason_t reason, struct timeval *tv)
{
	FILE *fp = NULL;
	int ret = 0;

	if ((fp = fopen(FILE_NTPC_STATUS, "w")) == NULL) {
		perror("fopen");
		return;
	}

	fprintf(fp, "%d %d %d %lu.%lu\n", status, timer, reason,
		tv->tv_sec, tv->tv_usec);
	fclose(fp);

	if ((status == NTPC_STATUS_CALIBRATE && ((reason == NTPC_REASON_CAL_START) ||
		(reason == NTPC_REASON_FAIL))) ||
		(status == NTPC_STATUS_TRIAL && reason == NTPC_REASON_TRIAL_START) ||
		(status == NTPC_STATUS_MONITOR && ((reason == NTPC_REASON_CAL_DONE) ||
		(reason == NTPC_REASON_FAIL)))) {
		kill(1, SIGALRM);
	}
}

int ntpc_get_cal_data(char *tick, char *freq)
{
	FILE *fp = NULL;

	if ((fp = fopen(FILE_NTPC_CAL_DATA, "r")) != NULL) {
		fscanf(fp, "%s %s", tick, freq);
		fclose(fp);
		return 0;
	}
	return -1;
}

void ntpc_set_cal_data(long int tick, long int freq)
{
	FILE *fp = NULL;

	if ((fp = fopen(FILE_NTPC_CAL_DATA, "w")) == NULL) {
		perror("fopen");
		return;
	}

	fprintf(fp, "%ld %ld", tick, freq);
	fclose(fp);
}
