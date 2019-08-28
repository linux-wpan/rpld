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

#ifndef __RPLD_NETLINK_H__
#define __RPLD_NETLINK_H__

#include "config.h"

int nl_add_addr(uint32_t ifindex, const struct in6_addr *addr);
int nl_get_llinfo(uint32_t ifindex, struct iface_llinfo *llinfo);
int nl_add_route_via(uint32_t ifindex, const struct in6_addr *route,
		     const struct in6_addr *via);
int nl_add_route_default(uint32_t ifindex, const struct in6_addr *via);
int nl_del_route_via(uint32_t ifindex, const struct in6_prefix *dst,
		     struct in6_addr *via);
int netlink_open(void);
void netlink_close(void);

#endif /* __RPLD_NETLINK_H__ */
