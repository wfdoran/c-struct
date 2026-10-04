#include <check.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

// The compile time hooks data_less, hkey_hash and hkey_equal.  The key is a
// struct ordered by k only, so the order cannot come from a default
// comparison, and tag shows that the hook looks at k alone.
typedef struct {
  int32_t k;
  int32_t tag;
} rec_t;

#define data_t rec_t
#define prefix hrec
#define data_less(a, b) ((a).k < (b).k)
#include <array.h>
#undef prefix
#undef data_t

#define data_t rec_t
#define prefix hrec
#define data_less(a, b) ((a).k < (b).k)
#include <tree.h>
#undef prefix
#undef data_t

#define data_t rec_t
#define prefix hrec
#define data_less(a, b) ((a).k > (b).k)
#include <pqueue.h>
#undef prefix
#undef data_t

#define data_t rec_t
#define prefix hrec
#define data_less(a, b) ((a).k < (b).k)
#define null_value ((rec_t){.k = -1, .tag = -1})
#include <linked_list.h>
#undef prefix
#undef data_t

#define hkey_t rec_t
#define value_t int
#define prefix hrec
#define hkey_hash(r) ((uint64_t) (uint32_t) (r).k * UINT64_C(0x9E3779B97F4A7C15))
#define hkey_equal(a, b) ((a).k == (b).k)
#include <hash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

#define hkey_t rec_t
#define value_t int
#define prefix hrec
#define hkey_hash(r) ((uint64_t) (uint32_t) (r).k * UINT64_C(0x9E3779B97F4A7C15))
#include <phash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

// no pointer to a comparison is set anywhere below: the hooks are enough

START_TEST(hook_test1)

// array: sort, bisect, heap
seed_rand(1);
array_hrec_t *a = array_hrec_init();
CHECK(a != NULL);
for (int i = 0; i < 1000; i++) {
  rec_t r = {.k = (int32_t) (get_rand() % 300), .tag = i};
  CHECK(array_hrec_append(a, r) == 0);
}
CHECK(array_hrec_sort(a) == 0);
for (size_t i = 1; i < array_hrec_size(a); i++) {
  CHECK(array_hrec_get(a, i - 1).k <= array_hrec_get(a, i).k);
  if (array_hrec_get(a, i - 1).k == array_hrec_get(a, i).k) {
    CHECK(array_hrec_get(a, i - 1).tag < array_hrec_get(a, i).tag); // stable
  }
}
for (int32_t k = -1; k <= 300; k++) {
  rec_t key = {.k = k, .tag = -1};
  ssize_t lo = array_hrec_bisect_lower(a, key);
  ssize_t up = array_hrec_bisect_upper(a, key);
  ssize_t any = array_hrec_bisect(a, key);
  ssize_t n = 0;
  for (size_t i = 0; i < array_hrec_size(a); i++) {
    n += array_hrec_get(a, i).k == k;
  }
  CHECK(up - lo + 1 == n);
  CHECK((any >= 0) == (n > 0));
}

for (size_t i = 0; i < array_hrec_size(a); i++) {
  rec_t r = array_hrec_get(a, i);
  r.k = (int32_t) (get_rand() % 500);
  array_hrec_set(a, r, i);
}
CHECK(array_hrec_heapify(a) == 0);
int32_t prev = INT32_MAX;
size_t count = 0;
while (array_hrec_size(a) > 0) {
  int32_t k = array_hrec_heappop(a).k;
  CHECK(k <= prev); // a max heap, like the default
  prev = k;
  count++;
}
CHECK(count == 1000);
array_hrec_destroy(&a);

END_TEST

START_TEST(hook_test2)

// tree, pqueue and linked list
seed_rand(2);
tree_hrec_t *t = tree_hrec_init();
pqueue_hrec_t *q = pqueue_hrec_init();
llist_hrec_t *l = llist_hrec_init();
CHECK(t != NULL && q != NULL && l != NULL);
int present[200] = {0};
for (int i = 0; i < 2000; i++) {
  rec_t r = {.k = (int32_t) (get_rand() % 200), .tag = i};
  CHECK(tree_hrec_insert(t, r, NULL) == 0);
  present[r.k] = 1;
  CHECK(pqueue_hrec_push(q, r, NULL) == 0);
  CHECK(llist_hrec_add_end(l, r) == 0);
}
int distinct = 0;
for (int k = 0; k < 200; k++) {
  distinct += present[k];
  rec_t key = {.k = k, .tag = -1};
  CHECK(tree_hrec_retrieve(t, key).found == (present[k] != 0));
}
CHECK(tree_hrec_size(t) == (size_t) distinct);

titer_hrec_t it;
rec_t key;
int32_t prev = -1;
for (int32_t rc = tree_hrec_first(t, &it, &key, NULL); rc == 0;
     rc = tree_hrec_next(&it, &key, NULL)) {
  CHECK(key.k > prev);
  prev = key.k;
}
CHECK(tree_hrec_delete(t, (rec_t){.k = 7, .tag = 0}).found == (present[7] != 0));
CHECK(!tree_hrec_retrieve(t, (rec_t){.k = 7, .tag = 0}).found);

// this pqueue was instantiated with the reverse ordering: smallest first
prev = -1;
while (!pqueue_hrec_is_empty(q)) {
  int32_t k = pqueue_hrec_pop(q).key.k;
  CHECK(k >= prev);
  prev = k;
}

CHECK(llist_hrec_msort(l) == 0);
prev = -1;
size_t n = 0;
for (lnode_hrec_t *x = l->head; x != NULL; x = x->next) {
  CHECK(x->data.k >= prev);
  prev = x->data.k;
  n++;
}
CHECK(n == 2000);

tree_hrec_destroy(&t);
pqueue_hrec_destroy(&q);
llist_hrec_destroy(&l);

END_TEST

START_TEST(hook_test3)

// hash tables: the hash and equality look at k only
htable_hrec_t *h = hash_hrec_init(0);
phtable_hrec_t *p = phash_hrec_init(0);
CHECK(h != NULL && p != NULL);
for (int i = 0; i < 5000; i++) {
  rec_t r = {.k = i, .tag = 100 + i};
  CHECK(hash_hrec_put(h, r, i) == 0);
  CHECK(phash_hrec_put(p, r, i) == 0);
}
CHECK(hash_hrec_size(h) == 5000);
for (int i = 0; i < 6000; i++) {
  rec_t r = {.k = i, .tag = -i}; // another tag: still the same key
  int v = -1;
  int w = -1;
  CHECK(hash_hrec_get(h, r, &v) == (i < 5000));
  CHECK(phash_hrec_get(p, r, &w) == (i < 5000));
  if (i < 5000) {
    CHECK(v == i && w == i);
  }
}
for (int i = 0; i < 5000; i += 2) {
  rec_t r = {.k = i, .tag = 0};
  CHECK(hash_hrec_remove(h, r, NULL) == 1);
  CHECK(phash_hrec_remove(p, r, NULL) == 1);
}
CHECK(hash_hrec_size(h) == 2500);
CHECK(phash_hrec_size(p) == 2500);
htable_hrec_t *c = hash_hrec_clone(h);
CHECK(c != NULL && hash_hrec_size(c) == 2500);
rec_t r1 = {.k = 3, .tag = 0};
CHECK(hash_hrec_get(c, r1, NULL) == 1);
hash_hrec_destroy(&c);
hash_hrec_destroy(&h);
phash_hrec_destroy(&p);

END_TEST
