#ifndef __RPL_TREE_H__
#define __RPL_TREE_H__

#include <arpa/inet.h>

#include "list.h"

struct t_path {
	struct in6_addr addr;
	struct in6_addr target;

	struct list list;
};

struct t_node {
	struct in6_addr addr;
	struct in6_addr target;

	struct t_node *parent;
	struct list_head childs;

	struct list list;
};

struct t_root {
	struct t_node *n;
};

void t_init(struct t_root *root, const struct in6_addr *addr,
	    const struct in6_addr *target);
struct t_node *t_insert(struct t_root *root, const struct in6_addr *parent,
			const struct in6_addr *addr,
			const struct in6_addr *target);
void t_free(struct t_root *root);

int t_path(const struct t_node *node, struct list_head *result);
void t_path_free(struct list_head *head);

#endif /* __RPL_TREE_H__ */
