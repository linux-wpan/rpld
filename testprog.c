#include <assert.h>
#include <stdio.h>

#include "tree.h"

void test_tree(void)
{
	struct in6_addr dummy = { .s6_addr = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } };
	struct in6_addr s0 = { .s6_addr = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } };
	struct in6_addr s1 = { .s6_addr = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 } };
	struct in6_addr s2 = { .s6_addr = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3 } };
	struct in6_addr s3 = { .s6_addr = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4 } };
	struct in6_addr s4 = { .s6_addr = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5 } };
	struct list_head path = {};
	struct t_root root = {};
	struct t_node *n;
	struct t_path *p;

	t_init(&root, &s0, &dummy);
	assert(t_insert(&root, &s1, &s2, &dummy) == NULL);
	t_insert(&root, &s0, &s1, &dummy);
	t_insert(&root, &s1, &s3, &dummy);
	t_insert(&root, &s0, &s4, &dummy);
	n = t_insert(&root, &s3, &s2, &dummy);

	t_path(n, &path);

	list_for_each_entry(p, &path, list)
		printf("FFF %d\n", p->addr.s6_addr[15]);

	t_path_free(&path);
	t_free(&root);
}

struct test_entry {
	int x;
	struct list list;
};

void test_list(void)
{
	struct list_head head = {};
	struct test_entry e = { .x = 5 };
	struct test_entry y = { .x = 6 };
	struct test_entry z = { .x = 7 };
	struct test_entry *x, *tmp;
	int i;

	assert(list_empty(&head));
	assert(list_first_entry(&head, struct test_entry, list) == NULL);

	list_for_each_entry(x, &head, list)
		assert(x->x == 5);

	list_add(&head, &e.list);
	assert(!list_empty(&head));
	assert(list_first_entry(&head, struct test_entry, list) != NULL);
	assert(list_first_entry(&head, struct test_entry, list)->x == 5);
	x = list_first_entry(&head, struct test_entry, list);
	assert(!list_next_entry(x, list));

	list_add(&head, &y.list);
	list_add(&head, &z.list);
	i = 0;
	list_for_each_entry(x, &head, list) {
		switch (i) {
		case 0:
			assert(x->x == 5);
			break;
		case 1:
			assert(x->x == 6);
			break;
		case 2:
			assert(x->x == 7);
			break;
		default:
			assert(0);
			break;
		}

		i++;
	}

	assert(i == 3);

	list_for_each_entry_safe(x, tmp, &head, list)
	       list_del(&head, &x->list);

	assert(list_empty(&head));
}

int main(int argc, const char *argv[])
{
	test_list();
	test_tree();
	return 0;
}
