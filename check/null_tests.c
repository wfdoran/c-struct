#include <check.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

// A NULL container is an error, not a crash: functions which return a status
// return -1, functions which return a count return 0, iterators report that
// there is nothing, and lookups report not found.

#define data_t int
#define prefix int
#include <array.h>
#include <tree.h>
#include <pqueue.h>
#include <option.h>
#undef data_t
#undef prefix

#define data_t int
#define prefix int
#define null_value -1
#include <linked_list.h>
#undef data_t
#undef prefix

#define hkey_t int
#define value_t int
#define prefix hi
#include <hash_table.h>
#undef hkey_t
#undef value_t
#undef prefix

#define hkey_t int
#define value_t int
#define prefix pi
#include <phash_table.h>
#undef hkey_t
#undef value_t
#undef prefix

static int comp_int_p(int *a, int *b) {
  return *a - *b;
}
static int comp_hkey(int a, int b) {
  return a - b;
}
static int update_hv(int a, int b) {
  return a + b;
}
static void *update_tree(void *a, void *b) {
  (void) a;
  return b;
}
static int hv_f(int k, int v) {
  return k + v;
}
static int hv_fr(int k, int v, void *arg) {
  (void) arg;
  return k + v;
}

START_TEST(null_test1)
// hash table
CHECK(hash_hi_set_comp(NULL, comp_hkey) == -1);
CHECK(hash_hi_set_update(NULL, update_hv) == -1);
CHECK(hash_hi_size(NULL) == 0);
CHECK(hash_hi_capacity(NULL) == 0);
CHECK(hash_hi_apply(NULL, hv_f) == -1);
CHECK(hash_hi_apply_r(NULL, hv_fr, NULL) == -1);
hiter_hi_t hit;
int hk, hv;
CHECK(hash_hi_first(NULL, &hit, &hk, &hv) == 1);
CHECK(hash_hi_next(NULL, &hk, &hv) == 1);
htable_hi_t *h = hash_hi_init(0);
CHECK(hash_hi_set_comp(h, comp_hkey) == 0);
CHECK(hash_hi_set_update(h, update_hv) == 0);
CHECK(hash_hi_apply(h, NULL) == -1);
CHECK(hash_hi_first(h, NULL, &hk, &hv) == 1);
hash_hi_destroy(&h);

// thread safe hash table
CHECK(phash_pi_set_comp(NULL, comp_hkey) == -1);
CHECK(phash_pi_set_update(NULL, update_hv) == -1);
CHECK(phash_pi_size(NULL) == 0);
CHECK(phash_pi_capacity(NULL) == 0);
phtable_pi_t *p = phash_pi_init(0);
CHECK(phash_pi_set_comp(p, comp_hkey) == 0);
CHECK(phash_pi_set_update(p, update_hv) == 0);
phash_pi_destroy(&p);
END_TEST

START_TEST(null_test2)
// priority queue
CHECK(pqueue_int_set_value_free(NULL, free) == -1);
CHECK(pqueue_int_size(NULL) == 0);
CHECK(pqueue_int_is_empty(NULL));
CHECK(!pqueue_int_pop(NULL).found);
CHECK(!pqueue_int_peek(NULL).found);
pqueue_int_t *q = pqueue_int_init();
CHECK(pqueue_int_set_value_free(q, NULL) == 0);
pqueue_int_destroy(&q);

// array
CHECK(array_int_size(NULL) == 0);
CHECK(array_int_capacity(NULL) == 0);

// option
CHECK(option_int_set(NULL, 1) == -1);
CHECK(!option_int_get_clear(NULL, NULL));
option_int_t o = option_int_init_empty();
CHECK(option_int_set(&o, 4) == 0);
CHECK(option_int_is_set(o));
END_TEST

START_TEST(null_test3)
// tree
CHECK(tree_int_set_update(NULL, update_tree) == -1);
CHECK(tree_int_set_value_free(NULL, free) == -1);
CHECK(tree_int_print(NULL, NULL) == -1);
titer_int_t tit;
int tk;
void *tv;
CHECK(tree_int_next(NULL, &tk, &tv) == 1);
CHECK(tree_int_first(NULL, NULL, &tk, &tv) == 1);
CHECK(tree_int_first(NULL, &tit, &tk, &tv) == 1);
CHECK(tree_int_first_from(NULL, 1, NULL, &tk, &tv) == 1);
CHECK(tree_int_post_next(NULL, &tk, &tv) == 1);
CHECK(tree_int_post_first(NULL, NULL, &tk, &tv) == 1);
tree_int_t *t = tree_int_init();
CHECK(tree_int_set_update(t, update_tree) == 0);
CHECK(tree_int_set_value_free(t, NULL) == 0);
tree_int_destroy(&t);

// linked list
CHECK(llist_int_set_comp(NULL, comp_int_p) == -1);
CHECK(llist_int_size(NULL) == 0);
lnode_int_t *it = NULL;
int v;
CHECK(llist_int_first(NULL, &it, &v) == 1);
CHECK(llist_int_last(NULL, &it, &v) == 1);
llist_int_t *l = llist_int_init();
CHECK(llist_int_set_comp(l, comp_int_p) == 0);
CHECK(llist_int_first(l, NULL, &v) == 1);
CHECK(llist_int_last(l, NULL, &v) == 1);
CHECK(llist_int_next(NULL, &v) == 1);
CHECK(llist_int_prev(NULL, &v) == 1);
CHECK(llist_int_remove_forward(NULL, &it) == -1);
CHECK(llist_int_remove_forward(l, NULL) == -1);
CHECK(llist_int_remove_backwards(NULL, &it) == -1);
CHECK(llist_int_remove_backwards(l, NULL) == -1);
CHECK(llist_int_insert_before(NULL, &it, 1) == -1);
CHECK(llist_int_insert_before(l, NULL, 1) == -1);
CHECK(llist_int_insert_after(NULL, &it, 1) == -1);
CHECK(llist_int_insert_after(l, NULL, 1) == -1);
CHECK(llist_int_add_node(NULL, NULL) == -1);
CHECK(llist_int_add_node(l, NULL) == -1);
llist_int_destroy(&l);
END_TEST
