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

#include "helpers.h"
#include "netlink.h"
#include "buffer.h"
#include "rpl.h"
#include "dag.h"

struct peer *dag_peer_create(const struct in6_addr *addr)
{
	struct peer *peer;

	peer = mzalloc(sizeof(*peer));
	if (!peer)
		return NULL;

	memcpy(&peer->addr, addr, sizeof(peer->addr));
	peer->rank = UINT16_MAX;

	return peer;
}

struct child *dag_child_create(const struct in6_addr *addr,
			       const struct in6_addr *from)
{
	struct child *peer;

	peer = mzalloc(sizeof(*peer));
	if (!peer)
		return NULL;

	memcpy(&peer->addr, addr, sizeof(peer->addr));
	memcpy(&peer->from, from, sizeof(peer->from));

	return peer;
}

bool dag_is_peer(const struct peer *peer, const struct in6_addr *addr)
{
	if (!peer || !addr)
		return false;

	return !memcmp(&peer->addr, addr, sizeof(peer->addr));
}

bool dag_is_child(const struct child *peer, const struct in6_addr *addr)
{
	if (!peer || !addr)
		return false;

	return !memcmp(&peer->addr, addr, sizeof(peer->addr));
}

static struct child *dag_lookup_child(const struct dag *dag,
				      const struct in6_addr *addr)
{
	struct child *peer;

	list_for_each_entry(peer, &dag->childs, list) {
		if (dag_is_child(peer, addr))
			return peer;
	}

	return NULL;
}

struct child *dag_lookup_child_or_create(struct dag *dag,
					 const struct in6_addr *addr,
					 const struct in6_addr *from)
{
	struct child *peer;

	peer = dag_lookup_child(dag, addr);
	if (peer)
		return peer;

	peer = dag_child_create(addr, from);
	if (peer)
		DL_APPEND(dag->childs.head, &peer->list);

	return peer;
}

static struct rpl *dag_lookup_rpl(const struct iface *iface,
				  uint8_t instance_id)
{
	struct rpl *rpl;

	list_for_each_entry(rpl, &iface->rpls, list) {
		if (rpl->instance_id == instance_id)
			return rpl;
	}

	return NULL;
}

static struct dag *dag_lookup_dodag(const struct rpl *rpl,
				    const struct in6_addr *dodagid)
{
	struct dag *dag;

	list_for_each_entry(dag, &rpl->dags, list) {
		if (!memcmp(&dag->dodagid, dodagid, sizeof(dag->dodagid)))
			return dag;
	}

	return NULL;
}

struct dag *dag_lookup(const struct iface *iface, uint8_t instance_id,
		       const struct in6_addr *dodagid)
{
	struct rpl *rpl;

	rpl = dag_lookup_rpl(iface, instance_id);
	if (!rpl)
		return NULL;

	return dag_lookup_dodag(rpl, dodagid);
}

static struct rpl *dag_rpl_create(uint8_t instance_id)
{
	struct rpl *rpl;

	rpl = mzalloc(sizeof(*rpl));
	if (!rpl)
		return NULL;

	rpl->instance_id = instance_id;
	return rpl;
}

struct dag_daoack *dag_lookup_daoack(const struct dag *dag, uint8_t dsn)
{
	struct dag_daoack *daoack;

	list_for_each_entry(daoack, &dag->pending_acks, list) {
		if (daoack->dsn == dsn)
			return daoack;
	}

	return NULL;
}

int dag_daoack_insert(struct dag *dag, uint8_t dsn)
{
	struct dag_daoack *daoack;

	daoack = mzalloc(sizeof(*daoack));
	if (!daoack)
		return -1;

	DL_APPEND(dag->pending_acks.head, &daoack->list);
	return 0;
}

void dag_init_timer(struct dag *dag);

static int dag_init(struct dag *dag, const struct iface *iface,
		    const struct rpl *rpl, const struct in6_addr *dodagid,
		    ev_tstamp trickle_t, uint16_t my_rank, uint8_t version,
		    const struct in6_prefix *dest)
{
	/* TODO dest is currently necessary */
	if (!dag || !iface || !rpl || !dest)
		return -1;

	memset(dag, 0, sizeof(*dag));

	dag->iface = iface;
	dag->rpl = rpl;
	dag->dest = *dest;
	dag->dodagid = *dodagid;

	dag->version = version;
	dag->my_rank = my_rank;
	dag->trickle_t = DEFAULT_TICKLE_T;

	dag_init_timer(dag);

	return 0;
}

struct dag *dag_create(struct iface *iface, uint8_t instanceid,
		       const struct in6_addr *dodagid, ev_tstamp trickle_t,
		       uint16_t my_rank, uint8_t version,
		       const struct in6_prefix *dest)
{
	bool append_rpl = false;
	struct rpl *rpl;
	struct dag *dag;
	int rc;

	rpl = dag_lookup_rpl(iface, instanceid);
	if (!rpl) {
		rpl = dag_rpl_create(instanceid);
		if (!rpl)
			return NULL;

		append_rpl = true;
	}

	/* sanity check because it's just a list
	 * we must avoid duplicate entries
	 */
	if (!append_rpl) {
		dag = dag_lookup_dodag(rpl, dodagid);
		if (dag) {
			free(rpl);
			return NULL;
		}
	}

	dag = mzalloc(sizeof(*dag));
	if (!dag) {
		free(rpl);
		return NULL;
	}

	rc = dag_init(dag, iface, rpl, dodagid, trickle_t,
		      my_rank, version, dest);
	if (rc != 0) {
		free(dag);
		free(rpl);
		return NULL;
	}

	if (append_rpl)
		DL_APPEND(iface->rpls.head, &rpl->list);

	DL_APPEND(rpl->dags.head, &dag->list);
	return dag;
}

void dag_free(struct dag *dag)
{
	free(dag);
}

static int append_destprefix(const struct dag *dag, struct safe_buffer *sb)
{
	struct rpl_dio_destprefix diodp = {};
	uint8_t len;

	len = sizeof(diodp) - sizeof(diodp.rpl_dio_prefix) +
		bits_to_bytes(dag->dest.len);

	diodp.rpl_dio_type = 0x3;
//	diodp.rpl_dio_prf = RPL_DIO_PREFIX_AUTONOMOUS_ADDR_CONFIG_FLAG;
	/* TODO crazy calculation here */
	diodp.rpl_dio_len = len - 2;
	diodp.rpl_dio_prefixlen = dag->dest.len;
	diodp.rpl_dio_prefix = dag->dest.prefix;
	diodp.rpl_dio_route_lifetime = UINT32_MAX;
	safe_buffer_append(sb, &diodp, len);

	return 0;
}

static void dag_build_icmp(struct safe_buffer *sb, uint8_t code)
{
	struct icmp6_hdr nd_rpl_hdr = {
		.icmp6_type = ND_RPL_MESSAGE,
		.icmp6_code = code,
	};

	/* TODO 4 is a hack */
	safe_buffer_append(sb, &nd_rpl_hdr, sizeof(nd_rpl_hdr) - 4);
}

void dag_build_dio(struct dag *dag, struct safe_buffer *sb)
{
	struct nd_rpl_dio dio = {};

	dag_build_icmp(sb, ND_RPL_DAG_IO);

	dio.rpl_instanceid = dag->rpl->instance_id;
	dio.rpl_version = dag->version;
	dio.rpl_dtsn = dag->dtsn++;
	flog(LOG_INFO, "my_rank %d", dag->my_rank);
	dio.rpl_dagrank = htons(dag->my_rank);
	dio.rpl_mopprf = ND_RPL_DIO_GROUNDED | RPL_DIO_STORING_NO_MULTICAST << 3;
	dio.rpl_dagid = dag->dodagid;

	safe_buffer_append(sb, &dio, sizeof(dio));
	append_destprefix(dag, sb);
}

void dag_process_dio(struct dag *dag)
{
	struct in6_addr addr;
	int rc;

	rc = gen_stateless_addr(&dag->dest, &dag->iface->llinfo,
				&addr);
	if (rc == -1)
		return;

	rc = nl_add_addr(dag->iface->ifindex, &addr);
	if (rc == -1 && errno != EEXIST) {
		flog(LOG_ERR, "error add nl %d", errno);
		return;
	}
	rc = nl_del_route_via(dag->iface->ifindex, &dag->dest, NULL);
	if (rc == -1) {
		flog(LOG_ERR, "error del nl %d, %s", errno, strerror(errno));
		return;
	}

	memcpy(&dag->self, &addr, sizeof(dag->self));
}

void dag_build_dao_ack(struct dag *dag, struct safe_buffer *sb)
{
	struct nd_rpl_daoack dao = {};

	dag_build_icmp(sb, ND_RPL_DAO_ACK);

	dao.rpl_instanceid = dag->rpl->instance_id;
	dao.rpl_flags |= RPL_DAO_K_MASK;
	dao.rpl_flags |= RPL_DAO_D_MASK;
	dao.rpl_daoseq = dag->dsn;
	dao.rpl_dagid = dag->dodagid;

	safe_buffer_append(sb, &dao, sizeof(dao));
	flog(LOG_INFO, "build dao");
}

static int append_target(const struct in6_prefix *prefix,
			 struct safe_buffer *sb)
{
	struct rpl_dao_target target = {};
	uint8_t len;

	len = sizeof(target) - sizeof(target.rpl_dao_prefix) +
		bits_to_bytes(prefix->len);

	target.rpl_dao_type = RPL_DAO_RPLTARGET;
	/* TODO crazy calculation here */
	target.rpl_dao_len = 18;
	target.rpl_dao_prefixlen = prefix->len;
	target.rpl_dao_prefix = prefix->prefix;
	safe_buffer_append(sb, &target, len);

	return 0;
}

void dag_build_dao(struct dag *dag, struct safe_buffer *sb)
{
	struct nd_rpl_daoack daoack = {};
	struct in6_prefix prefix;
	const struct child *child;

	dag_build_icmp(sb, ND_RPL_DAO);

	daoack.rpl_instanceid = dag->rpl->instance_id;
	daoack.rpl_flags |= RPL_DAO_D_MASK;
	daoack.rpl_dagid = dag->dodagid;

	safe_buffer_append(sb, &daoack, sizeof(daoack));
	prefix.prefix = dag->self;
	prefix.len = 128;
	append_target(&prefix, sb);

	list_for_each_entry(child, &dag->childs, list) {
		prefix.prefix = child->addr;
		prefix.len = 128;

		append_target(&prefix, sb);
	}

	dag_daoack_insert(dag, dag->dsn);
	daoack.rpl_daoseq = dag->dsn++;
	flog(LOG_INFO, "build dao");
}

void dag_build_dis(struct safe_buffer *sb)
{
	struct nd_rpl_dis dis = {};

	dag_build_icmp(sb, ND_RPL_DAG_IS);

	safe_buffer_append(sb, &dis, sizeof(dis));
	flog(LOG_INFO, "build dis");
}
