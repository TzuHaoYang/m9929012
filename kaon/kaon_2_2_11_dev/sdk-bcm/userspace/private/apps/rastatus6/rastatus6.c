/*
* <:copyright-BRCM:2007-2010:proprietary:standard
* 
*    Copyright (c) 2007-2010 Broadcom 
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
* :>

*/

#include "cms.h"
#include "cms_util.h"
#include "cms_msg.h"

#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <net/if.h>
#include <signal.h>

#ifdef DMP_DEVICE2_HOSTS_2
#include <sys/queue.h>
#include "cms_obj.h"
#include "cms_qdm.h"
#include "../../libs/cms_core/mdm.h"
#include "../../libs/cms_core/linux/rut_util.h"
#include "../../apps/ssk/ssk.h"

#define PER_CHECK_FILE "/tmp/check"

//#define DPRINTF

#ifdef DPRINTF
 #define DISC_DEBUG(str, args...) do {printf("%s:%d: " str "\n", __FUNCTION__, __LINE__, ##args); fflush(stdout);} while (0)
 #define DISC_ERROR(str, args...) DISC_DEBUG(str, ##args)
 #define DISC_NOTICE(str, args...) DISC_DEBUG(str, ##args)
#else
 #define DISC_DEBUG cmsLog_debug
 #define DISC_ERROR cmsLog_error
 #define DISC_NOTICE cmsLog_notice
#endif

SINT32 host6DiscFd;

struct discOfHost6 {
     /* link-layer address */
     char host[MAC_STR_LEN+1];

     /* global-unicast address */
     char addr[BUFLEN_48];

     char iface[CMS_IFNAME_LENGTH];
     char state[BUFLEN_16];
};

typedef enum {
     ACTION_LAUNCH,
     ACTION_RENEW,
     ACTION_CLOSE
} action_t;
#endif

#define ND_OPT_RDNSS  25 /* RFC 6106 */
#define ND_OPT_DNSSL 31 /* RFC 6106 */

#define ND_RA_FLAG_RTPREF_MASK 0x18

SINT32 raMonitorFd;
static int sigterm_received = 0;
void * msgHandle=NULL;

// brcm
#ifdef __linux__
/* from linux/ipv6.h */
struct in6_pktinfo {
     struct in6_addr ipi6_addr;
     int ipi6_ifindex;
};
#endif


static CmsRet parsepio(struct nd_opt_prefix_info *pio, UINT8 len __attribute__((unused)), 
                       RAStatus6MsgBody *ramsg)
{
   if (inet_ntop(AF_INET6, &pio->nd_opt_pi_prefix, ramsg->pio_prefix,
                  CMS_IPADDR_LENGTH) == NULL)
   {
      return CMSRET_INVALID_PARAM_VALUE;
   }

   ramsg->pio_prefixLen = pio->nd_opt_pi_prefix_len;
   ramsg->pio_L_flag = pio->nd_opt_pi_flags_reserved & ND_OPT_PI_FLAG_ONLINK;
   ramsg->pio_A_flag = pio->nd_opt_pi_flags_reserved & ND_OPT_PI_FLAG_AUTO;
   ramsg->pio_plt = ntohl(pio->nd_opt_pi_preferred_time);
   ramsg->pio_vlt = ntohl(pio->nd_opt_pi_valid_time);

   return CMSRET_SUCCESS;
}

static CmsRet parserdnss(const UINT8 *dns, UINT8 len, RAStatus6MsgBody *ramsg)
{
   /*
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |     Type      |     Length    |           Reserved            |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                           Lifetime                            |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                                                               |
     :            Addresses of IPv6 Recursive DNS Servers            :
     |                                                               |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
   int num;
   char *msgptr;
   const UINT8 *pktptr;

   pktptr = dns;
   cmsLog_debug("rdnss: len<%d>", len);

   /* number of addresses in the message is (len-1)/2 */
   num = (len -1)/2;
   if (num < 1)
   {
      cmsLog_debug("there must be at least one DNS server in dnss msg");
      return CMSRET_INVALID_PARAM_VALUE;
   }

   pktptr += 8;
   msgptr = ramsg->dns_servers;
   /* TODO: we only support at most two dns servers */
   if (num > 2)
   {
      cmsLog_debug("more than 2 dns servers in dnss msg");
      num = 2;
   }

   do
   {
      if (inet_ntop(AF_INET6, pktptr, msgptr, CMS_IPADDR_LENGTH) == NULL)
      {
         return CMSRET_INVALID_PARAM_VALUE;
      }
      
      num--;
      pktptr += sizeof (struct in6_addr);
      msgptr += strlen(msgptr);

      if (num == 1)
      {
         *(msgptr++) = ',';
      }

   } while(num);

   ramsg->dns_lifetime = *((uint32_t *)dns + 1);

   return CMSRET_SUCCESS;
}

#if 0 //TODO
static CmsRet parserdnssl(const UINT8 *domain, UINT8 len, RAStatus6MsgBody *ramsg)
{
   /*
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |     Type      |     Length    |           Reserved            |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                           Lifetime                            |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                                                               |
     :                Domain Names of DNS Search List                :
     |                                                               |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
   int num;
   char *msgptr;
   const UINT8 *pktptr;

   pktptr = domain;
   cmsLog_debug("dnssl: len<%d>", len);

   if (len < 2)
   {
      cmsLog_debug("there must be at least one domain in dnssl msg");
      return CMSRET_INVALID_PARAM_VALUE;
   }

   pktptr += 8;
   msgptr = ramsg->domainName;
   /* TODO: we only support one domain name up to 32 characters */




   return CMSRET_SUCCESS;
}
#endif

#ifdef DMP_DEVICE2_HOSTS_2
/*
 * hash_table_init() and
 * hash_table_cleanup() and
 * hash_table_find() and
 * hash_table_add() and
 * hash_table_remove() 
 * are copyrights as follows:
 *
 * Copyright (C) 1998 and 1999 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
struct hash_entry {
	LIST_ENTRY(hash_entry) list;
	char *val;
};

LIST_HEAD(hash_head, hash_entry);

typedef unsigned int (*pfn_hash_t)(void *val) ;
typedef int (*pfh_hash_match_t)(void *val1, void *val2); 

struct hash_table {
	struct hash_head *table;
	unsigned int size;
	unsigned int num; // brcm
	pfn_hash_t hash;
	pfh_hash_match_t match;
};

#define HOST6_DISC_TABLE_SIZE 256
#define HOST6_DISC_ENTRY_MAX 128

/* defaults from ndisc6(8) - Linux man page */
#define NDISC6_ATTEMPTS 3
#define NDISC6_WIAT_MSECS 1000

//#define PERIODIC_HOST6_CHECK_INTERVAL ((((HOST6_DISC_ENTRY_MAX * NDISC6_ATTEMPTS) / 2) * NDISC6_WIAT_MSECS) / MSECS_IN_SEC)
#define PERIODIC_HOST6_CHECK_INTERVAL 20  // as web page reload interval defined in dhcpinfo.html

/* A local cache to buffer MDM I/O. */
static struct hash_table host6_disc_table;

// brcm
static unsigned int
lladdr_hash(val)
	void *val;
{
	u_int8_t *addr = val;
	unsigned int hash = 0;
	int i;

	for (i = 0; i < MAC_ADDR_LEN; i++) {
		hash += addr[i];
	}

	return (hash);
}

// brcm
static int
lladdr_match(val1, val2)
	void *val1, *val2;
{
	u_int8_t * addr1 = val1;
	u_int8_t * addr2 = val2;

	return (memcmp(addr1, addr2, MAC_ADDR_LEN) == 0);
}

static int
hash_table_init(table, size, hash, match)
	struct hash_table *table;
	unsigned int size;
	pfn_hash_t hash;
	pfh_hash_match_t match;
{
	unsigned int i;

	if (!table || !hash || !match) {
		return (-1);
	}

	if ((table->table = malloc(sizeof(*table->table) * size)) == NULL) {
		return (-1);
	}

	for (i = 0; i < size; i++)
		LIST_INIT(&table->table[i]);

	table->size = size;
	table->num = 0; // brcm
	table->hash = hash;
	table->match = match;

	return (0);
}

static void
hash_table_cleanup(table)
	struct hash_table *table;
{
	unsigned int i;
	char macStr[MAC_ADDR_LEN]; // brcm

	if (!table) {
		return;
	}

	for (i = 0; i < table->size; i++) {
		while (!LIST_EMPTY(&table->table[i])) {
			struct hash_entry *entry = LIST_FIRST(&table->table[i]);
			LIST_REMOVE(entry, list);
			if (entry->val) {
				cmsUtl_macNumToStr((u_int8_t *)entry->val, macStr); // brcm
				DISC_DEBUG("table[%d]: entry->val=%s", i, macStr);
				free(entry->val);
			}
			free(entry);
		}
	}
	free(table->table);
	memset(table, 0, sizeof(*table));
}

static struct hash_entry *
hash_table_find(table, val)
	struct hash_table *table;
	void *val;
{
	struct hash_entry *entry;
	int i;

	if (!table || !val) {
		return (NULL);
	}

	i = table->hash(val) % table->size;
	LIST_FOREACH(entry, &table->table[i], list)
	{
		if (table->match(val, entry->val)) {
			return (entry);
		}
	}

	return (NULL);
}

static int
hash_table_add(table, val, size)
	struct hash_table *table;
	void *val;
	unsigned int size;
{
	struct hash_entry *entry = NULL;
	int i = 0;

	if (!table || !val) {
		return (-1);
	}

	// brcm
	if (table->num == HOST6_DISC_ENTRY_MAX) {
		return (-1);
	}

	// brcm
	if ((entry = hash_table_find(table, val)) != NULL) {
		return (-1);
	}

	if ((entry = malloc(sizeof(*entry))) == NULL) {
		return (-1);
	}
	memset(entry, 0, sizeof(*entry));

	if ((entry->val = malloc(size)) == NULL) {
		return (-1);
	}
	memcpy(entry->val, val, size);

	table->num++; // brcm
	i = table->hash(val) % table->size;
	LIST_INSERT_HEAD(&table->table[i], entry, list);

	return (0);
}

static int
hash_table_remove(table, val)
	struct hash_table *table;
	void *val;
{
	struct hash_entry *entry;

	if (!table || !val) {
		return (-1);
	}

	// brcm
	if (table->num == 0) {
		return (-1);
	}

	if ((entry = hash_table_find(table, val)) == NULL) {
		return (-1);
	}

	LIST_REMOVE(entry, list);
	if (entry->val)
		free(entry->val);
	free(entry);
	table->num--; // brcm

	return (0);
}

// brcm
static int
hash_entry_count(table)
	struct hash_table *table;
{
	return (!table ? -1 : (int) table->num);
}
#endif

CmsRet initRAMonitorFd()
{
   struct icmp6_filter filter;
   int val;

   if ( (raMonitorFd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0 )
   {
       cmsLog_error("Could not open socket for RA monitor");
       return CMSRET_INTERNAL_ERROR;
   }

   /*
    * Ask for ancillary data with each ICMPv6 packet so we can get
    * the incoming interface name.
    */
   val = 1;
   if ( setsockopt(raMonitorFd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &val,
                   sizeof(val)) < 0 )
   {
       cmsLog_error("could not set option to get ifname");
       return CMSRET_INTERNAL_ERROR;
   }

   /* fetch router advertisement only */
   ICMP6_FILTER_SETBLOCKALL(&filter);
   ICMP6_FILTER_SETPASS(ND_ROUTER_ADVERT, &filter);

   if ( setsockopt(raMonitorFd, IPPROTO_ICMPV6, ICMP6_FILTER, &filter,
                   sizeof(filter)) < 0 )
   {
       cmsLog_error("could not set filter for RA");
       return CMSRET_INTERNAL_ERROR;
   }

   return CMSRET_SUCCESS;
}


void cleanupRAMonitorFd()
{
   if ( raMonitorFd != CMS_INVALID_FD )
   {
      close(raMonitorFd);
   }
}


CmsRet processRAMonitor(RAStatus6MsgBody *ramsg)
{
   char buf[BUFLEN_1024], ifName[BUFLEN_32];
   int buflen;
   char ancbuf[CMSG_SPACE(sizeof (struct in6_pktinfo)) ];
   struct iovec iov;
   struct msghdr msg;
   struct cmsghdr *cmsgp;
   struct in6_pktinfo *pktinfo = NULL;
   struct sockaddr_in6 src;
   char gwAddr[CMS_IPADDR_LENGTH];          
   struct nd_router_advert *ra;
   UINT8 *data_p;
   int len, m_flag, o_flag, prf_flag, router_lifetime;
   CmsRet ret = CMSRET_SUCCESS;
    
   iov.iov_len = sizeof(buf);
   iov.iov_base = (void *)buf;

   memset(&msg, 0, sizeof(msg));
   msg.msg_name = (void *)&src;
   msg.msg_namelen = sizeof(src);
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;
   msg.msg_control = (void *)ancbuf;
   msg.msg_controllen = sizeof(ancbuf);

   if ( (buflen = recvmsg(raMonitorFd, &msg, 0)) < 0 )
   {
      cmsLog_error("read error on raw socket");
      return CMSRET_INTERNAL_ERROR;
   }

   for (cmsgp = CMSG_FIRSTHDR(&msg); cmsgp != NULL;
        cmsgp = CMSG_NXTHDR(&msg, cmsgp))
   {
      if (cmsgp->cmsg_len == 0)
      {
         cmsLog_error("ancillary data with zero length");
         return CMSRET_INTERNAL_ERROR;
      }

      if ((cmsgp->cmsg_level == IPPROTO_IPV6) && 
          (cmsgp->cmsg_type == IPV6_PKTINFO))
      {
         pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsgp);
      }
   }

   if ( pktinfo != NULL )
   {
      if ( if_indextoname(pktinfo->ipi6_ifindex, ifName) == NULL )
      {
         cmsLog_error("couldn't find interface name: index %d", 
                      pktinfo->ipi6_ifindex);
         return CMSRET_INTERNAL_ERROR;
      }
   }
   else
   {
      cmsLog_error("couldn't get ancillary data for packet");
      return CMSRET_INTERNAL_ERROR;
   }

   if (inet_ntop(AF_INET6, &src.sin6_addr, gwAddr, CMS_IPADDR_LENGTH)==NULL)
   {
      cmsLog_error("invalid IPv6 address??");
      return CMSRET_INTERNAL_ERROR;
   }

   /* parse the RA for detailed info */
   data_p = (UINT8 *)buf;
   len = buflen;
    
   ra = (struct nd_router_advert *)data_p;

   if (ra->nd_ra_type != ND_ROUTER_ADVERT)
   {
      cmsLog_debug("Not RA");
      return CMSRET_INTERNAL_ERROR;
   }

   m_flag = ra->nd_ra_flags_reserved & ND_RA_FLAG_MANAGED;
   o_flag = ra->nd_ra_flags_reserved & ND_RA_FLAG_OTHER;
   prf_flag = (ra->nd_ra_flags_reserved & ND_RA_FLAG_RTPREF_MASK) >> 3;

   router_lifetime = ntohs(ra->nd_ra_router_lifetime);

   cmsLog_debug("RA with M<%d> O<%d> p<%d> lifetime<%d> from<%s> at <%s>", 
                m_flag, o_flag, prf_flag, router_lifetime, gwAddr, ifName);
    
   /* fetch prefix info */
   data_p += sizeof (struct nd_router_advert);
   len -= sizeof (struct nd_router_advert);

   /* RFC 4861: options will be always at 64-bit boundaries */
   while (len >= 8)
   {
      UINT8 optlen;
      UINT8 pio_cnt = 0;

      optlen = data_p[1];
      if ( optlen == 0 )
      {
         cmsLog_debug("option with zero length");
         break;
      }

      switch (data_p[0])
      {
         case ND_OPT_PREFIX_INFORMATION:
            if (pio_cnt != 0)
            {
               cmsLog_debug("Do not support 2nd PIO in RA");
               break;
            }

            if ((ret = parsepio((struct nd_opt_prefix_info *)data_p, 
                           optlen, ramsg)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Cannot parse prefix option");
               return ret;
            }
            else
            {
               pio_cnt++;
            }

            break;

         case ND_OPT_RDNSS:
            if (router_lifetime)
            {
               if ((ret = parserdnss(data_p, 
                              optlen, ramsg)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("Cannot parse rdnss option");
                  return ret;
               }
            }
            break;

#if 0 //TODO
         case ND_OPT_DNSSL:
            if (router_lifetime)
            {
               if ((ret = parserdnssl(data_p, 
                              optlen, ramsg)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("Cannot parse rdnss option");
                  return ret;
               }
            }
            break;
#endif

         default:
               cmsLog_debug("option type<%d>", data_p[0]);
               break;
      }

      data_p += (optlen*8);
      len -= (optlen*8);
   }

   cmsLog_debug("PIO with L<%d> A<%d> plt<%u> vlt<%u> prefix<%s/%u>", 
                ramsg->pio_L_flag, ramsg->pio_A_flag, ramsg->pio_plt, 
                ramsg->pio_vlt, ramsg->pio_prefix, ramsg->pio_prefixLen);

   cmsUtl_strncpy(ramsg->router, gwAddr, BUFLEN_40);
   ramsg->router_lifetime = router_lifetime;
   ramsg->router_M_flags = m_flag;
   ramsg->router_O_flags = o_flag;
   ramsg->router_P_flags = prf_flag;
   cmsUtl_strncpy(ramsg->ifName, ifName, sizeof(ramsg->ifName));

   return ret;
}

void sendRAInfoEventMessage(RAStatus6MsgBody *raInfo)
{
   char buf[sizeof(CmsMsgHeader) + sizeof(RAStatus6MsgBody)]={0};
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   RAStatus6MsgBody *raStatus6MsgBody = (RAStatus6MsgBody *) (msg+1);
   CmsRet ret;

   msg->type = CMS_MSG_RASTATUS6_INFO;
   msg->src = MAKE_SPECIFIC_EID(getpid(), EID_RASTATUS6);
   msg->dst = EID_SSK;
   msg->flags_event = 1;
   msg->dataLength = sizeof(RAStatus6MsgBody);

   memcpy(raStatus6MsgBody, raInfo, sizeof(RAStatus6MsgBody));

   if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send out CMS_MSG_RASTATUS6_INFO, ret=%d", ret);
   }
   else
   {
      cmsLog_notice("sent out CMS_MSG_RASTATUS6_INFO");
   }

   return;
}

#ifdef DMP_DEVICE2_HOSTS_2
void cleanupHost6DiscFd()
{
   if ( host6DiscFd != CMS_INVALID_FD )
   {
      close(host6DiscFd);
   }
}

void send_one_info(UBOOL8 isDelete, const struct discOfHost6 *one)
{
   char buf[sizeof(CmsMsgHeader) + sizeof(DhcpdHostInfoMsgBody)] = {0};
   CmsMsgHeader *hdr = (CmsMsgHeader *) buf;
   DhcpdHostInfoMsgBody *body = (DhcpdHostInfoMsgBody *) (hdr+1);
   CmsRet ret;

   hdr->type = CMS_MSG_RASTATUS6_HOST6_INFO;
   hdr->src = EID_RASTATUS6;
   hdr->dst = EID_SSK;
   hdr->flags_event = 1;
   hdr->dataLength = sizeof(DhcpdHostInfoMsgBody);

   body->deleteHost = isDelete;

   /* No query of time remaining for SLAAC or DHCPv6 address */
   body->leaseTimeRemaining = 0;

   cmsUtl_strncpy(body->ifName, one->iface, sizeof(body->ifName));
   cmsUtl_strncpy(body->ipAddr, one->addr, sizeof(body->ipAddr));

   /* No query of host name */
   cmsUtl_strncpy(body->hostName, "", sizeof(body->hostName));

   cmsUtl_strncpy(body->macAddr, one->host, sizeof(body->macAddr));

   cmsUtl_strncpy(body->addressSource, MDMVS_DHCP, sizeof(body->addressSource));
   cmsUtl_strncpy(body->interfaceType, MDMVS_ETHERNET, sizeof(body->interfaceType));

   if ((ret = cmsMsg_send(msgHandle, hdr)) != CMSRET_SUCCESS)
   {
      DISC_DEBUG("could not send out CMS_MSG_RASTATUS6_HOST6_INFO, ret=%d", ret);
   }
}

CmsRet processHost6Disc(action_t this, const char *ifName)
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2IpInterfaceObject *ipIntf = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;

   if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      DISC_DEBUG("failed to get lock, ret=%d", ret);
      return ret;
   }

   if (qdmIpIntf_isAllBridgeWanServiceLocked())
   {
      DISC_DEBUG("All bridge configuration and no need to discover IPv6 hosts");
      cmsLck_releaseLock();
      return ret;
   }

   while (!found && (cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &iidStack, (void **) &ipIntf) == CMSRET_SUCCESS))
   {
      char subnetCidr6[CMS_IPADDR_LENGTH];
      char f_all[BUFLEN_64], f_each[BUFLEN_64];
      FILE *fp_all, *fp_each;
      char buf[BUFLEN_32];
      char cmd[BUFLEN_512];
      struct discOfHost6 one;

      /* skip all IP.Interface with name not of brx */
      if (cmsUtl_strncmp(ipIntf->name, "br", 2))
      {
         cmsObj_free((void **)&ipIntf);
         continue;
      }
      else if (cmsUtl_strcmp(ipIntf->name, ifName))
      {
         cmsObj_free((void **)&ipIntf);
         continue;
      }
      else
      {
         cmsObj_free((void **)&ipIntf);
         found = TRUE;
      }

      if ((this != ACTION_CLOSE) && !qdmIpIntf_getIpv6DelegatedPrefixByNameLocked_dev2(ifName, subnetCidr6))
      {
         continue;
      }

      cmsUtl_strncpy(subnetCidr6, strtok(subnetCidr6, "/"), sizeof(subnetCidr6));
      subnetCidr6[strlen(subnetCidr6) - 1] = '\0';

      /* Link-layer address changes dynamically on host side is not supported. */

      cmsLck_releaseLock();

      sprintf(f_all, HOST6_ACTIVES_FILE "_%s", ifName);

      if (this == ACTION_LAUNCH)
      {
         char *tmp = "-v -e FAILED -e INCOMPLETE";

         sprintf(buf, "ip -6 neigh show dev %s", ifName);
         sprintf(cmd, "test -n \"`%s`\" && (test -n \"`%s | grep %s | grep %s`\" && %s | grep %s | grep %s > %s || rm -f %s)",
                 buf, buf, tmp, subnetCidr6, buf, tmp, subnetCidr6, f_all, f_all);
         rut_doSystemAction("processHost6Disc", cmd);
         DISC_DEBUG("cmd<%s>", cmd);
      }

      if (access(f_all, F_OK) == -1)
      {
         goto lock_n_go;
      }

      if ((fp_all = fopen(f_all, "r")) == NULL)
      {
         ret = CMSRET_OPEN_FILE_ERROR;
         goto lock_n_go;
      }

      cmsUtl_strncpy(one.iface, ifName, sizeof(one.iface));

      while ((ret == CMSRET_SUCCESS) && (EOF != fscanf(fp_all, "%s lladdr %s %s", one.addr, one.host, one.state)))
      {
         UINT8 macNum[MAC_ADDR_LEN];

         DISC_DEBUG("ONE with host<%s> addr<%s> iface<%s>", one.host, one.addr, one.iface);

         if (this == ACTION_CLOSE)
         {
            send_one_info(TRUE, &one);
            continue;
         }

         cmsUtl_macStrToNum(one.host, macNum);

         sprintf(f_each, PER_CHECK_FILE "_%s_%s", one.addr, ifName);
         sprintf(cmd, "ndisc6 -r %d -qw %d %s %s > %s", NDISC6_ATTEMPTS, NDISC6_WIAT_MSECS, one.addr, ifName, f_each);
         rut_doSystemAction("processHost6Disc", cmd);

         if ((fp_each = fopen(f_each, "r")) == NULL)
         {
            ret = CMSRET_OPEN_FILE_ERROR;
            continue;
         }

         fseek(fp_each, 0, SEEK_END);

         if (ftell(fp_each) == 0)
         {
            if (hash_table_remove(&host6_disc_table, macNum) == 0)
            {
               sprintf(cmd, "ip -6 neigh del %s lladdr %s dev %s 2>/dev/null", one.addr, one.host, ifName);
               rut_doSystemAction("processHost6Disc", cmd);
               DISC_NOTICE("remove hash entry<%s>", one.addr);
               send_one_info(TRUE, &one);
            }
            else
            {
               /*
                * XXX: Data inconsistency may need further review since we expected
                * 1-to-1 mapping between HOST6_ACTIVES_FILE and host6_disc_table.
                */
               DISC_ERROR("Cannot find hash entry<%s>!?", one.addr);
//               ret = CMSRET_INTERNAL_ERROR;
//               continue;
            }

            DISC_NOTICE("# of hash entries<%d>", hash_entry_count(&host6_disc_table));
         }
         else if (this == ACTION_LAUNCH)
         {
            if (hash_table_add(&host6_disc_table, macNum, sizeof(macNum)) == 0)
            {
               DISC_NOTICE("add hash entry<%s>", one.addr);
               send_one_info(FALSE, &one);
            }
            else
            {
               DISC_NOTICE("hash entry<%s> likely existed", one.addr);
            }

            DISC_NOTICE("# of hash entries<%d>", hash_entry_count(&host6_disc_table));
         }

         fclose(fp_each);
         unlink(f_each);
      }

      fclose(fp_all);
#ifdef DPRINTF
      DISC_DEBUG("cmd<%s>", buf);
      rut_doSystemAction("processHost6Disc", buf);
      DISC_DEBUG("cmd<cat %s>", f_all);
      sprintf(cmd, "cat %s 2>/dev/null", f_all);
      rut_doSystemAction("processHost6Disc", cmd);
#endif

      if ((this == ACTION_LAUNCH) && (ret == CMSRET_SUCCESS))
      {
         /* XXX: all hosts could be physically disconnected */
//         sprintf(cmd, "test -z \"`%s`\" && rm -f %s", buf, f_all);
//         rut_doSystemAction("processHost6Disc", cmd);
      }
      else
      {
         if (this == ACTION_CLOSE)
         {
            hash_table_cleanup(&host6_disc_table);
         }

         /* in response to radvd restarting */
         DISC_DEBUG("delete %s", f_all);
         unlink(f_all);
      }

lock_n_go:
      if ((ret = cmsLck_acquireLockWithTimeout(SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
      {
         DISC_DEBUG("failed to get lock, ret=%d", ret);
         return ret;
      }
   }

   cmsLck_releaseLock();

   return ret;
}
#endif

void sigterm_handler()
{
   sigterm_received++;
}

SINT32 main(SINT32 argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
   CmsRet ret;
   SINT32 n;
   fd_set readFdsMaster, readFds;
   RAStatus6MsgBody raStatus6MsgBody;
#ifdef DMP_DEVICE2_HOSTS_2
   SINT32 shmId = 0;
   struct timeval tv;
   int maxFd;
#endif

   if ((ret = cmsMsg_init(EID_RASTATUS6, &msgHandle)) != CMSRET_SUCCESS)
   {
      cmsLog_error("msg initialization failed, ret=%d", ret);
      return -1;
   }

#ifdef DMP_DEVICE2_HOSTS_2
   if ((ret = cmsMdm_init(EID_RASTATUS6, msgHandle, &shmId)) != CMSRET_SUCCESS)
   {
      DISC_ERROR("cmsMdm_init failed, ret=%d", ret);
      cmsMsg_cleanup(&msgHandle);
      return -1;
   }
#endif

   raMonitorFd = CMS_INVALID_FD;

   if ((initRAMonitorFd() != CMSRET_SUCCESS) || (raMonitorFd == CMS_INVALID_FD))
   {
      cmsLog_error("initRAMonitorFd failed");
      cleanupRAMonitorFd();
#ifdef DMP_DEVICE2_HOSTS_2
      cmsMdm_cleanup();
      cmsMsg_cleanup(&msgHandle);
#endif
      return -1;
   }

#ifdef DMP_DEVICE2_HOSTS_2
   host6DiscFd = CMS_INVALID_FD;

   if ((cmsMsg_getEventHandle(msgHandle, &host6DiscFd) != CMSRET_SUCCESS) || (host6DiscFd == CMS_INVALID_FD))
   {
      DISC_ERROR("cmsMsg_getEventHandle failed");
      cleanupHost6DiscFd();
      cleanupRAMonitorFd();
      cmsMdm_cleanup();
      cmsMsg_cleanup(&msgHandle);
      return -1;
   }

   if (hash_table_init(&host6_disc_table, HOST6_DISC_TABLE_SIZE, lladdr_hash, lladdr_match) != 0)
   {
      cleanupHost6DiscFd();
      cleanupRAMonitorFd();
      cmsMdm_cleanup();
      cmsMsg_cleanup(&msgHandle);
      return -1;
   }
#endif

   signal(SIGHUP, SIG_IGN);
   signal(SIGTERM, sigterm_handler);
   signal(SIGPIPE, SIG_IGN);
   signal(SIGINT, SIG_IGN);

   FD_ZERO(&readFdsMaster);
   FD_SET(raMonitorFd, &readFdsMaster);
#ifdef DMP_DEVICE2_HOSTS_2
   FD_SET(host6DiscFd, &readFdsMaster);
   maxFd = (raMonitorFd > host6DiscFd) ? raMonitorFd : host6DiscFd;
#endif

   while (1)
   {
      readFds = readFdsMaster;
#ifdef DMP_DEVICE2_HOSTS_2
      tv.tv_sec = PERIODIC_HOST6_CHECK_INTERVAL;
      tv.tv_usec = 0;
      n = select(maxFd+1, &readFds, NULL, NULL, &tv);
#else
      n = select(raMonitorFd+1, &readFds, NULL, NULL, NULL);
#endif
      
      if (n > 0)
      {
         if (FD_ISSET(raMonitorFd, &readFds))
         {
            memset(&raStatus6MsgBody, 0, sizeof(RAStatus6MsgBody));
            ret = processRAMonitor(&raStatus6MsgBody);

            if (ret == CMSRET_SUCCESS)
            {
               /* 
                * - Send message to advertise RA info 
                * - TODO: Timeout according to router lifetime for each ifName?
                */
               sendRAInfoEventMessage(&raStatus6MsgBody);
//               tv. = raStatus6MsgBody.router_lifetime;
            }
            else
            {
               cmsLog_notice("processRAMonitor error!");
            }
         }

#ifdef DMP_DEVICE2_HOSTS_2
         if (FD_ISSET(host6DiscFd, &readFds))
         {
            CmsMsgHeader *msg;

            ret = cmsMsg_receiveWithTimeout(msgHandle, &msg, 0);

            if (ret == CMSRET_SUCCESS)
            {
               switch (msg->type)
               {
                  case CMS_MSG_RASTATUS6_HOST6_RENEW:
                     ret = processHost6Disc(ACTION_RENEW, "br0");

                     if (ret != CMSRET_SUCCESS)
                     {
                        DISC_NOTICE("processHost6Disc error!");
                     }

                     break;

                  default:
                     DISC_NOTICE("unrecognized msg, type=0x%x dataLength=%d", msg->type, msg->dataLength);
                     break;
               }
            }
         }
#endif
      }
      else if (n == 0)
      {
         /* timeout case */
#ifdef DMP_DEVICE2_HOSTS_2
         ret = processHost6Disc(ACTION_LAUNCH, "br0");

         if (ret != CMSRET_SUCCESS)
         {
            DISC_NOTICE("processHost6Disc error!");
         }
#endif
      }

      if (sigterm_received)
      {
         break;
      }
   }

#ifdef DMP_DEVICE2_HOSTS_2
   ret = processHost6Disc(ACTION_CLOSE, "br0");

   if (ret != CMSRET_SUCCESS)
   {
      DISC_NOTICE("processHost6Disc error!");
   }

   cleanupHost6DiscFd();
   cleanupRAMonitorFd();
   cmsMdm_cleanup();
#else
   cleanupRAMonitorFd();
#endif
   cmsMsg_cleanup(&msgHandle);
   exit(0);
}
