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

#ifndef __RPLD_DAG_H__
#define __RPLD_DAG_H__

#include <stdint.h>
#include <string.h>
#include <ev.h>

#include "buffer.h"
#include "list.h"
#include "tree.h"

struct peer {
	struct in6_addr addr;
	uint16_t rank;

	struct list list;
};

struct child {
	struct in6_addr addr;
	struct in6_addr from;

	struct list list;
};

struct dag_daoack {
	uint8_t dsn;

	struct list list;
};

struct dag {
	uint8_t version;
	/* trigger */
	uint8_t dtsn;
	/* if changed */
	uint8_t dsn;
	struct in6_addr dodagid;
	uint8_t mop;

	struct t_root root;
	struct in6_prefix dest;

	uint16_t my_rank;
	struct peer *parent;

	/* routable self address */
	struct in6_addr self;
	/* routable childs, if .head NULL -> leaf */
	struct list_head childs;

	ev_tstamp trickle_t;
	ev_timer trickle_w;

	/* iface which dag belongs to */
	const struct iface *iface;
	/* rpl instance which dag belongs to */
	const struct rpl *rpl;

	/* TODO dodagid is optional so move it to instance?
	 *
	 * No idea how it works when it's not given. We don't
	 * support it.
	 */
	struct list_head pending_acks;

	struct list list;
};

struct rpl {
	uint8_t instance_id;

	/* set of dags, unique key is dodagid */
	struct list_head dags;

	struct list list;
};

struct dag *dag_create(struct iface *iface, uint8_t instanceid,
		       const struct in6_addr *dodagid, ev_tstamp trickle_t,
		       uint16_t my_rank, uint8_t version,
		       uint8_t mop, const struct in6_prefix *dest);
void dag_free(struct dag *dag);
void dag_build_dio(struct dag *dag, struct safe_buffer *sb);
struct dag *dag_lookup(const struct iface *iface, uint8_t instance_id,
		       const struct in6_addr *dodagid);
void dag_process_dio(struct dag *dag);
struct peer *dag_peer_create(const struct in6_addr *addr);
void dag_build_dao(struct dag *dag, struct safe_buffer *sb);
void dag_build_dao_ack(struct dag *dag, struct safe_buffer *sb);
void dag_build_dis(struct safe_buffer *sb);
struct child *dag_lookup_child_or_create(struct dag *dag,
					 const struct in6_addr *addr,
					 const struct in6_addr *from);
bool dag_is_peer(const struct peer *peer, const struct in6_addr *addr);

#endif /* __RPLD_DAG_H__ */
