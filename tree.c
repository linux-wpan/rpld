/*
 *   Authors:
 *    Alexander Aring		<alex.aring@gmail.com>
 *
 *   This software is Copyright 2020 by the above mentioned author(s),
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <alex.aring@gmail.com>.
 */

#include "tree.h"

struct t_node *t_node_alloc(const struct in6_addr *addr,
			    const struct in6_addr *target)
{
	struct t_node *n;

	n = mzalloc(sizeof(*n));
	if (!n)
		return NULL;

	n->addr = *addr;
	n->target = *target;
	return n;
}

static struct t_node *t_insert_tail(struct t_node *node, const struct in6_addr *parent,
				    const struct in6_addr *addr,
				    const struct in6_addr *target)
{
	struct t_node *n, *res = NULL;

	if (!memcmp(&node->addr, parent, sizeof(node->addr))) {
		n = t_node_alloc(addr, target);
		if (!n)
			return NULL;

		n->parent = node;
		list_add(&node->childs, &n->list);
		return n;
	}

	list_for_each_entry(n, &node->childs, list) {
		res = t_insert_tail(n, parent, addr, target);
		if (res)
			break;
	}

	return res;
}

struct t_node *t_insert(struct t_root *root, const struct in6_addr *parent,
			const struct in6_addr *addr,
			const struct in6_addr *target)
{
	return t_insert_tail(root->n, parent, addr, target);
}

void t_init(struct t_root *root, const struct in6_addr *addr,
	    const struct in6_addr *dodagid)
{
	root->n = t_node_alloc(addr, dodagid);
}

static void t_free_tail(struct t_node *node)
{
	struct t_node *n, *tmp;

	list_for_each_entry_safe(n, tmp, &node->childs, list) {
		list_del(&node->childs, &n->list);
		t_free_tail(n);
	}

	free(node);
}

void t_free(struct t_root *root)
{
	t_free_tail(root->n);
}

static struct t_path *t_path_alloc(const struct in6_addr *addr,
				   const struct in6_addr *target)
{
	struct t_path *path;

	path = mzalloc(sizeof(*path));
	if (!path)
		return NULL;

	path->addr = *addr;
	path->target = *target;

	return path;
}

int t_path(const struct t_node *node, struct list_head *result)
{
	struct t_path *p;

	if (!node)
		return 0;

	p = t_path_alloc(&node->addr, &node->target);
	if (!p) {
		/* TODO memleak */
		return -ENOMEM;
	}

	list_add(result, &p->list);

	return t_path(node->parent, result);
}

void t_path_free(struct list_head *head)
{
	struct t_path *p, *tmp;

	list_for_each_entry_safe(p, tmp, head, list) {
	       list_del(head, &p->list);
	       free(p);
	}
}
