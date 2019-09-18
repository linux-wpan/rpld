/*
 *   Authors:
 *    Alexander Aring		<alex.aring@gmail.com>
 *
 *   Original Authors:
 *    Pedro Roque		<roque@di.fc.ul.pt>
 *    Lars Fenneberg		<lf@elemental.net>
 *
 *   This software is Copyright 1996,1997,2019 by the above mentioned author(s),
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <reubenhwk@gmail.com>.
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>

#include "helpers.h"
#include "socket.h"
#include "config.h"
#include "log.h"
#include "rpl.h"

/* Note: these are applicable to receiving sockopts only */
#if defined IPV6_HOPLIMIT && !defined IPV6_RECVHOPLIMIT
#define IPV6_RECVHOPLIMIT IPV6_HOPLIMIT
#endif

#if defined IPV6_PKTINFO && !defined IPV6_RECVPKTINFO
#define IPV6_RECVPKTINFO IPV6_PKTINFO
#endif

static int rpl_multicast_handler(int sock, const struct list_head *ifaces,
				 int optname)
{
	struct ipv6_mreq mreq;
	struct iface *iface;
	struct list *e;
	int rc;

	DL_FOREACH(ifaces->head, e) {
		iface = container_of(e, struct iface, list);

		memset(&mreq, 0, sizeof(mreq));

		mreq.ipv6mr_interface = iface->ifindex;
		/* all-rpl-nodes: ff02::1a */
	        memcpy(&mreq.ipv6mr_multiaddr.s6_addr[0], &all_rpl_addr,
		       sizeof(mreq.ipv6mr_multiaddr));

	        rc = setsockopt(sock, SOL_IPV6, optname, &mreq,
				sizeof(mreq));
		if (rc < 0)
			return -1;

		flog(LOG_INFO, "interface %s %s to rpl multicast",
		     iface->ifname, (optname == IPV6_ADD_MEMBERSHIP)?
		     "subscribe":"unsubscribe");
	}

	return 0;
}

static int subscribe_rpl_multicast(int sock, const struct list_head *ifaces)
{
	return rpl_multicast_handler(sock, ifaces, IPV6_ADD_MEMBERSHIP);
}

static void unsubscribe_rpl_multicast(int sock, const struct list_head *ifaces)
{
	rpl_multicast_handler(sock, ifaces, IPV6_DROP_MEMBERSHIP);
}

int open_icmpv6_socket(const struct list_head *ifaces)
{
	struct icmp6_filter filter;
	int sock;
	int err;

	sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if (sock < 0) {
		flog(LOG_ERR, "can't create socket(AF_INET6): %s", strerror(errno));
		return -1;
	}

	err = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, (int[]){1}, sizeof(int));
	if (err < 0) {
		flog(LOG_ERR, "setsockopt(IPV6_RECVPKTINFO): %s", strerror(errno));
		close(sock);
		return -1;
	}

	err = setsockopt(sock, IPPROTO_RAW, IPV6_CHECKSUM, (int[]){2}, sizeof(int));
	if (err < 0) {
		flog(LOG_ERR, "setsockopt(IPV6_CHECKSUM): %s", strerror(errno));
		close(sock);
		return -1;
	}

	err = setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, (int[]){255}, sizeof(int));
	if (err < 0) {
		flog(LOG_ERR, "setsockopt(IPV6_UNICAST_HOPS): %s", strerror(errno));
		close(sock);
		return -1;
	}

	err = setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (int[]){1}, sizeof(int));
	if (err < 0) {
		flog(LOG_ERR, "setsockopt(IPV6_MULTICAST_HOPS): %s", strerror(errno));
		close(sock);
		return -1;
	}

	err = setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (int[]){0}, sizeof(int));
	if (err < 0) {
		flog(LOG_ERR, "setsockopt(IPV6_MULTICAST_LOOP): %s", strerror(errno));
		close(sock);
		return -1;
	}

#ifdef IPV6_RECVHOPLIMIT
	err = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, (int[]){1}, sizeof(int));
	if (err < 0) {
		flog(LOG_ERR, "setsockopt(IPV6_RECVHOPLIMIT): %s", strerror(errno));
		close(sock);
		return -1;
	}
#endif

	err = subscribe_rpl_multicast(sock, ifaces);
	if (err < 0) {
		flog(LOG_ERR, "Failed to subscribe rpl multicast: %s", strerror(errno));
		close(sock);
		return -1;
	}

	ICMP6_FILTER_SETBLOCKALL(&filter);
	ICMP6_FILTER_SETPASS(ND_RPL_MESSAGE,    &filter);
#if 0
	ICMP6_FILTER_SETPASS(ND_ROUTER_SOLICIT, &filter);
	ICMP6_FILTER_SETPASS(ND_ROUTER_ADVERT,  &filter);
	ICMP6_FILTER_SETPASS(ND_NEIGHBOR_SOLICIT, &filter);
	ICMP6_FILTER_SETPASS(ND_NEIGHBOR_ADVERT,  &filter);
#endif

	return sock;
}

void close_icmpv6_socket(int sock, const struct list_head *ifaces)
{
	unsubscribe_rpl_multicast(sock, ifaces);
	close(sock);
}
