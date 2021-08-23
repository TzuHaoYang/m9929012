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

#ifndef NTPC_ADJTIME_H
#define NTPC_ADJTIME_H
#define FILE_NTPC_STATUS "/tmp/ntpc_status"
#define FILE_NTPC_CAL_DATA "/tmp/ntpc_cal_data"

typedef enum {
	NTPC_STATUS_INIT = 0,
	NTPC_STATUS_TRIAL,
	NTPC_STATUS_CALIBRATE,
	NTPC_STATUS_MONITOR
} ntpc_status_t;

typedef enum {
	NTPC_REASON_NONE = 0,
	NTPC_REASON_TRIAL_START,
	NTPC_REASON_TRIAL_WAITING,
	NTPC_REASON_CAL_START,
	NTPC_REASON_CAL_ONGOING,
	NTPC_REASON_CAL_DONE,
	NTPC_REASON_CAL_SAVED,
	NTPC_REASON_FAIL
} ntpc_reason_t;

extern void ntpc_get_curr_status(ntpc_status_t *status, int *timer,
	ntpc_reason_t *reason, struct timeval *tv);
extern void ntpc_set_next_status(ntpc_status_t status, int timer,
	ntpc_reason_t reason, struct timeval *tv);
extern int ntpc_get_cal_data(char *tick, char *freq);
extern void ntpc_set_cal_data(long int tick, long int freq);

#endif /* NTPC_ADJTIME_H */
