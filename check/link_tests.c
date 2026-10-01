#include <check.h>
#include <stdlib.h>
#include <sys/types.h>
#include <any.h>

// Every library function is static inline, so a type can be instantiated in
// as many translation units as needed.  This file instantiates the same
// prefixes as array_tests.c, linked_list_tests.c, tree_tests.c, chan_tests.c,
// hash_table_tests.c, phash_table_tests.c and pqueue_tests.c.  They are all
// linked into one program, which would fail with "multiple definition" errors
// if any of these functions had external linkage.

#define data_t int
#define prefix int
#include <array.h>
#undef data_t
#undef prefix

#define data_t int
#define prefix int
#include <tree.h>
#undef data_t
#undef prefix

#define data_t int
#define prefix int
#define null_value -1
#include <linked_list.h>
#undef null_value
#undef data_t
#undef prefix

#define data_t int
#define prefix int
#include <chan.h>
#undef data_t
#undef prefix

#define hkey_t int
#define value_t int
#define prefix hi
#include <hash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

#define hkey_t int
#define value_t int
#define prefix pi
#include <phash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

#define data_t int
#define prefix pqi
#include <pqueue.h>
#undef data_t
#undef prefix

START_TEST(link_test1)

array_int_t *a = array_int_init();
for (int i = 5; i > 0; i--) {
  array_int_append(a, i);
}
CHECK(array_int_sort(a) == 0);
CHECK(array_int_get(a, 0) == 1);
CHECK(array_int_get(a, 4) == 5);
array_int_destroy(&a);

tree_int_t *t = tree_int_init();
for (int i = 0; i < 100; i++) {
  tree_int_insert(t, i * 7 % 100, NULL);
}
CHECK(tree_int_size(t) == 100);
CHECK(tree_int_delete_min(t).key == 0);
CHECK(tree_int_delete_max(t).key == 99);
tree_int_destroy(&t);

llist_int_t *l = llist_int_init();
for (int i = 5; i > 0; i--) {
  llist_int_add_end(l, i);
}
CHECK(llist_int_msort(l) == 0);
CHECK(llist_int_remove_start(l) == 1);
CHECK(llist_int_remove_end(l) == 5);
llist_int_destroy(&l);

END_TEST

START_TEST(link_test2)

htable_hi_t *h = hash_hi_init(0);
CHECK(hash_hi_put(h, 1, 10) == 0);
int v = 0;
CHECK(hash_hi_get(h, 1, &v) == 1);
CHECK(v == 10);
hash_hi_destroy(&h);

phtable_pi_t *p = phash_pi_init(0);
CHECK(phash_pi_put(p, 2, 20) == 0);
CHECK(phash_pi_get(p, 2, &v) == 1);
CHECK(v == 20);
phash_pi_destroy(&p);

pqueue_pqi_t *q = pqueue_pqi_init();
pqueue_pqi_push(q, 3, NULL);
pqueue_pqi_push(q, 8, NULL);
CHECK(pqueue_pqi_pop(q).key == 8);
pqueue_pqi_destroy(&q);

chan_int_t *c = chan_int_init(4);
CHECK(chan_int_send(c, 42) == CHAN_SUCCESS);
CHECK(chan_int_recv(c, &v) == CHAN_SUCCESS);
CHECK(v == 42);
chan_int_destroy(&c);

any_t x = any_init(7);
CHECK(any_get_type(x) == ANY_I32);

END_TEST
