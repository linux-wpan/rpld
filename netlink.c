/*
 *   Authors:
 *    Alexander Aring		<alex.aring@gmail.com>
 *
 *   This software is Copyright 2019 by the above mentioned author(s),
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <alex.aring@gmail.com>.
 */

#include <time.h>

#include <libmnl/libmnl.h>
#include <linux/rtnetlink.h>

#include "netlink.h"
#include "log.h"

static struct mnl_socket *nl;
static unsigned int portid;

int netlink_open()
{
	int rc;

	nl = mnl_socket_open(NETLINK_ROUTE);
	if (!nl) {
		perror("mnl_socket_open");
		return -1;
	}

	rc = mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID);
	if (rc < 0) {
		mnl_socket_close(nl);
		perror("mnl_socket_bind");
		return -1;
	}
	portid = mnl_socket_get_portid(nl);

	return 0;
}

void netlink_close()
{
	mnl_socket_close(nl);
}

static int data_attr_cb(const struct nlattr *attr, void *data)
{
	int type = mnl_attr_get_type(attr);
	const struct nlattr **tb = data;

	if (mnl_attr_type_valid(attr, IFLA_MAX) < 0)
		return MNL_CB_OK;

	switch(type) {
	case IFLA_LINK:
		if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0) {
			perror("mnl_attr_validate");
			return MNL_CB_ERROR;
		}
		break;
	case IFLA_ADDRESS:
		if (mnl_attr_validate(attr, MNL_TYPE_BINARY) < 0) {
			perror("mnl_attr_validate");
			return MNL_CB_ERROR;
		}
		break;
	default:
		break;
	}

	tb[type] = attr;
	return MNL_CB_OK;
}

static int data_cb(const struct nlmsghdr *nlh, void *data)
{
	struct ifinfomsg *ifm = mnl_nlmsg_get_payload(nlh);
	struct nlattr *tb[IFLA_MAX + 1] = {};
	struct iface_llinfo *llinfo = data;

	/* TODO this will not available on bluetooth */
	mnl_attr_parse(nlh, sizeof(*ifm), data_attr_cb, tb);
	if (tb[IFLA_LINK])
		llinfo->ifindex = mnl_attr_get_u32(tb[IFLA_LINK]);

	if (tb[IFLA_ADDRESS]) {
		llinfo->addr_len = mnl_attr_get_payload_len(tb[IFLA_ADDRESS]);
		llinfo->addr = mzalloc(llinfo->addr_len);
		if (!llinfo->addr)
			return MNL_CB_ERROR;

		memcpy(llinfo->addr, mnl_attr_get_payload(tb[IFLA_ADDRESS]),
		       llinfo->addr_len);
	}

	return MNL_CB_OK;
}

int nl_get_llinfo(uint32_t ifindex, struct iface_llinfo *llinfo)
{
	unsigned char buf[MNL_SOCKET_BUFFER_SIZE];
	struct ifinfomsg *ifm;
	struct nlmsghdr *nlh;
	unsigned int seq;
	int ret;

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= RTM_GETLINK;
	nlh->nlmsg_flags = NLM_F_REQUEST;
	nlh->nlmsg_seq = seq = time(NULL);
	ifm = mnl_nlmsg_put_extra_header(nlh, sizeof(*ifm));
	ifm->ifi_family = AF_UNSPEC;
	ifm->ifi_index = ifindex;

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		perror("mnl_socket_send");
		return -1;
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	if (ret == -1)
		return -1;

	return mnl_cb_run(buf, ret, seq, portid, data_cb, llinfo);
}

int nl_add_addr(uint32_t ifindex, const struct in6_addr *addr)
{
	unsigned char buf[MNL_SOCKET_BUFFER_SIZE];
	struct ifaddrmsg *ifm;
	struct nlmsghdr *nlh;
	unsigned int seq;
	int ret;

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= RTM_NEWADDR;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_ACK;
	nlh->nlmsg_seq = seq = time(NULL);
	ifm = mnl_nlmsg_put_extra_header(nlh, sizeof(*ifm));

	ifm->ifa_index = ifindex;
	ifm->ifa_family = AF_INET6;
	ifm->ifa_prefixlen = 64;
	ifm->ifa_flags = IFA_F_PERMANENT;
	ifm->ifa_scope = RT_SCOPE_UNIVERSE;

	mnl_attr_put(nlh, IFA_ADDRESS, sizeof(*addr), addr);

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		perror("mnl_socket_send");
		return -1;
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	if (ret == -1) {
		perror("mnl_socket_recvfrom");
		return -1;
	}

	return mnl_cb_run(buf, ret, seq, portid, NULL, NULL);
}

int nl_add_route_via(uint32_t ifindex, const struct in6_addr *dst,
		     const struct in6_addr *via)
{
	unsigned char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	struct rtmsg *rtm;
	unsigned int seq;
	int ret;

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= RTM_NEWROUTE;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_ACK;
	nlh->nlmsg_seq = seq = time(NULL);
	rtm = mnl_nlmsg_put_extra_header(nlh, sizeof(*rtm));

	rtm->rtm_family = AF_INET6;
	rtm->rtm_dst_len = 128;
	rtm->rtm_src_len = 0;
	rtm->rtm_tos = 0;
	rtm->rtm_protocol = RTPROT_STATIC;
	rtm->rtm_table = RT_TABLE_MAIN;
	rtm->rtm_type = RTN_UNICAST;
	/* is there any gateway? */
	rtm->rtm_scope = RT_SCOPE_UNIVERSE;
	rtm->rtm_flags = 0;

	mnl_attr_put(nlh, RTA_DST, sizeof(*dst), dst);
	mnl_attr_put(nlh, RTA_GATEWAY, sizeof(*via), via);
	mnl_attr_put_u32(nlh, RTA_OIF, ifindex);

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		perror("mnl_socket_send");
		return -1;
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	if (ret == -1) {
		perror("mnl_socket_recvfrom");
		return -1;
	}

	return mnl_cb_run(buf, ret, seq, portid, NULL, NULL);
}

int nl_add_route_default(uint32_t ifindex, const struct in6_addr *dst)
{
	unsigned char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	struct rtmsg *rtm;
	unsigned int seq;
	int ret;

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= RTM_NEWROUTE;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_ACK;
	nlh->nlmsg_seq = seq = time(NULL);
	rtm = mnl_nlmsg_put_extra_header(nlh, sizeof(*rtm));

	rtm->rtm_family = AF_INET6;
	rtm->rtm_dst_len = 0;
	rtm->rtm_src_len = 0;
	rtm->rtm_tos = 0;
	rtm->rtm_protocol = RTPROT_STATIC;
	rtm->rtm_table = RT_TABLE_MAIN;
	rtm->rtm_type = RTN_UNICAST;
	/* is there any gateway? */
	rtm->rtm_scope = RT_SCOPE_UNIVERSE;
	rtm->rtm_flags = 0;

	mnl_attr_put(nlh, RTA_GATEWAY, sizeof(*dst), dst);
	mnl_attr_put_u32(nlh, RTA_OIF, ifindex);

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		perror("mnl_socket_send");
		return -1;
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	if (ret == -1) {
		perror("mnl_socket_recvfrom");
		return -1;
	}

	return mnl_cb_run(buf, ret, seq, portid, NULL, NULL);
}

int nl_del_route_via(uint32_t ifindex, const struct in6_prefix *dst,
		     struct in6_addr *via)
{
	unsigned char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	struct rtmsg *rtm;
	unsigned int seq;
	int ret;

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= RTM_DELROUTE;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	nlh->nlmsg_seq = seq = time(NULL);
	rtm = mnl_nlmsg_put_extra_header(nlh, sizeof(*rtm));

	rtm->rtm_family = AF_INET6;
	rtm->rtm_dst_len = dst->len;
	rtm->rtm_src_len = 0;
	rtm->rtm_tos = 0;
	rtm->rtm_table = RT_TABLE_MAIN;
	/* is there any gateway? */
	rtm->rtm_flags = 0;

	mnl_attr_put(nlh, RTA_DST, sizeof(dst->prefix), &dst->prefix);
	if (via)
		mnl_attr_put(nlh, RTA_GATEWAY, sizeof(*via), via);
	mnl_attr_put_u32(nlh, RTA_OIF, ifindex);

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		perror("mnl_socket_send");
		return -1;
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	if (ret == -1) {
		perror("mnl_socket_recvfrom");
		return -1;
	}

	return mnl_cb_run(buf, ret, seq, portid, NULL, NULL);
}

/* TODO THIS WILL ADD A STATEFUL COMPRESSION ENTRY INTO THE KERNEL
 * BUT I WANT A NETLINK API BEFORE WE IMPLEMENT IT. IT MAKES SENSE
 * WE ONLY USE STATELESS TO CONFIGURE THE NETWORK.
 */
#if 0
int nl_stateful_add_compression_entry(uint32_t ifindex, const struct in6_prefix *pfx)
{

}
#endif
