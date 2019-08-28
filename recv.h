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

#ifndef __RPLD_RECV_H__
#define __RPLD_RECV_H__

#include <linux/ipv6.h>
#include <netinet/in.h>

#define MSG_SIZE_RECV 1500

int recv_rs_ra(int sock, unsigned char *msg, struct sockaddr_in6 *addr,
	       struct in6_pktinfo **pkt_info, int *hoplimit,
	       unsigned char *chdr);

#endif /* __RPLD_RECV_H__ */
