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

#ifndef __RPLD_PROCESS_H__
#define __RPLD_PROCESS_H__

#include <linux/ipv6.h>
#include <netinet/in.h>

#include "config.h"

void process(int sock, struct list_head *ifaces, unsigned char *msg,
	     int len, struct sockaddr_in6 *addr, struct in6_pktinfo *pkt_info,
	     int hoplimit);

#endif /* __RPLD_PROCESS_H__ */
