/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/*
 *  IPv6 RPL-SR implementation
 *
 *  Author:
 *  Alexander Aring <alex.aring@gmail.com>
 *
 *
 *  This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 */

#ifndef _UAPI_LINUX_RPL_IPTUNNEL_H
#define _UAPI_LINUX_RPL_IPTUNNEL_H

enum {
	RPL_IPTUNNEL_UNSPEC,
	RPL_IPTUNNEL_SRH,
	__RPL_IPTUNNEL_MAX,
};
#define RPL_IPTUNNEL_MAX (__RPL_IPTUNNEL_MAX - 1)

#define RPL_IPTUNNEL_SRH_SIZE(srh) (((srh)->hdrlen + 1) << 3)

#endif
