/*
 * WLC Object Registry API definition
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wlc_objregistry.h 571054 2015-07-14 07:28:41Z $
 */

/**
 * @file
 * @brief
 * Chip/Feature specific enable/disable need to be done for Object registry.
 * Moved object registry related functions from wl/sys to utils folder.
 * A wrapper is provided in this layer/file to enab/disab obj registry for each key
 */

#ifndef _wlc_objregistry_h_
#define _wlc_objregistry_h_

#include <bcm_objregistry.h>

typedef enum obj_registry_key {
	OBJR_INVALID=-1,	/* dummy place holder */
	OBJR_SELF,		/* Must be @ 0 */
	OBJR_MODULE_ID,		/* Stores wlc->modulecb */
	OBJR_BSSCFG_PTR,
	OBJR_CLM_PTR,
	OBJR_AMPDUTX_CONFIG,
	OBJR_AMPDURX_CONFIG,
	OBJR_WLC_CMN_INFO,	/* wlc shared structure */
	OBJR_WLC_PUB_CMN_INFO,	/* wlc pub_cmn_t structure */
	OBJR_PHY_CMN_SROM_INFO, /* sharing phycmn srom structure */
	OBJR_PHY_CMN_INFO,	/* phy shared structure	*/
	OBJR_PHY_CMN_RADAR_INFO, /* phy shared radar structure */
	OBJR_ACPHY_SROM_INFO,	/* sharing acphy srom structure	*/
	OBJR_WLC_BANDSTATE,	/* wlc->bandstate[] */
	OBJR_SCANPUBLIC_CMN, /* Common info for scan public */
	OBJR_SCANPRIV_CMN, /* Common info for scan private */
	OBJR_DEFAULT_BSS, /* Shared default BSS  */
	OBJR_P2P_DATA_PTR,	/* shared p2p data */
	OBJR_ASSOCIATION_INFO, /* Shared association info. */
	OBJR_BSSCFG_CUBBY,
	OBJR_SCB_CUBBY,
	OBJR_BWTE_INFO,	/* BT tunnel engine info */
	OBJR_NAN_CMN_INFO, /* nan module cmn info */
	OBJR_NAN_MAC_CFG,	/* obsolete - nan mac configs */
	OBJR_DFS_SCAN_INFO,	/* DFS scan information. */
	OBJR_SCB_HANDLE_MAP, /* sharing wlc->wlfc_data */
	OBJR_NAN_MAC_INFO,	/* nan mac info */
	OBJR_PFN_CMN_INFO,  /* shared PFN info */
	OBJR_NAN_DISC_INFO,  /* nan disc info */
	OBJR_TDLS_CMN_INFO,
	OBJR_SHUB_CMN_INFO,	/* shub common info */
	OBJR_TBOW_INFO,	/* tbow info */
	OBJR_ASDB_INFO, /* ASDB info */
	OBJR_WL_CMN_INFO,	/* wl common info */
	OBJR_MAX_KEYS
} obj_registry_key_t;

typedef struct wlc_obj_registry wlc_obj_registry_t;

#ifdef WL_OBJ_REGISTRY

/* === Object Registry API === */

/* Create the registry only once in wl_xxx.c per port layer and pass it to wlc_attach()
 * Each wlc_attach() creates a new WLC instance that shares the same objr instance
 */
wlc_obj_registry_t* obj_registry_alloc(osl_t *osh, int count);

/* Destroy the registry at the end, after all instances of WLC are freed-up */
void obj_registry_free(wlc_obj_registry_t *wlc_objr, osl_t *osh);

/* obj_registry_set() is used to setup the value for key.
 * It simply overwrites the existing value if any
 * returns, BCME_OK on success
 * returns, BCME_RANGE if key exceeds, max limit
 */
int obj_registry_set(wlc_obj_registry_t *wlc_objr, obj_registry_key_t key, void *value);

/* obj_registry_get() is used to get the value for key.
 * return of NULL, indicates key is unused / invalid
 *
 */
void * obj_registry_get(wlc_obj_registry_t *wlc_objr, obj_registry_key_t key);

/* Ref counting on registry objects is provided for users to keep track of ref counts */

/*
 * Typical call sequence will be as follows:
 * 	Step (1). check if the registry has a value for key 'KEY_X'
 * 	Step (2). if it has value, go to Step (4)
 * 	Step (3). registry has no value, so allocate and store value for 'KEY_X'
 *	Step (4). reference the stored value for 'KEY_X'
 */

/* obj_registy_ref() is used to increment ref_cnt associated with 'key'
 * If there is no value stored, reference is not incremented.
 */
int obj_registry_ref(wlc_obj_registry_t *wlc_objr, obj_registry_key_t key);

/* obj_registry_unref() is used to decrement ref_cnt associated with 'key'
 * Decrements the reference count for each call.
 * If there is no value stored, reference is not decremented.
 */
int obj_registry_unref(wlc_obj_registry_t *wlc_objr, obj_registry_key_t key);

int obj_registry_islast(wlc_obj_registry_t *wlc_objr);

#if defined(BCMDBG)
/* Debug function to dump out all contents of the registry */
int wlc_dump_objr(wlc_obj_registry_t *wlc_objr, struct bcmstrbuf *b);
#endif // endif

#else /* WL_OBJ_REGISTRY */

#define obj_registry_alloc(o, c) (NULL)
#define obj_registry_free(d, o)
#define obj_registry_set(d, k, v)
#define obj_registry_get(d, k) (NULL)
#define obj_registry_ref(d, k) (1)
#define obj_registry_unref(d, k) (0)
#define obj_registry_islast(d) (1)

#endif /* WL_OBJ_REGISTRY */

#endif /* _wlc_objregistry_h_ */
