/*
 *   Authors:
 *    Alexander Aring		<alex.aring@gmail.com>
 *
 *   Original Authors:
 *    Lars Fenneberg		<lf@elemental.net>
 *
 *   This software is Copyright 1996,1997,2019 by the above mentioned author(s),
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <alex.aring@gmail.com>.
 */

#include <linux/ipv6.h>
#include <netinet/icmp6.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "helpers.h"
#include "buffer.h"
#include "config.h"
#include "config.h"
#include "send.h"
#include "log.h"
#include "rpl.h"

static int really_send(int sock, const struct iface *iface,
		       const struct in6_addr *dest,
		       const struct safe_buffer *sb)
{
	struct sockaddr_in6 addr;
	memset((void *)&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(IPPROTO_ICMPV6);
	memcpy(&addr.sin6_addr, dest, sizeof(struct in6_addr));

	struct iovec iov;
	iov.iov_len = sb->used;
	iov.iov_base = (caddr_t)sb->buffer;

	char __attribute__((aligned(8))) chdr[CMSG_SPACE(sizeof(struct in6_pktinfo))];
	memset(chdr, 0, sizeof(chdr));
	struct cmsghdr *cmsg = (struct cmsghdr *)chdr;

	cmsg->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
	cmsg->cmsg_level = IPPROTO_IPV6;
	cmsg->cmsg_type = IPV6_PKTINFO;

	struct in6_pktinfo *pkt_info = (struct in6_pktinfo *)CMSG_DATA(cmsg);
	pkt_info->ipi6_ifindex = iface->ifindex;
	memcpy(&pkt_info->ipi6_addr, iface->ifaddr_src, sizeof(struct in6_addr));

#if 1
//#ifdef HAVE_SIN6_SCOPE_ID
	if (IN6_IS_ADDR_LINKLOCAL(&addr.sin6_addr) || IN6_IS_ADDR_MC_LINKLOCAL(&addr.sin6_addr))
		addr.sin6_scope_id = iface->ifindex;
//#endif
#endif

	struct msghdr mhdr;
	memset(&mhdr, 0, sizeof(mhdr));
	mhdr.msg_name = (caddr_t)&addr;
	mhdr.msg_namelen = sizeof(struct sockaddr_in6);
	mhdr.msg_iov = &iov;
	mhdr.msg_iovlen = 1;
	mhdr.msg_control = (void *)cmsg;
	mhdr.msg_controllen = sizeof(chdr);

	return sendmsg(sock, &mhdr, 0);
}
void send_dio(int sock, struct dag *dag)
{
	struct safe_buffer *sb;
	int rc;

	sb = safe_buffer_new();
	if (!sb)
		return;

	dag_build_dio(dag, sb);
	rc = really_send(sock, dag->iface, &all_rpl_addr, sb);
	flog(LOG_INFO, "foo! %s %d %s", dag->iface->ifname, rc ,strerror(errno));
}

void send_dao(int sock, const struct in6_addr *to, struct dag *dag)
{
	struct safe_buffer *sb;
	int rc;

	sb = safe_buffer_new();
	if (!sb)
		return;

	dag_build_dao(dag, sb);
	rc = really_send(sock, dag->iface, to, sb);
	flog(LOG_INFO, "send_dao! %d", rc);
}

void send_dao_ack(int sock, const struct in6_addr *to, struct dag *dag)
{
	struct safe_buffer *sb;
	int rc;

	sb = safe_buffer_new();
	if (!sb)
		return;

	dag_build_dao_ack(dag, sb);
	rc = really_send(sock, dag->iface, to, sb);
	flog(LOG_INFO, "send_dao_ack! %d", rc);
}

void send_dis(int sock, struct iface *iface)
{
	struct safe_buffer *sb;
	int rc;

	sb = safe_buffer_new();
	if (!sb)
		return;

	dag_build_dis(sb);
	rc = really_send(sock, iface, &all_rpl_addr, sb);
	flog(LOG_INFO, "send_dis! %d", rc);
}
