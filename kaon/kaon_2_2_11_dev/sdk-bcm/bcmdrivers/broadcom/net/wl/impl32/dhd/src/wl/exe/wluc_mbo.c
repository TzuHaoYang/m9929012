/*
 * wl mbo command module
 *
 * Copyright 2019 Broadcom
 *
 * This program is the proprietary software of Broadcom and/or
 * its licensors, and may only be used, duplicated, modified or distributed
 * pursuant to the terms and conditions of a separate, written license
 * agreement executed between you and Broadcom (an "Authorized License").
 * Except as set forth in an Authorized License, Broadcom grants no license
 * (express or implied), right to use, or waiver of any kind with respect to
 * the Software, and Broadcom expressly reserves all rights in and to the
 * Software and all intellectual property rights therein.  IF YOU HAVE NO
 * AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 * WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 * THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1. This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof, and to
 * use this information only in connection with your use of Broadcom
 * integrated circuit products.
 *
 * 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 * "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 * OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 * SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 * IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 * IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 * ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 * OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 * NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wluc_mbo.c 684714 2017-02-14 07:45:24Z $
 */
#include <bcmendian.h>
#include "wlu_common.h"
#include "wlu.h"
#include <wlioctl.h>

/* from bcmiov.h: non-batched command version = major|minor w/ major <= 127 */
#define WL_MBO_IOV_MAJOR_VER 1
#define WL_MBO_IOV_MINOR_VER 1
#define WL_MBO_IOV_MAJOR_VER_SHIFT 8
#define WL_MBO_IOV_VERSION \
	((WL_MBO_IOV_MAJOR_VER << WL_MBO_IOV_MAJOR_VER_SHIFT)| WL_MBO_IOV_MINOR_VER)
#define WL_MBO_CELLULAR_DATA_AWARE_AP	((0X01) << 6)
#define MBO_ASSOC_DISALLOWED_DISABLE				0x00
#define MBO_ASSOC_DISALLOWED_REASON_UNSPECIFIED			0x01
#define MBO_ASSOC_DISALLOWED_REASON_MAX_STA_LIMIT_REACHED	0x02
#define MBO_ASSOC_DISALLOWED_REASON_AIR_INTERFACE_OVERLOAD	0x03
#define MBO_ASSOC_DISALLOWED_REASON_AUTH_SERVER_OVERLOAD	0x04
#define MBO_ASSOC_DISALLOWED_REASON_INSUFFICIENT_RSSI		0x05

/* Forward declarations */
typedef uint16 bcm_iov_cmd_id_t;
typedef struct bcm_iov_buf bcm_iov_buf_t;

/* non-batched command version = major|minor w/ major <= 127 */
struct bcm_iov_buf {
	uint16 version;
	uint16 len;
	bcm_iov_cmd_id_t id;
	uint16 data[1]; /* 32 bit alignment may be repurposed by the command */
	/* command specific data follows */
};

typedef struct wl_mbo_sub_cmd wl_mbo_sub_cmd_t;
typedef int (subcmd_handler_t)(void *wl, const wl_mbo_sub_cmd_t *cmd,
	char **argv);
typedef void (help_handler_t) (void);
struct wl_mbo_sub_cmd {
	char *name;
	uint8 version;
	uint16 cmd_id;
	uint16 type;
	subcmd_handler_t *hndler;
	help_handler_t *help;
};

static cmd_func_t wl_mbo_main;

static cmd_t wl_mbo_cmds[] = {
	{ "mbo", wl_mbo_main, WLC_GET_VAR, WLC_SET_VAR,
	"Please follow usage shown above\n"},
	{ NULL, NULL, 0, 0, NULL }
};

/* command handlers */
static subcmd_handler_t wl_mbo_sub_cmd_ap_attr;
static subcmd_handler_t wl_mbo_sub_cmd_ap_assoc_disallowed;
static subcmd_handler_t wl_mbo_sub_cmd_ap_enab;

/* help handlers */
static help_handler_t wl_mbo_ap_attr_help_fn;
static help_handler_t wl_mbo_ap_assoc_disallowed_help_fn;
static help_handler_t wl_mbo_ap_enab_help_fn;

#define WL_MBO_CMD_ALL 0
static const wl_mbo_sub_cmd_t mbo_subcmd_lists[] = {
	{ "ap_attr", 0x1, WL_MBO_CMD_AP_ATTRIBUTE,
	IOVT_BUFFER, wl_mbo_sub_cmd_ap_attr,
	wl_mbo_ap_attr_help_fn
	},
	{ "ap_assoc_disallowed", 0x1, WL_MBO_CMD_AP_ASSOC_DISALLOWED,
	IOVT_BUFFER, wl_mbo_sub_cmd_ap_assoc_disallowed,
	wl_mbo_ap_assoc_disallowed_help_fn
	},
	{ "ap_enable", 0x1, WL_MBO_CMD_AP_ENAB,
	IOVT_BUFFER, wl_mbo_sub_cmd_ap_enab,
	wl_mbo_ap_enab_help_fn
	},
	{ NULL, 0, 0, 0, NULL, NULL }
};

void wluc_mbo_module_init(void)
{
	/* register mbo commands */
	wl_module_cmds_register(wl_mbo_cmds);
}

static void
wl_mbo_usage(int cmd_id)
{
	const wl_mbo_sub_cmd_t *subcmd = &mbo_subcmd_lists[0];

	if (cmd_id > (WL_MBO_CMD_LAST - 1)) {
		return;
	}
	while (subcmd->name) {
		if (cmd_id == WL_MBO_CMD_ALL) {
			subcmd->help();
		} else if (cmd_id == subcmd->cmd_id) {
			subcmd->help();
		} else {
			/* do nothing */
		}
		subcmd++;
	}
	return;
}

static int
wl_mbo_main(void *wl, cmd_t *cmd, char **argv)
{
	int ret = BCME_OK;
	const wl_mbo_sub_cmd_t *subcmd = &mbo_subcmd_lists[0];

	UNUSED_PARAMETER(cmd);
	/* skip to command name */
	argv++;
	if (*argv) {
		if (!strcmp(*argv, "-h") || !strcmp(*argv, "help")) {
			wl_mbo_usage(WL_MBO_CMD_ALL);
			return BCME_OK;
		}
		while (subcmd->name) {
			if (strcmp(subcmd->name, *argv) == 0) {
				if (subcmd->hndler) {
					ret = subcmd->hndler(wl, subcmd, ++argv);
					return ret;
				}
			}
			subcmd++;
		}
	} else {
		wl_mbo_usage(WL_MBO_CMD_ALL);
		return BCME_USAGE_ERROR;
	}

	return ret;
}

static int
wl_mbo_process_iov_resp_buf(void *iov_resp, uint16 cmd_id, bcm_xtlv_unpack_cbfn_t cbfn)
{
	int ret = BCME_OK;
	uint16 version;

	/* Check for version */
	version = dtoh16(*(uint16 *)iov_resp);
	if (version != WL_MBO_IOV_VERSION) {
		ret = BCME_UNSUPPORTED;
	}
	bcm_iov_buf_t *p_resp = (bcm_iov_buf_t *)iov_resp;
	if (p_resp->id == cmd_id && cbfn != NULL) {
		ret = bcm_unpack_xtlv_buf((void *)p_resp, (uint8 *)p_resp->data, p_resp->len,
			BCM_XTLV_OPTION_ALIGN32, cbfn);
	}
	return ret;
}

static int
wl_mbo_get_iov_resp(void *wl, const wl_mbo_sub_cmd_t *cmd, bcm_xtlv_unpack_cbfn_t cbfn)
{
	int ret = BCME_OK;
	bcm_iov_buf_t *iov_buf = NULL;
	uint8 *iov_resp = NULL;

	UNUSED_PARAMETER(wl);
	iov_buf = (bcm_iov_buf_t *)calloc(1, WLC_IOCTL_SMLEN);
	if (iov_buf == NULL) {
		ret = BCME_NOMEM;
		goto fail;
	}
	iov_resp = (uint8 *)calloc(1, WLC_IOCTL_MAXLEN);
	if (iov_resp == NULL) {
		ret = BCME_NOMEM;
		goto fail;
	}
	/* fill header */
	iov_buf->version = WL_MBO_IOV_VERSION;
	iov_buf->id = cmd->cmd_id;

	ret = wlu_iovar_getbuf(wl, "mbo", iov_buf, WLC_IOCTL_SMLEN, iov_resp, WLC_IOCTL_MAXLEN);
	if ((ret == BCME_OK) && (iov_resp != NULL)) {
		wl_mbo_process_iov_resp_buf(iov_resp, cmd->cmd_id, cbfn);
	}
fail:
	if (iov_buf) {
		free(iov_buf);
	}
	if (iov_resp) {
		free(iov_resp);
	}
	return ret;
}

static int
wl_mbo_ap_attr_cbfn(void *ctx, uint8 *data, uint16 type, uint16 len)
{
	UNUSED_PARAMETER(ctx);
	UNUSED_PARAMETER(len);
	if (data == NULL) {
		printf("%s: Bad argument !!\n", __FUNCTION__);
		return BCME_BADARG;
	}
	switch (type) {
		case WL_MBO_XTLV_AP_ATTR:
				printf("ap attribute : %0x\n", *data);
			break;
		default:
			printf("%s: Unknown tlv %u\n", __FUNCTION__, type);
	}
	return BCME_OK;
}

static int
wl_mbo_ap_enab_cbfn(void *ctx, uint8 *data, uint16 type, uint16 len)
{
	UNUSED_PARAMETER(ctx);
	UNUSED_PARAMETER(len);
	if (data == NULL) {
		printf("%s: Bad argument !!\n", __FUNCTION__);
		return BCME_BADARG;
	}
	switch (type) {
		case WL_MBO_XTLV_AP_ENAB:
				printf("MBO AP ENABLE : %0x\n", *data);
			break;
		default:
			printf("%s: Unknown tlv %u\n", __FUNCTION__, type);
	}
	return BCME_OK;
}

static int
wl_mbo_ap_assoc_disallowed_cbfn(void *ctx, uint8 *data, uint16 type, uint16 len)
{
	UNUSED_PARAMETER(ctx);
	UNUSED_PARAMETER(len);
	if (data == NULL) {
		printf("%s: Bad argument !!\n", __FUNCTION__);
		return BCME_BADARG;
	}
	switch (type) {
		case WL_MBO_XTLV_AP_ASSOC_DISALLOWED:
				printf("ap attribute assoc disallowed : %0x\n", *data);
			break;
		default:
			printf("%s: Unknown tlv %u\n", __FUNCTION__, type);
	}
	return BCME_OK;
}

static void
wl_mbo_ap_attr_help_fn(void)
{
	printf("Possible values for mbo ap_attr iovars are: \n");
	printf("Reserved bit(0 - 5) i.e. 0x01 - 0x20 \n");
	printf("Set bit 6 for cellular data aware i.e 0x40 \n");
	printf("Reserved bit(7) i.e 0x80 \n");
}

static void
wl_mbo_ap_enab_help_fn(void)
{
	printf("Only get /set mbo ap feature options \n");
	printf("usage: to enable/disable : wl -i <interface> mbo ap_enable <1/0> \n");
	printf("usage: get mbo ap feature : wl -i <interface> mbo ap_enable\n");
}

static void
wl_mbo_ap_assoc_disallowed_help_fn(void)
{
	printf("Possible values for mbo ap_assoc_disallowed iovars are: \n");
	printf(" 1 - Reason unspecified \n");
	printf(" 2 - Max sta limit reached \n");
	printf(" 3 - Air interface overload \n");
	printf(" 4 - Auth server overload \n");
	printf(" 5 - Insufficient RSSI \n");
}

static int
wl_mbo_sub_cmd_ap_attr(void *wl, const wl_mbo_sub_cmd_t *cmd, char **argv)
{
	int ret = BCME_OK;
	bcm_iov_buf_t *iov_buf = NULL;
	uint8 *pxtlv = NULL;
	uint16 buflen = 0, buflen_start = 0;
	char *val_p = NULL;
	uint16 iovlen = 0;
	uint8 ap_attr;

	/* get */
	if (*argv == NULL) {
		ret = wl_mbo_get_iov_resp(wl, cmd, wl_mbo_ap_attr_cbfn);
	} else {
		iov_buf = (bcm_iov_buf_t *)calloc(1, WLC_IOCTL_MEDLEN);
		if (iov_buf == NULL) {
			ret = BCME_NOMEM;
			goto fail;
		}
		/* fill header */
		iov_buf->version = WL_MBO_IOV_VERSION;
		iov_buf->id = cmd->cmd_id;

		pxtlv = (uint8 *)&iov_buf->data[0];
		val_p = *argv;
		if (val_p == NULL) {
			wl_mbo_usage(WL_MBO_CMD_AP_ATTRIBUTE);
			ret = BCME_USAGE_ERROR;
			goto fail;
		}
		ap_attr = strtoul(val_p, NULL, 0);
		if ((ap_attr == WL_MBO_CELLULAR_DATA_AWARE_AP)) {
			buflen = buflen_start = WLC_IOCTL_MEDLEN - sizeof(bcm_iov_buf_t);
			ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_MBO_XTLV_AP_ATTR,
				sizeof(ap_attr), &ap_attr, BCM_XTLV_OPTION_ALIGN32);
			if (ret != BCME_OK) {
				goto fail;
			}
			iov_buf->len = buflen_start - buflen;
			iovlen = sizeof(bcm_iov_buf_t) + iov_buf->len;
			ret = wlu_var_setbuf(wl, "mbo", (void *)iov_buf, iovlen);
		} else {
			wl_mbo_usage(WL_MBO_CMD_AP_ATTRIBUTE);
			ret = BCME_USAGE_ERROR;
			goto fail;
		}
	}
fail:
	if (iov_buf) {
		free(iov_buf);
	}
	return ret;
}

static int
wl_mbo_sub_cmd_ap_assoc_disallowed(void *wl, const wl_mbo_sub_cmd_t *cmd, char **argv)
{
	int ret = BCME_OK;
	bcm_iov_buf_t *iov_buf = NULL;
	uint8 *pxtlv = NULL;
	uint16 buflen = 0, buflen_start = 0;
	char *val_p = NULL;
	uint16 iovlen = 0;
	uint8 ap_assoc_disallowed;

	/* get */
	if (*argv == NULL) {
		ret = wl_mbo_get_iov_resp(wl, cmd, wl_mbo_ap_assoc_disallowed_cbfn);
	} else {
		iov_buf = (bcm_iov_buf_t *)calloc(1, WLC_IOCTL_MEDLEN);
		if (iov_buf == NULL) {
			ret = BCME_NOMEM;
			goto fail;
		}
		/* fill header */
		iov_buf->version = WL_MBO_IOV_VERSION;
		iov_buf->id = cmd->cmd_id;

		pxtlv = (uint8 *)&iov_buf->data[0];
		val_p = *argv;
		if (val_p == NULL) {
			wl_mbo_usage(WL_MBO_CMD_AP_ASSOC_DISALLOWED);
			ret = BCME_USAGE_ERROR;
			goto fail;
		}
		ap_assoc_disallowed = strtoul(val_p, NULL, 0);
		/* MBO standard possible values:
		 * 0: use to Disable from IOVAR.
		 * 1: Unspecified reason
		 * 2: Max number of sta association reahed
		 * 3: Air interface is overloaded
		 * 4: Authentication server overloaded
		 * 5: Insufficient RSSI
		 * 6 - 255: Reserved
		 */
		buflen = buflen_start = WLC_IOCTL_MEDLEN - sizeof(bcm_iov_buf_t);
		ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_MBO_XTLV_AP_ASSOC_DISALLOWED,
				sizeof(ap_assoc_disallowed), &ap_assoc_disallowed,
				BCM_XTLV_OPTION_ALIGN32);
		if (ret != BCME_OK) {
			goto fail;
		}
		iov_buf->len = buflen_start - buflen;
		iovlen = sizeof(bcm_iov_buf_t) + iov_buf->len;
		ret = wlu_var_setbuf(wl, "mbo", (void *)iov_buf, iovlen);
	}
fail:
	if (iov_buf) {
		free(iov_buf);
	}
	return ret;
}

static int
wl_mbo_sub_cmd_ap_enab(void *wl, const wl_mbo_sub_cmd_t *cmd, char **argv)
{
	int ret = BCME_OK;
	bcm_iov_buf_t *iov_buf = NULL;
	uint8 *pxtlv = NULL;
	uint16 buflen = 0, buflen_start = 0;
	char *val_p = NULL;
	uint16 iovlen = 0;
	uint8 mbo_ap_enable;

	/* get */
	if (*argv == NULL) {
		ret = wl_mbo_get_iov_resp(wl, cmd, wl_mbo_ap_enab_cbfn);
	} else {
		iov_buf = (bcm_iov_buf_t *)calloc(1, WLC_IOCTL_MEDLEN);
		if (iov_buf == NULL) {
			ret = BCME_NOMEM;
			goto fail;
		}
		/* fill header */
		iov_buf->version = WL_MBO_IOV_VERSION;
		iov_buf->id = cmd->cmd_id;

		pxtlv = (uint8 *)&iov_buf->data[0];
		val_p = *argv;
		if (val_p == NULL) {
			wl_mbo_usage(WL_MBO_CMD_AP_ENAB);
			ret = BCME_USAGE_ERROR;
			goto fail;
		}
		mbo_ap_enable = strtoul(val_p, NULL, 0);
		buflen = buflen_start = WLC_IOCTL_MEDLEN - sizeof(bcm_iov_buf_t);
		ret = bcm_pack_xtlv_entry(&pxtlv, &buflen, WL_MBO_XTLV_AP_ENAB,
				sizeof(mbo_ap_enable), &mbo_ap_enable,
				BCM_XTLV_OPTION_ALIGN32);
		if (ret != BCME_OK) {
			goto fail;
		}
		iov_buf->len = buflen_start - buflen;
		iovlen = sizeof(bcm_iov_buf_t) + iov_buf->len;
		ret = wlu_var_setbuf(wl, "mbo", (void *)iov_buf, iovlen);
	}
fail:
	if (iov_buf) {
		free(iov_buf);
	}
	return ret;
}
