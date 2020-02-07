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

#ifndef __RPLD_LIST_H__
#define __RPLD_LIST_H__

#include "helpers.h"
#include "utlist.h"

/* TODO I really want to improve this wrapper API to send it upstream in utlist
 * as an "option" to use. I need to add more static inline wrapper here.
 */

/* utlist is great but the API is shit, so make it like linux
 * specially I HATE the head handling, which is a assing which
 * introduce a lot of weird behaviours when you don't pass head
 * as ** as function who deals with it when the list is empty
 * as NULL, weird is when not NULL you don't have issues.
 *
 * Sorry, but I said, it's a great implementation. :-)
 * This makes it like linux API which doesn't have this issue
 * because pointers are packed and using of container_of()
 */
struct list_head {
	struct list *head;
};

struct list {
	struct list *prev;
	struct list *next;
};

static inline void list_add(struct list_head *list_head, struct list *entry)
{
	DL_APPEND(list_head->head, entry);
}

/* TODO Not 100% linux api may we can get rid of head arg */
static inline void list_del(struct list_head *list_head, struct list *entry)
{
	DL_DELETE(list_head->head, entry);
}

static inline int list_empty(const struct list_head *list_head)
{
	return !list_head->head;
}

#define list_first_entry(list_head, type, member)							\
	((!(list_head)->head)?(NULL):(container_of((list_head)->head, type, member)))

#define list_next_entry(pos, member)									\
	((!(pos)->member.next)?(NULL):(container_of((pos)->member.next, typeof(*(pos)), member)))

#define list_for_each_entry(pos, list_head, member)							\
    for ((pos) = list_first_entry(list_head, typeof(*(pos)), member); pos;				\
	 (pos) = list_next_entry(pos, list))

#define list_for_each_entry_safe(pos, n, list_head, member)						\
    for ((pos) = list_first_entry(list_head, typeof(*(pos)), member);					\
	 (pos) && ((n) = list_next_entry(pos, list), 1);					\
	 (pos) = (n))

#endif /* __RPLD_LIST_H__ */
