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

#include "process.h"
#include "netlink.h"
#include "send.h"
#include "dag.h"
#include "log.h"
#include "rpl.h"

static void process_dio(int sock, struct iface *iface, const void *msg,
			size_t len, struct sockaddr_in6 *addr)
{
	const struct nd_rpl_dio *dio = msg;
	const struct rpl_dio_destprefix *diodp;
	char addr_str[INET6_ADDRSTRLEN];
	struct in6_prefix pfx;
	struct dag *dag;
	uint16_t rank;

	if (len < sizeof(*dio)) {
		flog(LOG_INFO, "dio length mismatch, drop");
		return;
	}
	len -= sizeof(*dio);

	addrtostr(&addr->sin6_addr, addr_str, sizeof(addr_str));
	flog(LOG_INFO, "received dio %s", addr_str);

	dag = dag_lookup(iface, dio->rpl_instanceid,
			 &dio->rpl_dagid);
	if (dag) {
		if (dag->my_rank == 1)
			return;
	} else {
		diodp = (struct rpl_dio_destprefix *)
			 (((unsigned char *)msg) + sizeof(*dio));

		if (len < sizeof(*diodp) - 16) {
			flog(LOG_INFO, "diodp length mismatch, drop");
			return;
		}
		len -= sizeof(*diodp) - 16;

		if (diodp->rpl_dio_type != 0x3) {
			flog(LOG_INFO, "we assume diodp - not supported, drop");
			return;
		}

		if (len < bits_to_bytes(diodp->rpl_dio_prefixlen)) {
			flog(LOG_INFO, "diodp prefix length mismatch, drop");
			return;
		}
		len -= bits_to_bytes(diodp->rpl_dio_prefixlen);

		pfx.len = diodp->rpl_dio_prefixlen;
		memcpy(&pfx.prefix, &diodp->rpl_dio_prefix,
		       bits_to_bytes(pfx.len));

		flog(LOG_INFO, "received but no dag found %s", addr_str);
		dag = dag_create(iface, dio->rpl_instanceid,
				 &dio->rpl_dagid, DEFAULT_TICKLE_T,
				 UINT16_MAX, dio->rpl_version,
				 RPL_DIO_MOP(dio->rpl_mopprf), &pfx);
		if (!dag)
			return;

		addrtostr(&dio->rpl_dagid, addr_str, sizeof(addr_str));
		flog(LOG_INFO, "created dag %s", addr_str);
	}

	flog(LOG_INFO, "process dio %s", addr_str);

	rank = ntohs(dio->rpl_dagrank);
	if (!dag->parent) {
		dag->parent = dag_peer_create(&addr->sin6_addr);
		if (!dag->parent)
			return;
	}

	if (rank > dag->parent->rank)
		return;

	dag->parent->rank = rank;
	dag->my_rank = rank + 1;

	dag_process_dio(dag);

	send_dao(sock, &dag->parent->addr, dag);
}

static void process_dao(int sock, struct iface *iface, const void *msg,
			size_t len, struct sockaddr_in6 *addr)
{
	const struct rpl_dao_target *target;
	const struct nd_rpl_dao *dao = msg;
	char addr_str[INET6_ADDRSTRLEN];
	const struct nd_rpl_opt *opt;
	const unsigned char *p;
	struct child *child;
	struct dag *dag;
	int optlen;
	int rc;

	if (len < sizeof(*dao)) {
		flog(LOG_INFO, "dao length mismatch, drop");
		return;
	}
	len -= sizeof(*dao);

	addrtostr(&addr->sin6_addr, addr_str, sizeof(addr_str));
	flog(LOG_INFO, "received dao %s", addr_str);

	dag = dag_lookup(iface, dao->rpl_instanceid,
			 &dao->rpl_dagid);
	if (!dag) {
		addrtostr(&dao->rpl_dagid, addr_str, sizeof(addr_str));
		flog(LOG_INFO, "can't find dag %s", addr_str);
		return;
	}

	p = msg;
	p += sizeof(*dao);
	optlen = len;
	flog(LOG_INFO, "dao optlen %d", optlen);
	while (optlen > 0) {
		opt = (const struct nd_rpl_opt *)p;

		if (optlen < sizeof(*opt)) {
			flog(LOG_INFO, "rpl opt length mismatch, drop");
			return;
		}

		flog(LOG_INFO, "dao opt %d", opt->type);
		switch (opt->type) {
		case RPL_DAO_RPLTARGET:
			target = (const struct rpl_dao_target *)p;
			if (optlen < sizeof(*opt)) {
				flog(LOG_INFO, "rpl target length mismatch, drop");
				return;
			}

			addrtostr(&target->rpl_dao_prefix, addr_str, sizeof(addr_str));
			flog(LOG_INFO, "dao target %s", addr_str);
			dag_lookup_child_or_create(dag,
						   &target->rpl_dao_prefix,
						   &addr->sin6_addr);
			break;
		default:
			/* IGNORE NOT SUPPORTED */
			break;
		}

		/* TODO critical, we trust opt->len here... which is wire data */
		optlen -= (2 + opt->len);
		p += (2 + opt->len);
		flog(LOG_INFO, "dao optlen %d", optlen);
	}

	list_for_each_entry(child, &dag->childs, list) {
		rc = nl_add_route_via(dag->iface->ifindex, &child->addr,
				      &child->from);
		flog(LOG_INFO, "via route %d %s", rc, strerror(errno));
	}

	flog(LOG_INFO, "process dao %s", addr_str);
	send_dao_ack(sock, &addr->sin6_addr, dag);
}

static void process_daoack(int sock, struct iface *iface, const void *msg,
			   size_t len, struct sockaddr_in6 *addr)
{
	const struct nd_rpl_daoack *daoack = msg;
	char addr_str[INET6_ADDRSTRLEN];
	struct dag *dag;
	int rc;

	if (len < sizeof(*daoack)) {
		flog(LOG_INFO, "rpl daoack length mismatch, drop");
		return;
	}

	addrtostr(&addr->sin6_addr, addr_str, sizeof(addr_str));
	flog(LOG_INFO, "received daoack %s", addr_str);

	dag = dag_lookup(iface, daoack->rpl_instanceid,
			 &daoack->rpl_dagid);
	if (!dag) {
		addrtostr(&daoack->rpl_dagid, addr_str, sizeof(addr_str));
		flog(LOG_INFO, "can't find dag %s", addr_str);
		return;
	}

	if (dag->parent) {
		rc = nl_add_route_default(dag->iface->ifindex, &dag->parent->addr);
		flog(LOG_INFO, "default route %d %s", rc, strerror(errno));
	}

}

static void process_dis(int sock, struct iface *iface, const void *msg,
			size_t len, struct sockaddr_in6 *addr)
{
	char addr_str[INET6_ADDRSTRLEN];
	struct rpl *rpl;
	struct dag *dag;

	addrtostr(&addr->sin6_addr, addr_str, sizeof(addr_str));
	flog(LOG_INFO, "received dis %s", addr_str);

	list_for_each_entry(rpl, &iface->rpls, list) {
		list_for_each_entry(dag, &rpl->dags, list)
			send_dio(sock, dag);
	}
}

void process(int sock, struct list_head *ifaces, unsigned char *msg,
	     int len, struct sockaddr_in6 *addr, struct in6_pktinfo *pkt_info,
	     int hoplimit)
{
	char addr_str[INET6_ADDRSTRLEN];
	char if_namebuf[IFNAMSIZ] = {""};
	char *if_name = if_indextoname(pkt_info->ipi6_ifindex, if_namebuf);
	if (!if_name) {
		if_name = "unknown interface";
	}
	dlog(LOG_DEBUG, 4, "%s received a packet", if_name);

	addrtostr(&addr->sin6_addr, addr_str, sizeof(addr_str));

	if (!pkt_info) {
		flog(LOG_WARNING, "%s received packet with no pkt_info from %s!", if_name, addr_str);
		return;
	}

	/*
	 * can this happen?
	 */

	if (len < 4) {
		flog(LOG_WARNING, "%s received icmpv6 packet with invalid length (%d) from %s", if_name, len, addr_str);
		return;
	}
	len -= 4;

	struct icmp6_hdr *icmph = (struct icmp6_hdr *)msg;
	struct iface *iface = iface_find_by_ifindex(ifaces, pkt_info->ipi6_ifindex);
	if (!iface) {
		dlog(LOG_WARNING, 4, "%s received icmpv6 RS/RA packet on an unknown interface with index %d", if_name,
		     pkt_info->ipi6_ifindex);
		return;
	}

	if (icmph->icmp6_type != ND_RPL_MESSAGE) {
		/*
		 *      We just want to listen to RPL
		 */

		flog(LOG_ERR, "%s icmpv6 filter failed", if_name);
		return;
	}

	switch (icmph->icmp6_code) {
	case ND_RPL_DAG_IS:
		process_dis(sock, iface, &icmph->icmp6_dataun, len, addr);
		break;
	case ND_RPL_DAG_IO:
		process_dio(sock, iface, &icmph->icmp6_dataun, len, addr);
		break;
	case ND_RPL_DAO:
		process_dao(sock, iface, &icmph->icmp6_dataun, len, addr);
		break;
	case ND_RPL_DAO_ACK:
		process_daoack(sock, iface, &icmph->icmp6_dataun, len, addr);
		break;
	default:
		flog(LOG_ERR, "%s received unsupported RPL code 0x%02x",
		     if_name, icmph->icmp6_code);
		break;
	}
}
