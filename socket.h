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

#ifndef __RPLD_SOCKET_H__
#define __RPLD_SOCKET_H__

#include "list.h"

int open_icmpv6_socket(const struct list_head *ifaces);
void close_icmpv6_socket(int sock, const struct list_head *ifaces);

#endif /* __RPLD_SOCKET_H__ */
