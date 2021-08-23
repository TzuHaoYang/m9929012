#if defined(CONFIG_BCM_OVS_MCAST)
/*
* <:copyright-BRCM:2019:proprietary:standard
* 
*    Copyright (c) 2019 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
:>
*/
#ifndef __MCPD_OVS_H__
#define __MCPD_OVS_H__
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/** Initialize socket for OVS to MCPD Communication
 *
 * @return 0 - Success, -1 Otherwise
 *
 */
int mcpd_ovs_socket_init(void);

/** Accept incoming connection request from OvS v-switchd daemon
 *
 * @return 0 - Success, -1 Otherwise
 *
 */
int mcpd_ovs_socket_accept(void);

/** Receive incoming messages from OvS vswitchd daemon and
 *  process them
 * 
 * @return None
 *
 */
void mcpd_ovs_socket_receive(void);

/** Check if the input bridge is a OvS bridge. If then, return
 *  bridge ifindex.
 *
 * @param *brname      (IN) Bridge name
 * 
 * @param *ret_brIdx   (OUT) Bridge IfIndex
 *
 * @return 1 - Input bridge is OvS Bridge
 *        0 - Input bridge is not a OvS bridge
 *
 */
int mcpd_is_ovs_bridge(char *brname, int *ret_brIdx);

/** Set snoop aging time in OvS snooping table for all
 *  OvS bridges
 *
 * @param agingtimeinsec      (IN) Snoop entry aging time
 * 
 * @return 0 - Success, -1 - Otherwise
 *
 */
int mcpd_ovs_set_snoop_aging_time(int agingtimeinsec);

/** Send message to OvS vswitchd daemon
 *
 * @param *pBuf           (IN) Buffer to send
 *
 * @param buf_size        (IN) Buffer length
 *
 * @return 0 - Success, -1 - Otherwise
 *
 */
int mcpd_ovs_send_msg(void *pBuf, int buf_size);

/** Get all the WAN interfaces on the input OvS bridge
 *
 * @param *brname      (IN) Bridge name
 * 
 * @param *ifindices[] (OUT) Array containing the WAN
 *                           interface indices
 * 
 * @param *num         (OUT) Number of WAN interfaces
 * 
 * @return 0 - Success, -1 - Otherwise
 *
 */
int mcpd_ovs_get_bridge_wan_interfaces(char *brname, 
                                       int ifindices[], 
                                       unsigned int *num);

/** Get all the OvS bridge interfaces
 *
 * @param *ifindices[] (OUT) Array containing the bridge
 *                           interface indices
 * 
 * @param *num         (OUT) Number of bridge interfaces
 * 
 * @return 0 - Success, -1 - Otherwise
 *
 */
int mcpd_ovs_get_bridges(int ifindices[], unsigned int *num);

/** Get all the lower ports on the input OvS bridge
 *
 * @param *brname      (IN) Bridge name
 * 
 * @param *ifindices[] (OUT) Array containing the lower
 *                           port interface indices
 * 
 * @param *num         (OUT) Number of lower port interfaces
 * 
 * @return 0 - Success, -1 - Otherwise
 *
 */
int mcpd_ovs_get_bridge_port_interfaces(char *brName, 
                                        int ifindices[], 
                                        unsigned int *num);

/** Send a Manage(Add/Remove) Snoop Entry message to OvS
 *
 * @param repifi      (IN) IfIndex of Reporter port
 * 
 * @param brifi       (IN) Reporter parent bridge ifindex
 * 
 * @param gpaddr      (IN) IPv4 IGMP Mcast group address
 * 
 * @param *gpaddr6    (IN) IPv6 MLD Mcast group address
 *                         (NULL if IGMP)
 * 
 * @param srcaddr     (IN) IPv4 IGMP Mcast source address
 * 
 * @param *srcaddr6   (IN) IPv6 MLD Mcast source address
 *                         (NULL if IGMP)
 * 
 * @param  addentry   (IN) 1 - Add, 0 - Remove
 *  
 * @return None
 *
 */
void mcpd_ovs_manage_snoop_entry(int repifi, int brifi, 
                                 UINT32 gpaddr, UINT8 *gpaddr6,
                                 UINT32 srcaddr, UINT8 *srcaddr6,
                                 int addentry,
                                 UINT16 lan_tci);
#endif /* __MCPD_OVS_H__ */
#endif /* defined(CONFIG_BCM_OVS_MCAST) */
