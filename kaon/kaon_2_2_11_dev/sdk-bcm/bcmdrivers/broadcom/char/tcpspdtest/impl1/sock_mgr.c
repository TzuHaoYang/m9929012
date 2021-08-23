/*
<:copyright-BRCM:2018:DUAL/GPL:standard

   Copyright (c) 2018 Broadcom 
   All Rights Reserved

Unless you and Broadcom execute a separate written software license
agreement governing use of this software, this software is licensed
to you under the terms of the GNU General Public License version 2
(the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
with the following added to such license:

   As a special exception, the copyright holders of this software give
   you permission to link this software with independent modules, and
   to copy and distribute the resulting executable under terms of your
   choice, provided that you also meet, for each linked independent
   module, the terms and conditions of the license of that module.
   An independent module is a module which is not derived from this
   software.  The special exception does not apply to any modifications
   of the software.

Not withstanding the above, under no circumstances may you combine
this software in any way with any other Broadcom software provided
under a license other than the GPL, without Broadcom's express prior
written consent.

:>
*/

/*
*******************************************************************************
* File Name  : sock_mgr.c
*
* Description: This file contains the Broadcom Tcp Speed Test Socket Manager Implementation.
*
*  Created on: Dec 6, 2016
*      Author: yonatani, ilanb
*******************************************************************************
*/

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/bcm_log.h>
#include "tcpspdtest.h"
#include "tcp_engine_api.h"
#include "tcpspdtest_defs.h"
#include "sock_mgr.h"
#ifdef CONFIG_BCM_XRDP
#include "rdpa_udpspdtest.h"
#include "rdpa_ag_udpspdtest.h"
#include <net/udp.h>
#include <net/ip6_checksum.h>
#endif

#define WAIT_LOOP 100
#define WAIT_MSEC 50

typedef int (*hijack_tcp_in)(uint8_t stream_idx, void *, uint32_t txwscale);
typedef int (*hijack_tcp_out)(uint8_t stream_idx, void *, uint8_t);

/******************************************** Defines ********************************************/
#define TCP_MAX_MSS 1460

/**************************************** Global / Static ****************************************/
static struct tcp_info sock_tcpinfo[SPDT_NUM_OF_STREAMS];
static atomic_t nf_hook_prtcl[SPDT_NUM_OF_STREAMS] = {ATOMIC_INIT(0)};
static int sock_learned[SPDT_NUM_OF_STREAMS] = {0};
static atomic_t connected_streams = ATOMIC_INIT(0);

/**************************************** Implementation *****************************************/
/* TODO:REPLACE with HASH table!!! */
static int8_t get_stream_idx(struct sk_buff *skb)
{
    int8_t i;

    for (i = 0; i < SPDT_NUM_OF_STREAMS; i++)
    {
        if (g_tcpspd[i].srv_socket && skb->sk && skb->sk == g_tcpspd[i].srv_socket->sk)
            return i;
    }

    /* not found */
    return -1;
}

/* Check valid packet before hijacked */
static int8_t nf_hook_check_valid(struct sk_buff *skb, int is_tcp)
{
    int8_t stream_idx = -1;

    if (!skb || !skb->sk || !skb->sk->sk_socket) 
        goto out;

    if (is_tcp)
    {
        /* Handle our socket only, and only TCP packets (our socket might send ARPs as well) */
        stream_idx = get_stream_idx(skb);
        if (stream_idx < 0)
            goto out;
    }
#ifdef CONFIG_BCM_XRDP
    else
    {
        /* Check if SK Mark belong to UDP SPDT */
        if ((skb->sk->sk_mark & RDPA_UDPSPDTEST_SO_MARK_PREFIX) == RDPA_UDPSPDTEST_SO_MARK_PREFIX)
            stream_idx = 0;
        else
            goto out;
    }
#endif

    tc_debug("match stream_idx=%d\n", stream_idx);

out:
    return stream_idx;
}

extern int ip_rcv_fkb(pNBuff_t pNBuff);

int _tcp_recv(pNBuff_t pNBuff, BlogFcArgs_t *fc_args)
{
    FkBuff_t *fkb;

    if (IS_FKBUFF_PTR(pNBuff))
    {
        fkb = PNBUFF_2_FKBUFF(pNBuff);
        fkb_pull(fkb, ETH_HLEN);
        ip_rcv_fkb(fkb);
        /* FKB->data needs to be cache invalidated before getting
         * freed, or else it would cause recycled buffer issue */
        fkb_push(fkb, ETH_HLEN);
        fkb_flush(fkb, fkb->data, fkb->len, FKB_CACHE_INV); 
        fkb_free(fkb);
    }
    else
    {
        tc_err("_tcp_recv not fkb %p\n", pNBuff);
    }
      
    return 0;
}

static const struct net_device_ops _spdtst_netdev_ops = {
    .ndo_open	= NULL,
    .ndo_stop	= NULL,
    .ndo_start_xmit	= (HardStartXmitFuncP)_tcp_recv,
    .ndo_set_mac_address = NULL,
    .ndo_do_ioctl	= NULL,
    .ndo_tx_timeout	= NULL,
    .ndo_get_stats	= NULL,
    .ndo_change_mtu	= NULL 
};

static int cpu_refcnt[NR_CPUS];

struct net_device _spdtst_netdev = {
    .name		= "spdtst_netdev",
    .mtu		= 1500,
    .netdev_ops	= &_spdtst_netdev_ops,
    .pcpu_refcnt = cpu_refcnt,
};

int tcpspd_rnr_engine_on_set(int is_on);

static void tcpspeedtest_on_set(int enabled)
{
#ifdef CONFIG_BCM_XRDP
    if (rnr_engine)
        tcpspd_rnr_engine_on_set(enabled);
#endif
}

/* Netfilter IPv4/6 In/Out hooks for TCP */
static unsigned int _nf_hook_in_out_tcp(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct nf_hook_state *state,
    hijack_tcp_in hijack_tcp_in, hijack_tcp_out hijack_tcp_out)
{
    int rc;
    int8_t stream_idx;
    uint32_t mark = 0;

    /* Discard not valid packets */
    stream_idx = nf_hook_check_valid(skb, 1);
    if (stream_idx < 0)
        return NF_ACCEPT;

    /* Delete blog created by PKT_TCP4_LOCAL during http head request.
       Created blog sent the received packets directly to linux tcp layer skipping ip layer and netfilter hooks */
    if (!g_tcpspd[stream_idx].add_flow && NF_INET_LOCAL_IN == ops->hooknum && skb->blog_p)
    {
        tc_debug("blog_skip %p\n", skb->blog_p);
        blog_skip(skb, blog_skip_reason_ct_tcp_state_ignore);
    }

    mark |= ((rnr_engine) << 0); 
    mark |= ((rnr_engine) << 1); 
    mark |= ((g_tcpspd[stream_idx].action == SPDT_DIR_TX) << 2); 
    mark |= ((stream_idx) << 3); 

    if (NF_INET_LOCAL_OUT == ops->hooknum &&
        g_tcpspd[stream_idx].srv_socket &&
        (SS_CONNECTING == g_tcpspd[stream_idx].srv_socket->state ||
        SS_UNCONNECTED == g_tcpspd[stream_idx].srv_socket->state))
    {
        BlogAction_t action;

        skb_push(skb, ETH_HLEN);

        if (tcpspd_engine_hijack_hook_conn_learn(stream_idx, skb, 1))
        {
            skb_pull(skb, ETH_HLEN);
            return NF_ACCEPT;
        }
        
        if (SS_CONNECTING == g_tcpspd[stream_idx].srv_socket->state)
        {
            sock_learned[stream_idx] = 1;
            action = blog_sinit(skb, &_spdtst_netdev, TYPE_ETH, 0, BLOG_SPDTST);
            tc_debug("blog_sinit() action=%d blog=%p\n", action, skb->blog_p);
        }

        skb_pull(skb, ETH_HLEN);

        if (skb->blog_p)
        {
            skb->blog_p->spdtst = mark;
            skb->blog_p->iq_prio = BLOG_IQ_PRIO_HIGH;
        }
    }

    if (!tcpspd_engine_is_sock_learned(stream_idx))
        return NF_ACCEPT;

    tc_debug("[%hhu] state:%d, hooknum:%d\n", stream_idx, g_tcpspd[stream_idx].srv_socket->state, ops->hooknum);

    /* Ignore hijacked packet during engine data transfer (except for for FIN) */
    if (tcpspd_engine_is_state_up(stream_idx))
    {
        tcpspd_engine_done_data(stream_idx, skb_network_header(skb));
        return NF_ACCEPT;
    }

    /* Hijack packet for prtcl processing */
    if (atomic_read(&nf_hook_prtcl[stream_idx]))
    {
        /* HTTP prtcl processing, head response - learn content length */
        rc = tcpspd_prtcl_nf_hook(stream_idx, skb, ops->hooknum);
        if (NF_DROP == rc)
            return NF_DROP;
    }

    /* Server to Linux and Linux to Server conversion during TCP connected state */
    if (sock_learned[stream_idx] || SS_CONNECTED == g_tcpspd[stream_idx].srv_socket->state)
    {
        if (NF_INET_LOCAL_OUT == ops->hooknum)
        {
            if (hijack_tcp_out(stream_idx, skb_network_header(skb), skb->ip_summed))
                return NF_DROP;
        }
        else if (NF_INET_LOCAL_IN == ops->hooknum)
        {
            if (SS_CONNECTED == g_tcpspd[stream_idx].srv_socket->state)
                hijack_tcp_in(stream_idx, skb_network_header(skb), sock_tcpinfo[stream_idx].tcpi_snd_wscale);
        }
    }

    if (!sock_learned[stream_idx])
        return NF_ACCEPT;

    if (NF_INET_LOCAL_IN == ops->hooknum && SS_CONNECTED == g_tcpspd[stream_idx].srv_socket->state)
    {
        BlogAction_t action;
        struct net_device *tmpdev;

        if (g_tcpspd[stream_idx].add_flow && skb->blog_p)
        {
            uint8_t l2_hdr[ETH_HLEN] = {};
            uint16_t ethertype = (((struct ipv6hdr *)skb->data)->version == 6) ? ETH_P_IPV6 : ETH_P_IP;

            skb->blog_p->spdtst = mark;
            skb->blog_p->iq_prio = BLOG_IQ_PRIO_HIGH;
            tc_debug("blog_emit() skb=%p blog=%p\n", skb, skb->blog_p);
            skb_push(skb, ETH_HLEN);
            *(uint16_t *)(l2_hdr + (ETH_ALEN << 1)) = htons(ethertype);
            memcpy(skb->data, l2_hdr, ETH_HLEN);

            tmpdev = skb->dev;
            skb->dev = &_spdtst_netdev;
            action = blog_emit(skb, tmpdev, TYPE_ETH, 0, BLOG_SPDTST);
            skb->dev = tmpdev;

            skb_pull(skb, ETH_HLEN);
            g_tcpspd[stream_idx].add_flow = 0;
        }
    }

    return NF_ACCEPT;
}

/* Netfilter IPv4 In/Out hooks for TCP */
static unsigned int nf_hook_in_out_tcp(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct nf_hook_state *state)
{
    return _nf_hook_in_out_tcp(ops, skb, state, tcpspd_engine_hijack_tcp_in, tcpspd_engine_hijack_tcp_out);
}

/* Netfilter IPv6 In/Out hooks for TCP */
static unsigned int nf_hook_in_out6_tcp(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct nf_hook_state *state)
{
    return _nf_hook_in_out_tcp(ops, skb, state, tcpspd_engine_hijack_tcp_in6, tcpspd_engine_hijack_tcp_out6);
}

/* Netfilter declare hooks for TCP */
static struct nf_hook_ops nf_hks_tcp[] = {
    {
        .hook = nf_hook_in_out_tcp,
        .hooknum = NF_INET_LOCAL_OUT,
        .pf = NFPROTO_IPV4,
        .priority = NF_IP_PRI_FIRST,
        .owner = THIS_MODULE
    }, /* net filter OUT hook options */
    {
        .hook = nf_hook_in_out_tcp,
        .hooknum = NF_INET_LOCAL_IN,
        .pf = NFPROTO_IPV4,
        .priority = NF_IP_PRI_LAST,
        .owner = THIS_MODULE
    }, /* net filter IN hook options */
    {
        .hook = nf_hook_in_out6_tcp,
        .hooknum = NF_INET_LOCAL_OUT,
        .pf = NFPROTO_IPV6,
        .priority = NF_IP6_PRI_FIRST,
        .owner = THIS_MODULE
    }, /* net filter OUT hook options */
    {
        .hook = nf_hook_in_out6_tcp,
        .hooknum = NF_INET_LOCAL_IN,
        .pf = NFPROTO_IPV6,
        .priority = NF_IP6_PRI_LAST,
        .owner = THIS_MODULE
    }, /* net filter IN hook options */
};

#ifdef CONFIG_BCM_XRDP
static int _nf_hook_in_out_udp_set_ref_pkt(struct sk_buff *skb, int is_v6)
{
    rdpa_spdtest_ref_pkt_t ref_pkt;
    bdmf_object_handle bdmf_udpspdtest_obj_h;
    BlogAction_t action;
    struct udphdr *uh = udp_hdr(skb);
    uint16_t payload_offset = sizeof(struct udphdr) + ETH_HLEN;
    int udp_len;
    __wsum csum;
    int rc;

    /* Propagate the UDP reference packet from skb->data */
    rc = rdpa_udpspdtest_get(&bdmf_udpspdtest_obj_h);
    if (rc)
    {
        tc_err("Could not retrieve udpspdtest object, rc = %d\n", rc);
        return -1;
    }
    
    /* Recalculate UDP checksum */
    skb->ip_summed = CHECKSUM_NONE;
    if (is_v6)
    {
        struct ipv6hdr *ipv6h = ipv6_hdr(skb);
        int cs_base_buf_len;

        payload_offset += sizeof(struct ipv6hdr);
        udp_len = skb->len - ETH_HLEN - sizeof(struct ipv6hdr);
        uh->check = 0;
        cs_base_buf_len = udp_len - ETH_HLEN - sizeof(struct ipv6hdr);
        if (cs_base_buf_len < 0)
            cs_base_buf_len = udp_len;
        csum = csum_partial(skb_transport_header(skb), cs_base_buf_len, 0);

        uh->check = csum_ipv6_magic(&ipv6h->saddr, &ipv6h->daddr, udp_len, IPPROTO_UDP, csum);
        
        tc_debug("Calculated checksum for len %d = 0x%x, src_addr = %pI6, dst_addr = %pI6, skb_len = %d\n",
            udp_len, htons(uh->check), &(ipv6h->saddr), &(ipv6h->daddr), skb->len);
    }
    else
    {
        struct iphdr *iph = ip_hdr(skb);

        payload_offset += sizeof(struct iphdr);
        uh->check = 0;
        udp_len = skb->len - ETH_HLEN - sizeof(struct iphdr);
        csum = csum_partial(skb_network_header(skb), skb->len - ETH_HLEN, 0);
        uh->check = csum_tcpudp_magic(iph->saddr, iph->daddr, udp_len, IPPROTO_UDP, csum);

        tc_debug("Calculated checksum for len %d = 0x%x, src_addr = %pI4, dst_addr = %pI4, skb->len = %d\n",
            udp_len, htons(uh->check), &(iph->saddr), &(iph->daddr), skb->len);
    }
    if (uh->check == 0)
        uh->check = CSUM_MANGLED_0;
    /* For PPPoE connections, need to mark the checksum as completed */
    skb->ip_summed = CHECKSUM_COMPLETE;

    ref_pkt.data = skb->data;
    ref_pkt.size = skb->len;
    ref_pkt.udp.payload_offset = payload_offset;

    action = blog_sinit(skb, &_spdtst_netdev, TYPE_ETH, 0, BLOG_SPDTST);
    tc_debug("blog_sinit() action=%d blog=%p\n", action, skb->blog_p);
    if (skb->blog_p)
    {
        skb->blog_p->iq_prio = BLOG_IQ_PRIO_HIGH;
    }

    rc = rdpa_udpspdtest_ref_pkt_set(bdmf_udpspdtest_obj_h, 0, &ref_pkt);
    bdmf_put(bdmf_udpspdtest_obj_h);
    if (rc)
    {
        print_hex_dump(KERN_INFO, "Could not set reference packet: ", DUMP_PREFIX_NONE, 16, 1, ref_pkt.data, ref_pkt.size, true);
        return -1;
    }
    return 0;
}
#endif

/* Netfilter IPv4/6 In/Out hooks for UDP */
static unsigned int _nf_hook_in_out_udp(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct nf_hook_state *state, int is_v6)
{
    int8_t stream_idx;
    uint32_t mark = 0;

    /* Discard not valid packets */
    stream_idx = nf_hook_check_valid(skb, 0);
    if (stream_idx < 0)
        return NF_ACCEPT;

    mark |= ((rnr_engine) << 0); 
    mark |= ((rnr_engine) << 1);
    mark |= (1 << 2); 

    if (ops->hooknum == NF_INET_LOCAL_OUT)
    {
        skb_push(skb, ETH_HLEN);
        /* Partially re-use TCP learning code */
        if (tcpspd_engine_hijack_hook_conn_learn(stream_idx, skb, 0))
        {
            skb_pull(skb, ETH_HLEN);
            return NF_ACCEPT;
        }

#ifdef CONFIG_BCM_XRDP
        /* Set reference packet and invoke blog_sinit */
        if (_nf_hook_in_out_udp_set_ref_pkt(skb, is_v6))
        {
            return NF_ACCEPT;
        }
#endif

        skb_pull(skb, ETH_HLEN);
        if (skb->blog_p)
            skb->blog_p->spdtst = mark;
        if (skb->sk)
        {
#ifdef CONFIG_BCM_XRDP
            if (skb->sk->sk_mark == RDPA_UDPSPDTEST_SO_MARK_BASIC) /* mark as basic */
                skb->recycle_and_rnr_flags |= SKB_RNR_UDPSPDT_BASIC;
            else if (skb->sk->sk_mark == RDPA_UDPSPDTEST_SO_MARK_IPERF3) /* mark as iperf3 */
                skb->recycle_and_rnr_flags |= SKB_RNR_UDPSPDT_IPERF3;
#endif
        }
    }

    return NF_ACCEPT;
}

/* Netfilter IPv4 In/Out hooks for UDP */
static unsigned int nf_hook_in_out_udp(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct nf_hook_state *state)
{
    return _nf_hook_in_out_udp(ops, skb, state, 0);
}

/* Netfilter IPv6 In/Out hooks for UDP */
static unsigned int nf_hook_in_out6_udp(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct nf_hook_state *state)
{
    return _nf_hook_in_out_udp(ops, skb, state, 1);
}

/* Netfilter declare hooks for UDP */
static struct nf_hook_ops nf_hks_udp[] = {
    {
        .hook = nf_hook_in_out_udp,
        .hooknum = NF_INET_LOCAL_OUT,
        .pf = NFPROTO_IPV4,
        .priority = NF_IP_PRI_LAST,
        .owner = THIS_MODULE
    }, /* net filter OUT hook options */
    {
        .hook = nf_hook_in_out_udp,
        .hooknum = NF_INET_LOCAL_IN,
        .pf = NFPROTO_IPV4,
        .priority = NF_IP_PRI_LAST,
        .owner = THIS_MODULE
    }, /* net filter IN hook options */
    {
        .hook = nf_hook_in_out6_udp,
        .hooknum = NF_INET_LOCAL_OUT,
        .pf = NFPROTO_IPV6,
        .priority = NF_IP6_PRI_LAST,
        .owner = THIS_MODULE
    }, /* net filter OUT hook options */
    {
        .hook = nf_hook_in_out6_udp,
        .hooknum = NF_INET_LOCAL_IN,
        .pf = NFPROTO_IPV6,
        .priority = NF_IP6_PRI_LAST,
        .owner = THIS_MODULE
    }, /* net filter IN hook options */
};

int udpspd_sock_mgr_init(void)
{
    int i;

    /* Register nf hooks */
    for (i = 0; i < sizeof(nf_hks_udp) / sizeof(struct nf_hook_ops); i++)
        nf_register_hook(&nf_hks_udp[i]);

    return 0;
}

int udpspd_sock_mgr_uninit(void)
{
    int i;

    /* Register nf hooks */
    for (i = 0; i < sizeof(nf_hks_udp) / sizeof(struct nf_hook_ops); i++)
        nf_unregister_hook(&nf_hks_udp[i]);

    return 0;
}

/* Socket Connect to Server */
int tcpspd_sock_mgr_connect(uint8_t stream_idx, struct socket **sock, spdt_conn_params_t *params)
{
    int i, option, rc;
    int streams;

    streams = atomic_inc_return(&connected_streams);
    if (streams == 1)
    {
        tcpspeedtest_on_set(1);

        /* Register nf hooks */
        for (i = 0; i < sizeof(nf_hks_tcp) / sizeof(struct nf_hook_ops); i++)
            nf_register_hook(&nf_hks_tcp[i]);
    }

    /* Create Kernel Socket */
    rc = sock_create_kern(params->server_addr.ss_family, SOCK_STREAM, IPPROTO_TCP, sock);
    if (rc)
    {
        tc_err("[%hhu] Failed to create kernel socket\n", stream_idx);
        goto rel_sock;
    }

    /* Configure Socket options */
    option = 1;
    rc = kernel_setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof(option));
    if (rc < 0)
    {
        tc_err("[%hhu] kernel_setsockopt() SO_REUSEADDR Failure, rc:%d\n", stream_idx, rc);
        goto rel_sock;
    }

    option = 1;
    rc = kernel_setsockopt(*sock, SOL_TCP, TCP_NODELAY, (char *) &option, sizeof(option));
    if (rc < 0)
    {
        tc_err("[%hhu] kernel_setsockopt() TCP_NODELAY Failure, rc:%d\n", stream_idx, rc);
        goto rel_sock;
    }

    option = 0;
    rc = kernel_setsockopt(*sock, SOL_SOCKET, SO_TIMESTAMP, (char *) &option, sizeof(option));
    if (rc < 0)
    {
        tc_err("[%hhu] kernel_setsockopt() SO_TIMESTAMP Failure, rc:%d\n", stream_idx, rc);
        goto rel_sock;
    }

    option = (params->server_addr.ss_family == AF_INET ? TCP_MAX_MSS : TCP_MAX_MSS - 20);
    rc = kernel_setsockopt(*sock, IPPROTO_TCP, TCP_MAXSEG, (char *)&option, sizeof(option));
    if (rc < 0)
    {
        tc_err("[%hhu] kernel_setsockopt() TCP_MAXSEG Failure, rc:%d\n", stream_idx, rc);
        goto rel_sock;
    }

    if (params->tos)
    {
        option = params->tos;
        if (params->server_addr.ss_family == AF_INET)
        {
            rc = kernel_setsockopt(*sock, IPPROTO_IP, IP_TOS, (char *)&option, sizeof(option));
            if (rc < 0)
            {
                tc_err("[%hhu] kernel_setsockopt() IP_TOS Failure tos:%u, rc:%d\n", stream_idx, params->tos, rc);
                goto rel_sock;
            }
        }
        else
        {
            rc = kernel_setsockopt(*sock, IPPROTO_IPV6, IPV6_TCLASS, (char *)&option, sizeof(option));
            if (rc < 0)
            {
                tc_err("[%hhu] kernel_setsockopt() IPV6_TCLASS Failure tos:%u, rc:%d\n", stream_idx, params->tos, rc);
                goto rel_sock;
            }
        }
    }
 
#if 0
    option = 0;
    rc = kernel_setsockopt(*sock, SOL_TCP, SO_KEEPALIVE, (char *) &option, sizeof(option));
    if (rc < 0)
    {
        tc_err("kernel_setsockopt() SO_KEEPALIVE Failure, rc:%d\n", rc);
        goto rel_sock;
    }
#endif
    tc_debug("[%hhu] tcpspdtest: local socket created\n", stream_idx);

    /* Bind to local address */
    if (params->local_addr.ss_family)
    {
        rc = kernel_bind(*sock, (struct sockaddr *)&params->local_addr, sizeof(params->local_addr));
        if (rc < 0)
        {
            tc_err("[%hhu] kernel_bind() Failure, rc:%d\n", stream_idx, rc);
            goto rel_sock;
        }
        tc_debug("[%hhu] tcpspdtest: bound local socket to %pIScp\n", stream_idx, (struct sockaddr *)&params->local_addr);
    }

    sock_learned[stream_idx] = 0;
    g_tcpspd[stream_idx].add_flow = 0;

    rc = kernel_connect(*sock, (struct sockaddr *)&params->server_addr, sizeof(params->server_addr), 0);
    if (rc < 0)
    {
        tc_err("[%hhu] Connect to server Failed ,rc:%d\n", stream_idx, rc);
        goto rel_sock;
    }

    rc = tcpspd_engine_is_sock_learned(stream_idx);
    if (!rc)
    {
        tc_err("[%hhu] Failed to learn connection.\n", stream_idx);
        goto rel_sock;
    }

    option = sizeof(struct tcp_info);  
    rc = kernel_getsockopt(*sock, SOL_TCP, TCP_INFO, (char *)&sock_tcpinfo[stream_idx], &option);
    if (rc < 0)
    {
        tc_err("[%hhu] kernel_getsockopt() TCP_INFO Failure, rc:%d\n", stream_idx, rc);
        goto rel_sock;
    }

    if (sock_tcpinfo[stream_idx].tcpi_options & TCPI_OPT_SACK)
    	tcpspd_engine_set_sack(stream_idx);

    tcpspd_engine_set_mss(stream_idx, sock_tcpinfo[stream_idx].tcpi_snd_mss, 
                            sock_tcpinfo[stream_idx].tcpi_advmss);
    tcpspd_engine_set_rtt(stream_idx, sock_tcpinfo[stream_idx].tcpi_rtt);
    tcpspd_engine_set_rwnd(stream_idx, g_tcpspd[stream_idx].rwnd_bytes, sock_tcpinfo[stream_idx].tcpi_rtt, sock_tcpinfo[stream_idx].tcpi_rcv_wscale, g_tcpspd[stream_idx].rate_Mbps);

    return 0;

rel_sock:
    tcpspd_sock_mgr_release(stream_idx, *sock);
    *sock = NULL;
    return -1;
}

/* Send Socket Kernel message to the Server */
int tcpspd_sock_mgr_sendmsg(uint8_t stream_idx, uint8_t *data, uint32_t len)
{
    struct msghdr message = {};
    struct kvec ioVector = {};
    int rc;

    /* Prepare the message */
    ioVector.iov_base = (void *)data;
    ioVector.iov_len = len;
    message.msg_flags = MSG_NOSIGNAL;

    /* Send the message */
    rc = kernel_sendmsg(g_tcpspd[stream_idx].srv_socket, &message, &ioVector, 1, ioVector.iov_len);
    if (rc != ioVector.iov_len)
    {
        tc_err("[%hhu] kernel_sendmsg() Failure, rc:%d\n", stream_idx, rc);
        return -1;
    }

    return 0;
}

/* Wait for send operation to be fully Acked by the Server */
int tcpspd_sock_mgr_wait_send_complete(uint8_t stream_idx, struct socket *sock)
{
    int opt_size = sizeof(struct tcp_info);
    int cnt = WAIT_LOOP;
    int rc;

    do
    {
        mdelay(WAIT_MSEC);
        rc = kernel_getsockopt(sock, SOL_TCP, TCP_INFO, (char *) &sock_tcpinfo[stream_idx], &opt_size);
        if (rc < 0)
        {
            tc_err("[%hhu] kernel_getsockopt() Failure, rc:%d\n", stream_idx, rc);
            return -1;
        }
    }
    while (cnt-- && sock_tcpinfo[stream_idx].tcpi_unacked && TCP_ESTABLISHED == sock_tcpinfo[stream_idx].tcpi_state);

    if (cnt <= 0 || TCP_ESTABLISHED != sock_tcpinfo[stream_idx].tcpi_state)
        return -1;

    return 0;
}

/* Start/Stop hijack nf_hook out/in packets for prtcl processing */
void tcpspd_sock_mgr_set_nf_hook_prtcl(uint8_t stream_idx, int accept)
{
    atomic_set(&nf_hook_prtcl[stream_idx], accept);
}

/* Socket manager disconnect, shutdown the connection, delete runner flows */
int tcpspd_sock_mgr_disconnect(uint8_t stream_idx, struct socket *sock)
{
    tc_debug("[%hhu] TCPSPDTEST: Disconnecting from server\n", stream_idx);

    if (NULL == sock)
    {
        tc_err("[%hhu] Socket already released!\n", stream_idx);
        return -1;
    }

    if (SS_CONNECTED != g_tcpspd[stream_idx].srv_socket->state)
    {
        tc_err("[%hhu] Try to disconnect socket (state=%d) not in CONNECTED state\n", stream_idx,
            g_tcpspd[stream_idx].srv_socket->state);
        return -1;
    }

    /* Don't create RX flow */
    g_tcpspd[stream_idx].add_flow = 0;

    /* Engine down, delete runner flows */
    tcpspd_engine_down(stream_idx);

    /* Shutdown the connection */
    return kernel_sock_shutdown(sock, SHUT_RDWR); /* no data rx/tx is expected */
}

/* Release Socket, unregister netfilter hooks */
int tcpspd_sock_mgr_release(uint8_t stream_idx, struct socket *sock)
{
    int i, rc = 0;
    int streams;

    tc_debug("[%hhu] TCPSPDTEST: Release socket\n", stream_idx);

    if (NULL == sock)
    {
        tc_err("[%hhu] Socket already released!\n", stream_idx);
        return -1;
    }

    if (SS_CONNECTED != g_tcpspd[stream_idx].srv_socket->state)
    {
        tc_err("[%hhu] Try to release socket (state=%d) not in CONNECTED state! Release anyway.\n",
            stream_idx, g_tcpspd[stream_idx].srv_socket->state);
        rc = -1;
    }

    sock_learned[stream_idx] = 0;

    mdelay(WAIT_MSEC); /* Wait for FIN, ACK */

    /* Kernel Socket release */
    sock_release(sock);

    streams = atomic_dec_return(&connected_streams);
    if (streams == 0)
    {
        tcpspeedtest_on_set(0);

        /* Unregister netfilter hooks */
        for (i = 0; i < sizeof(nf_hks_tcp) / sizeof(struct nf_hook_ops); i++)
            nf_unregister_hook(&nf_hks_tcp[i]);
    }
    return rc;
}

/* Socket manager init */
int tcpspd_sock_mgr_init(void)
{
    blog_netdev_register_dummy(&_spdtst_netdev);

    return 0;
}

/* Shutdown the socket manager */
int tcpspd_sock_mgr_shutdown(uint8_t stream_idx)
{
    if (g_tcpspd[stream_idx].srv_socket)
    {
        /* Release Socket, unregister net filter hooks */
        tcpspd_sock_mgr_release(stream_idx, g_tcpspd[stream_idx].srv_socket);
        g_tcpspd[stream_idx].srv_socket = NULL;
    }

    blog_netdev_unregister_dummy(&_spdtst_netdev);

    return 0;
}
