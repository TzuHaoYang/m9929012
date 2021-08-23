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
#include <fcntl.h>
#include <errno.h>
#include <linux/un.h>
#include <bcm_local_kernel_include/linux/sockios.h>
#include <sys/ioctl.h>
#include "bcm_mcast_api.h"
#include "mcpd.h"
#include "mcpd_ovs.h"
#include "igmp_main.h"
#include "mld_main.h"
#include "common.h"

extern t_MCPD_ROUTER mcpd_router;

/** Initialize socket for OVS vswitchd daemon to MCPD
 *  Communication
 *
 * @return 0 - Success, -1 Otherwise
 *
 */
int mcpd_ovs_socket_init(void)
{
    struct sockaddr_in sa;
    int             sd;
    int             flags;
    int             optval = 1;
    socklen_t       optlen = sizeof(optval);
  
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        MCPD_TRACE(MCPD_TRC_ERR, "socket() error, %s", strerror(errno));
        return -1;
    }

    /* Allow reusing the socket immediately when application is restarted */
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen))
    {
        MCPD_TRACE(MCPD_TRC_INFO, "setsockopt error %s", strerror(errno));
    }

    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_port        = htons((unsigned short)OVS_2_MCPD_SOCK_PORT);
    if((bind(sd, (struct sockaddr *)&sa, sizeof(sa))) == -1)
    {
        MCPD_TRACE(MCPD_TRC_ERR, "bind() to port %d error, %s", 
                   OVS_2_MCPD_SOCK_PORT, strerror(errno));
        close(sd);
        return -1;
    }

    if ((listen(sd, 3)) == -1)
    {
        MCPD_TRACE(MCPD_TRC_ERR, "listen() to port %d error, %s", 
                   OVS_2_MCPD_SOCK_PORT, strerror(errno));
        close(sd);
        return -1;
    }

    flags = fcntl(sd, F_GETFL, 0);
    if(flags < 0)
    {
        MCPD_TRACE(MCPD_TRC_INFO, "cannot retrieve socket flags. error=%s", 
                   strerror(errno));
    }
    if ( fcntl(sd, F_SETFL, flags | O_NONBLOCK) < 0 )
    {
        MCPD_TRACE(MCPD_TRC_INFO, "cannot set socket to non-blocking. error=%s", 
                   strerror(errno));
    }

    mcpd_router.ovs_info.sock_ovs = sd;
    return(0);
}

/** Accept incoming connection request from OvS v-switchd daemon
 *
 * @return 0 - Success, -1 Otherwise
 *
 */
int mcpd_ovs_socket_accept(void)
{
    struct sockaddr_un clientAddr;
    unsigned int sockAddrSize;
    int sd;
    int flags;

    if ( mcpd_router.ovs_info.sock_ovs_con != -1 )
    {
        MCPD_TRACE(MCPD_TRC_INFO, "Only one connection available");
        return -1;
    }

    sockAddrSize = sizeof(clientAddr);
    if ((sd = accept(mcpd_router.ovs_info.sock_ovs, (struct sockaddr *)&clientAddr, &sockAddrSize)) < 0) {
        MCPD_TRACE(MCPD_TRC_ERR, "accept connection failed. errno=%d", errno);
        return -1;
    }
  
    flags = fcntl(sd, F_GETFL, 0);
    if(flags < 0) {
        MCPD_TRACE(MCPD_TRC_INFO, "cannot retrieve socket flags. errno=%d", errno);
    }
    if ( fcntl(sd, F_SETFL, flags | O_NONBLOCK) < 0 ) {
        MCPD_TRACE(MCPD_TRC_INFO, "cannot set socket to non-blocking. errno=%d", errno);
    }
    mcpd_router.ovs_info.sock_ovs_con = sd;
    return 0;
}

/** Send message to OvS vswitchd daemon
 *
 * @param *pBuf           (IN) Buffer to send
 *
 * @param buf_size        (IN) Buffer length
 *
 * @return 0 - Success, -1 - Otherwise
 *
 */
int mcpd_ovs_send_msg(void *pBuf, int buf_size)
{
    return bcm_mcast_api_send_sock_msg(MCPD_2_OVS_SOCK_PORT, pBuf, buf_size);
}

/** Process bridge configuration update message from OvS.
 *  Forward the update to Multicast driver
 *
 * @param *pBrCfgMsg      (IN) Msg structure with Bridge/port
 *                             info
 *
 * @return 0 - Success, -1 - Otherwise
 *
 */
int mcpd_ovs_brcfg_update(t_ovs_mcpd_brcfg_info *pBrCfgMsg)
{
    int brIdx, portIdx;
    t_ovs_mcpd_brcfg_info *pMcpdBrcfg = &(mcpd_router.ovs_info.brcfg); 

    pMcpdBrcfg->numbrs = pBrCfgMsg->numbrs;
    for (brIdx = 0; brIdx < pBrCfgMsg->numbrs; brIdx++) 
    {
        pMcpdBrcfg->numports[brIdx] = pBrCfgMsg->numports[brIdx];
        pMcpdBrcfg->ovs_br[brIdx] = pBrCfgMsg->ovs_br[brIdx];
        for (portIdx = 0; portIdx < pBrCfgMsg->numports[brIdx]; portIdx++) 
        {
            pMcpdBrcfg->ovs_ports[brIdx][portIdx] = pBrCfgMsg->ovs_ports[brIdx][portIdx];
        }
    }

    /* Pass on the config information to multicast driver */
    bcm_mcast_api_update_ovs_brcfg(mcpd_router.sock_nl, pBrCfgMsg);
    return 0;
}

/** Check if the input bridge is a OvS bridge. If then, return
 *  bridge ifindex.
 *
 * @param *brname      (IN) Bridge name
 * 
 * @param *ret_brIdx   (OUT) Bridge IfIndex
 *
 * @return 1 - Input bridge is OvS Bridge
 *         0 - Input bridge is not a OvS bridge
 *
 */
int mcpd_is_ovs_bridge(char *brname, int *ret_brIdx)
{
    int ifIndex = -1;
    int brIdx = 0;
    t_ovs_mcpd_brcfg_info *pMcpdBrcfg = &(mcpd_router.ovs_info.brcfg); 

    if (bcm_mcast_api_ifname_to_idx(brname, &ifIndex) < 0 )
    {
        return 0;
    }

    for (brIdx = 0; brIdx < pMcpdBrcfg->numbrs; brIdx++)
    {
        if (pMcpdBrcfg->ovs_br[brIdx] == ifIndex)
        {
#ifdef SUPPORT_OVS_BRIDGE_WAN_MCAST
            /* if the bridge is set in proxy-interfaces, return false. */
            if (mcpd_upstream_interface_lookup(brname, MCPD_PROTO_MAX) == MCPD_TRUE)
            {
                return 0;
            }
#endif
            if (ret_brIdx)
            { 
                *ret_brIdx = brIdx;
            }
            return 1;
        }
    }
    return 0;
}

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
                                       unsigned int *num)
{
    int                   socket_fd;
    struct ifreq          ifr_port;
    int                   wanidx = 0;
    int                   bridx;
    char                  ovs_portname[IFNAMSIZ];
    int                   rt;
    int                   portidx;
    t_ovs_mcpd_brcfg_info *pMcpdBrcfg = &(mcpd_router.ovs_info.brcfg); 

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) 
    {
        return -EIO;
    }

    if (0 == mcpd_is_ovs_bridge(brname, &bridx)) 
    {
        close(socket_fd);
        return -EINVAL;
    }

    for (portidx = 0; portidx < pMcpdBrcfg->numports[bridx]; portidx++) 
    {
        if (bcm_mcast_api_ifindex_to_name(pMcpdBrcfg->ovs_ports[bridx][portidx], 
                                          ovs_portname) != NULL) 
        {
            memset(&ifr_port, 0, sizeof(struct ifreq));
            strncpy(ifr_port.ifr_name, ovs_portname, IFNAMSIZ);
            rt = ioctl(socket_fd, SIOCDEVISWANDEV, (void *)&ifr_port);
            if ((rt >= 0) && (1 == ifr_port.ifr_flags)) 
            {
                ifindices[wanidx] = pMcpdBrcfg->ovs_ports[bridx][portidx];
                wanidx++;
            }
        }
    }
    close(socket_fd);
    *num = wanidx;
    return 0;
}

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
int mcpd_ovs_get_bridges(int ifindices[], unsigned int *num)
{
    int brIdx = 0;
    t_ovs_mcpd_brcfg_info *pMcpdBrcfg = &(mcpd_router.ovs_info.brcfg); 
    for (brIdx = 0; brIdx < pMcpdBrcfg->numbrs; brIdx++)
    {
        ifindices[brIdx] = pMcpdBrcfg->ovs_br[brIdx];
    }
    *num = pMcpdBrcfg->numbrs;
    return 0;
}

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
                                 int addentry, UINT16 lan_tci)
{
    char repifname[IFNAMSIZ]={0};
    char brname[IFNAMSIZ]={0};
     
    if (bcm_mcast_api_ifindex_to_name(brifi, brname) &&
        !mcpd_is_ovs_bridge(brname, NULL))
    {
        /* Not an OVS bridge. Nothing to do */
        return;
    }

    /* Send drop group message to OvS */
    if (bcm_mcast_api_ifindex_to_name(repifi, repifname) != NULL) 
    {
        t_ovs_mcpd_msg ovsMsg;
        ovsMsg.msgtype = MCPD_OVS_MSG_MANAGE_SNOOP_ENTRY;
        ovsMsg.snoopentry.addentry = addentry;
        strncpy(ovsMsg.snoopentry.brname, brname, sizeof(ovsMsg.snoopentry.brname));
        strncpy(ovsMsg.snoopentry.portname, repifname, sizeof(repifname));
        ovsMsg.snoopentry.vlan = lan_tci;
        if (gpaddr6) 
        {
            /* MLD */
            ovsMsg.snoopentry.is_igmp = 0;
            memcpy(&ovsMsg.snoopentry.ipv6grp, gpaddr6, sizeof(struct in6_addr));
            if (srcaddr6) 
            {
                memcpy(&ovsMsg.snoopentry.ipv6src, srcaddr6, sizeof(struct in6_addr));
            }
            else
            {
                memset(&ovsMsg.snoopentry.ipv6src, 0, sizeof(struct in6_addr));
            }
        }
        else
        {
            /* IGMP */
            ovsMsg.snoopentry.is_igmp = 1;
            ovsMsg.snoopentry.ipv4grp.s_addr = gpaddr;
            ovsMsg.snoopentry.ipv4src.s_addr = srcaddr;
        }
        mcpd_ovs_send_msg((void *)&ovsMsg, sizeof(ovsMsg));
    }
}

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
                                        unsigned int *num)
{
    int ifIndex = -1;
    int brIdx   = 0;
    int portIdx = 0;
    t_ovs_mcpd_brcfg_info *pMcpdBrcfg = &(mcpd_router.ovs_info.brcfg); 

    if (bcm_mcast_api_ifname_to_idx(brName, &ifIndex) < 0 )
    {
        return -1;
    }

    for (brIdx = 0; brIdx < pMcpdBrcfg->numbrs; brIdx++)
    {
        if (pMcpdBrcfg->ovs_br[brIdx] == ifIndex)
        {
            /* Found a matching bridge. Populate port info */
            *num = pMcpdBrcfg->numports[brIdx];
            for (portIdx = 0; portIdx < (*num); portIdx++) 
            {
                ifindices[portIdx] = pMcpdBrcfg->ovs_ports[brIdx][portIdx];
            }
            return 0;
        }
    }
    /* Matching bridge not found. Return error */
    return -1;
}

/** Receive incoming messages from OvS vswitchd daemon and
 *  process them
 * 
 * @return None
 *
 */
void mcpd_ovs_socket_receive(void)
{
    int length;
    int ret = 0;
  
    while ((length = recv(mcpd_router.ovs_info.sock_ovs_con, &mcpd_router.ovs_info.sock_ovs_buff[0], 
                  BCM_MCAST_NL_RX_BUF_SIZE, 0)) > 0)
    {
        t_ovs_mcpd_msg *pMsg;

        pMsg = (t_ovs_mcpd_msg *)&mcpd_router.ovs_info.sock_ovs_buff[0];

        switch (pMsg->msgtype)
        {
            case OVS_MCPD_MSG_IGMP:
            {
                 ret = mcpd_igmp_process_input(&(pMsg->pktInfo));
                 MCPD_TRACE(MCPD_TRC_LOG,"OVS_MCPD_MSG_IGMP %d datalen %d ret %d type 0x%x\n", 
                            (int)pMsg->msgtype, pMsg->pktInfo.data_len, ret, pMsg->pktInfo.pkt[0]);
                 break;
            }

            case OVS_MCPD_MSG_MLD:
            {
                 ret = mcpd_mld_process_input(&(pMsg->pktInfo));
                 MCPD_TRACE(MCPD_TRC_LOG, "OVS_MCPD_MSG_MLD %d datalen %d ret %d type 0x%x\n", 
                            (int)pMsg->msgtype, pMsg->pktInfo.data_len, ret, pMsg->pktInfo.pkt[0]);
                 break;
            }

            case OVS_MCPD_MSG_BRIDGE_CONFIG_UPDATE:
            {
                t_ovs_mcpd_brcfg_info *pBrCfgMsg = &(pMsg->brcfgmsg);
                mcpd_ovs_brcfg_update(pBrCfgMsg);
                MCPD_TRACE(MCPD_TRC_LOG, "mcpd_ovs_brcfg_update successful\n");
                break;
            }

            default:
                MCPD_TRACE(MCPD_TRC_ERR, "unknown message (%d)", pMsg->msgtype);
                break;
        }
    }

    /* if length <= 0 no more messages to receive. Close socket */
    close(mcpd_router.ovs_info.sock_ovs_con);
    mcpd_router.ovs_info.sock_ovs_con = -1;
}

/** Set snoop aging time in OvS snooping table for all
 *  OvS bridges
 *
 * @param agingtimeinsec      (IN) Snoop entry aging time
 * 
 * @return 0 - Success, -1 - Otherwise
 *
 */
int mcpd_ovs_set_snoop_aging_time(int agingtimeinsec)
{
    char cmd[1024]={0};
    char brname[IFNAMSIZ]={0};
    int bridx;

    for (bridx = 0; bridx < mcpd_router.ovs_info.brcfg.numbrs; bridx++)
    {
        if (bcm_mcast_api_ifindex_to_name(mcpd_router.ovs_info.brcfg.ovs_br[bridx], brname) &&
            (mcpd_downstream_interface_lookup(brname, MCPD_PROTO_IGMP) || 
             mcpd_downstream_interface_lookup(brname, MCPD_PROTO_MLD)))
        {
            /* It is important to have the & and run this as a background process. Otherwise,
               the system cmd performance is bad and MCPD gets blocked too long resulting in
               OvS->MCPD socket timeouts. Also tried forking a child process
               (similar to runCommandInShell() in prctl.c) to run the system command. However,
               no improvement in performance was noticed. The performance was comparable to
               running the system command with &. Note that this bridge command would trigger
               a brcfg update from OvS */
            snprintf(cmd, sizeof(cmd), "ovs-vsctl set Bridge %s other_config:mcast-snooping-aging-time=%d &",
                    brname, agingtimeinsec);
            system(cmd);
        }
    }
    return 0;
}
#endif /* defined(CONFIG_BCM_OVS_MCAST) */
