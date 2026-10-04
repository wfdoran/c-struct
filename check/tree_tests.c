#include <check.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define data_t int
#define prefix int
#include <tree.h>
#undef data_t
#undef prefix

typedef struct {
  int x;
  int y;
} pair_t;
    

#define data_t pair_t
#define prefix pair
#include <tree.h>
#undef prefix
#undef data_t

static int comp_pair(pair_t *a, pair_t *b) {
  if (a->x < b->x) {
    return -1;
  }
  if (a->x > b->x) {
    return 1;
  }
  if (a->y < b->y) {
    return -1;
  }
  if (a->y > b->y) {
    return 1;
  }
  return 0;
}

static void* add_one(void *current, void *new) {
  int *a = (int*) current;

  if (a == NULL) {
    int *rv = malloc(sizeof(int));
    *rv = 1;
    return (void*) rv;
  }

  int x = *a;
  *a = (x + 1);
  return a;
}



#define data_t int
#define prefix dn
#include <tree.h>
#undef prefix
#undef data_t

// A key type with no default comparison function, and no comparison function set.
typedef struct {
  int v;
} nocomp_t;

#define data_t nocomp_t
#define prefix nc
#include <tree.h>
#undef prefix
#undef data_t

// An instantiation whose allocations can be made to fail, to test the out of memory paths.
static long oom_budget = -1;   // allocations which may still succeed; -1 for no limit

static void *oom_malloc(size_t n) {
  if (oom_budget == 0) {
    return NULL;
  }
  if (oom_budget > 0) {
    oom_budget--;
  }
  return malloc(n);
}

#define malloc oom_malloc
#define data_t int
#define prefix oom
#include <tree.h>
#undef prefix
#undef data_t
#undef malloc

START_TEST(tree_test1)

tree_int_t *a = tree_int_init();
CHECK(a != NULL);

tree_int_destroy(&a);
CHECK(a == NULL);

END_TEST


START_TEST(tree_test2)

tree_int_t *a = tree_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 0; i < n; i++) {
  tree_int_insert(a, i, NULL);
}

CHECK(tree_int_size(a) == n);

tree_int_destroy(&a);
CHECK(a == NULL);

END_TEST


START_TEST(tree_test3)

tree_int_t *a = tree_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 0; i < n; i++) {
  tree_int_insert(a, i, NULL);
}

{
  key_int_value_t x = tree_int_retrieve(a, n+1);
  CHECK(!x.found);
}

{
  key_int_value_t x = tree_int_delete(a, n+1);
  CHECK(!x.found);
}

for (int i = 0; i < n; i++) {
  key_int_value_t x = tree_int_retrieve(a, i);
  CHECK(x.key == i);
  CHECK(x.found);
}

CHECK(tree_int_size(a) == n);

for (int i = 0; i < n; i++) {
  key_int_value_t x = tree_int_delete(a, i);
  CHECK(x.key == i);
  CHECK(x.found);
}

CHECK(tree_int_size(a) == 0);

tree_int_destroy(&a);
CHECK(a == NULL);

END_TEST


START_TEST(tree_test4)

seed_rand(445);

tree_int_t *a = tree_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 0; i < n; i++) {
  int key = get_rand() & 0xffff;
  tree_int_insert(a, key, NULL);
}

int prev = 0;
while (true) {
  key_int_value_t x = tree_int_delete_min(a);
  if (!x.found) {
    break;
  }

  int key = x.key;
  CHECK(key >= prev);
  prev = key;
}

CHECK(tree_int_size(a) == 0);

tree_int_destroy(&a);
CHECK(a == NULL);

END_TEST


START_TEST(tree_test5)

seed_rand(446);

tree_pair_t *a = tree_pair_init();
CHECK(a != NULL);

tree_pair_set_comp(a, &comp_pair);

int n = 10;
for (int i = 0; i < n; i++) {
  pair_t q = {
    .x = (get_rand() >> 4) & 0xf,
    .y = (get_rand() >> 8) & 0xffff,
  };
  tree_pair_insert(a, q, NULL);
}

pair_t prev = {.x = 0, .y = 0};
while (true) {
  key_pair_value_t v = tree_pair_delete_min(a);
  if (!v.found) {
    break;
  }

  CHECK(prev.x < v.key.x || (prev.x == v.key.x && prev.y <= v.key.y));
  prev = v.key;
}
  
tree_pair_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(tree_test6) 

tree_int_t *a = tree_int_init();
CHECK(a != NULL);

tree_int_set_update(a, &add_one);

int n = 10;
int m = 5;

for (int i = 0; i < m; i++) {
  for (int j = 0; j < n; j++) {
    tree_int_insert(a, j, NULL);
  }
}

for (int j = 0; j < n; j++) {
  key_int_value_t x = tree_int_retrieve(a, j);
  CHECK(x.found);
  CHECK(*((int*) x.value) == m);
}

tree_int_set_value_free(a, free);

tree_int_destroy(&a);
CHECK(a == NULL);


END_TEST


START_TEST(tree_test7) 

seed_rand(447);

tree_int_t *a = tree_int_init();
CHECK(a != NULL);

int n = 100;
int pivot_value = 0;
int num_less = 0;

for (int i = 0; i < n; i++) {
  int key = get_rand() & 0xffffff;
  tree_int_insert(a, key, NULL);
  if (i == 0) {
    pivot_value = key;
  } else if (key < pivot_value) {
    num_less++;
  }
}

CHECK(tree_int_num_less(a, pivot_value) == num_less);

tree_int_destroy(&a);
CHECK(a == NULL);


END_TEST


START_TEST(tree_test8)

tree_int_t *a = tree_int_init();
CHECK(a != NULL);

int n = 100;
int mult = 7;
int add = 23;

for (int i = 0; i < n; i++) {
  int key = ((mult * i) + add) % n;
  tree_int_insert(a, key, NULL);
}

CHECK(tree_int_size(a) == n);


{
  key_int_value_t kv = tree_int_delete(a, n + 3);
  CHECK(!kv.found);
}

for (int i = 0; i < n; i++) {
  key_int_value_t kv = tree_int_delete(a, i);
  CHECK(kv.found);
}

CHECK(tree_int_size(a) == 0);

tree_int_destroy(&a);
CHECK(a == NULL);


END_TEST


START_TEST(tree_test9)

seed_rand(448);

tree_int_t *a = tree_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 0; i < n; i++) {
  int key = get_rand() & 0xffff;
  tree_int_insert(a, key, NULL);
}

int prev = 0x10000;
while (true) {
  key_int_value_t x = tree_int_delete_max(a);
  if (!x.found) {
    break;
  }

  int key = x.key;
  CHECK(key <= prev);
  prev = key;
}

CHECK(tree_int_size(a) == 0);

tree_int_destroy(&a);
CHECK(a == NULL);

END_TEST


START_TEST(tree_test10)

seed_rand(449);

tree_int_t *a = tree_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 0; i < n; i++) {
  int key = get_rand() & 0xffff;
  tree_int_insert(a, key, NULL);
}

titer_int_t it;
int prev = -1;
int key;
for (int32_t rc = tree_int_first(a, &it, &key, NULL); rc == 0; rc = tree_int_next(&it, &key, NULL)) {
  CHECK(key > prev);
  prev = key;
}


tree_int_destroy(&a);
CHECK(a == NULL);

END_TEST


START_TEST(tree_test11)

seed_rand(450);
     
tree_int_t *a = tree_int_init();
CHECK(a != NULL);


int n = 100;
int *keys = malloc(n * sizeof(int));
CHECK(keys != NULL);

keys[0] = get_rand() % 0xffff;
for (int i = 1; i < n; i++) {
  keys[i] = keys[i - 1] + 1 + get_rand() % 0xffff;
}

int mult = 57;

for (int i = 0; i < n; i++) {
  int idx = (i * mult) % n;
  tree_int_insert(a, keys[idx], NULL);
}


for (int quant = 1; quant <= 3; quant++) {
  int rank = quant * n / 4;
  key_int_value_t kv = tree_int_get_rank(a, rank);
  CHECK(kv.found);
  CHECK(kv.key == keys[rank]);
}

tree_int_destroy(&a);
CHECK(a == NULL);
free(keys);
      
END_TEST

// destroy accepts a NULL pointer and an already destroyed container
START_TEST(tree_test12)

tree_dn_t *t = NULL;
tree_dn_destroy(NULL);
tree_dn_destroy(&t);
CHECK(t == NULL);
t = tree_dn_init();
tree_dn_destroy(&t);
tree_dn_destroy(&t);
CHECK(t == NULL);

END_TEST

// first / next, first_from, and the post-order walk, against a model
START_TEST(tree_test13)

tree_int_t *t = tree_int_init();
titer_int_t it;
int key;
void *value;

// an empty tree
CHECK(tree_int_first(t, &it, &key, &value) == 1);
CHECK(tree_int_next(&it, &key, &value) == 1);
CHECK(tree_int_first_from(t, 5, &it, &key, &value) == 1);
CHECK(tree_int_post_first(t, &it, &key, &value) == 1);
CHECK(tree_int_post_next(&it, &key, &value) == 1);

seed_rand(13);
int present[300] = {0};
for (int i = 0; i < 200; i++) {
  int k = get_rand() % 300;
  tree_int_insert(t, k, (void *) (long) (k * 2));
  present[k] = 1;
}
size_t count = tree_int_size(t);

// in order, with the values, with NULL for key and value, and the end is sticky
int expect = 0;
size_t seen = 0;
for (int32_t rc = tree_int_first(t, &it, &key, &value); rc == 0; rc = tree_int_next(&it, &key, &value)) {
  while (!present[expect]) {
    expect++;
  }
  CHECK(key == expect && (long) value == 2 * key);
  expect++;
  seen++;
}
CHECK(seen == count);
CHECK(tree_int_next(&it, &key, &value) == 1);
seen = 0;
for (int32_t rc = tree_int_first(t, &it, NULL, NULL); rc == 0; rc = tree_int_next(&it, NULL, NULL)) {
  seen++;
}
CHECK(seen == count);

// first_from starts at the first key which is >= the one asked for, present or not
for (int from = -2; from < 305; from++) {
  int want = from < 0 ? 0 : from;
  while (want < 300 && !present[want]) {
    want++;
  }
  int32_t rc = tree_int_first_from(t, from, &it, &key, &value);
  if (want >= 300) {
    CHECK(rc == 1);
  } else {
    CHECK(rc == 0 && key == want);
  }
}

// post order: every node after its children, each node once, the root last
int visited = 0;
int last_key = -1;
for (int32_t rc = tree_int_post_first(t, &it, &key, NULL); rc == 0; rc = tree_int_post_next(&it, &key, NULL)) {
  visited++;
  last_key = key;
}
CHECK((size_t) visited == count);
CHECK(last_key == t->root->key);
CHECK(tree_int_post_next(&it, &key, NULL) == 1);

// stopping early needs no clean up
CHECK(tree_int_first(t, &it, &key, NULL) == 0);
CHECK(tree_int_next(&it, &key, NULL) == 0);

tree_int_destroy(&t);

END_TEST

// ---------------------------------------------------------------------------
// A checker for the structure of a tree: the keys are in order, the parent
// pointers are right, size and height of every node are right, and the heights
// of the two children of every node differ by at most one (the AVL property).
// Returns the number of nodes below n, or -1 if something is wrong.

static long tree_check_node(tnode_int_t *n, tnode_int_t *parent, long lo, long hi, int *height) {
  if (n == NULL) {
    *height = 0;
    return 0;
  }
  if (n->parent != parent || n->key <= lo || n->key >= hi) {
    return -1;
  }
  int lh, rh;
  long ls = tree_check_node(n->left, n, lo, n->key, &lh);
  long rs = tree_check_node(n->right, n, n->key, hi, &rh);
  if (ls < 0 || rs < 0) {
    return -1;
  }
  *height = 1 + (lh > rh ? lh : rh);
  if (n->height != *height || n->size != (size_t) (1 + ls + rs) || lh - rh > 1 || rh - lh > 1) {
    return -1;
  }
  return 1 + ls + rs;
}

// 1 if the tree is well formed and has the given number of nodes
static int tree_ok(tree_int_t *t, size_t expected_size) {
  int h;
  long n = tree_check_node(t->root, NULL, INT64_MIN, INT64_MAX, &h);
  return n == (long) expected_size && tree_int_size(t) == expected_size && tree_int_height(t) == h;
}

static int tree_free_count;

static void count_free(void *v) {
  tree_free_count++;
  free(v);
}

static void *keep_first(void *current, void *incoming) {
  return current != NULL ? current : incoming;
}

static void *sum_values(void *current, void *incoming) {
  return (void *) ((long) current + (long) incoming);
}

// the checker itself must notice damage, or the tests which use it prove nothing
START_TEST(tree_test14)

tree_int_t *t = tree_int_init();
CHECK(tree_ok(t, 0));
for (int i = 1; i <= 7; i++) {
  tree_int_insert(t, i, NULL);
}
CHECK(tree_ok(t, 7));
CHECK(!tree_ok(t, 6));

t->root->size++;
CHECK(!tree_ok(t, 7));
t->root->size--;
t->root->left->parent = NULL;
CHECK(!tree_ok(t, 7));
t->root->left->parent = t->root;
t->root->left->key = 100;
CHECK(!tree_ok(t, 7));
t->root->left->key = 2;
t->root->height++;
CHECK(!tree_ok(t, 7));
t->root->height--;
CHECK(tree_ok(t, 7));

tree_int_destroy(&t);

END_TEST

// the four rotations, and a long run of ordered inserts which keeps the tree balanced
START_TEST(tree_test15)

int orders[4][3] = {{3, 2, 1}, {1, 2, 3}, {3, 1, 2}, {1, 3, 2}};   // LL, RR, LR, RL
for (int i = 0; i < 4; i++) {
  tree_int_t *t = tree_int_init();
  for (int j = 0; j < 3; j++) {
    CHECK(tree_int_insert(t, orders[i][j], NULL) == 0);
  }
  CHECK(tree_ok(t, 3));
  CHECK(tree_int_height(t) == 2);
  CHECK(t->root->key == 2);
  CHECK(t->root->left->key == 1 && t->root->right->key == 3);
  tree_int_destroy(&t);
}

// ascending, descending and zig-zag inserts of many keys give a perfectly balanced tree
for (int pattern = 0; pattern < 3; pattern++) {
  tree_int_t *t = tree_int_init();
  int n = 1023;
  for (int i = 0; i < n; i++) {
    int key = pattern == 0 ? i : pattern == 1 ? n - i : (i % 2 ? n - i : i);
    tree_int_insert(t, key, NULL);
    if (i % 100 == 0) {
      CHECK(tree_ok(t, tree_int_size(t)));
    }
  }
  CHECK(tree_ok(t, tree_int_size(t)));
  CHECK(tree_int_size(t) == (size_t) n || pattern == 2);
  if (pattern < 2) {
    CHECK(tree_int_height(t) == 10);
  }
  CHECK(tree_int_height(t) <= 11);
  tree_int_destroy(&t);
}

END_TEST

// deleting every kind of node: leaf, one child, two children
START_TEST(tree_test16)

//            50
//        30      70
//      20  40  60  80          and 65 below 60 on the right
tree_int_t *t = tree_int_init();
int keys[] = {50, 30, 70, 20, 40, 60, 80, 65};
for (int i = 0; i < 8; i++) {
  tree_int_insert(t, keys[i], (void *) (long) (keys[i] * 10));
}
CHECK(tree_ok(t, 8));

// a key which is not there, and an empty tree
key_int_value_t r = tree_int_delete(t, 55);
CHECK(!r.found);
CHECK(tree_ok(t, 8));
tree_int_t *e = tree_int_init();
CHECK(!tree_int_delete(e, 1).found);
CHECK(tree_ok(e, 0));
tree_int_destroy(&e);

// a leaf
r = tree_int_delete(t, 20);
CHECK(r.found && r.key == 20 && (long) r.value == 200);
CHECK(tree_ok(t, 7));
CHECK(!tree_int_retrieve(t, 20).found);

// a node with two children whose successor (the smallest key of the right subtree) is deep and
// has a right child of its own: 50 is replaced by 60, and 65 moves up
r = tree_int_delete(t, 50);
CHECK(r.found && r.key == 50 && (long) r.value == 500);
CHECK(tree_ok(t, 6));
CHECK(t->root->key == 60);
CHECK(tree_int_retrieve(t, 65).found);
CHECK(tree_int_get_rank(t, 0).key == 30);

// a node with two children whose successor is its right child
tree_int_t *u = tree_int_init();
tree_int_insert(u, 2, NULL);
tree_int_insert(u, 1, NULL);
tree_int_insert(u, 3, NULL);
CHECK(tree_int_delete(u, 2).found);
CHECK(tree_ok(u, 2));
CHECK(u->root->key == 3 && u->root->left->key == 1);
// a node with only a left child, and only a right child
CHECK(tree_int_delete(u, 3).found);
CHECK(tree_ok(u, 1));
CHECK(u->root->key == 1);
tree_int_insert(u, 5, NULL);
CHECK(tree_int_delete(u, 1).found);
CHECK(tree_ok(u, 1) && u->root->key == 5);
CHECK(tree_int_delete(u, 5).found);
CHECK(tree_ok(u, 0) && u->root == NULL);
tree_int_destroy(&u);

// delete everything in an order which forces rebalancing on the way
int rest[] = {40, 80, 30, 65, 70, 60};
size_t left = 6;
for (int i = 0; i < 6; i++) {
  CHECK(tree_int_delete(t, rest[i]).found);
  left--;
  CHECK(tree_ok(t, left));
}
CHECK(t->root == NULL);

tree_int_destroy(&t);

END_TEST

// a long random mix of operations, against a model, with the structure checked as it goes
START_TEST(tree_test17)

#define MODEL_KEYS 500
tree_int_t *t = tree_int_init();
int present[MODEL_KEYS] = {0};
long value[MODEL_KEYS] = {0};
size_t count = 0;
int two_child_deletes = 0;

seed_rand(17);
for (int step = 0; step < 40000; step++) {
  int op = get_rand() % 10;
  int k = get_rand() % MODEL_KEYS;
  if (op < 5) {
    long v = get_rand() & 0xffff;
    CHECK(tree_int_insert(t, k, (void *) v) == 0);
    if (!present[k]) {
      count++;
    }
    present[k] = 1;
    value[k] = v;
  } else if (op < 8) {
    tnode_int_t *n = t->root;
    while (n != NULL && n->key != k) {
      n = k < n->key ? n->left : n->right;
    }
    if (n != NULL && n->left != NULL && n->right != NULL) {
      two_child_deletes++;
    }
    key_int_value_t r = tree_int_delete(t, k);
    CHECK(r.found == (present[k] != 0));
    if (present[k]) {
      CHECK(r.key == k && (long) r.value == value[k]);
      present[k] = 0;
      count--;
    }
  } else if (op == 8) {
    key_int_value_t r = tree_int_retrieve(t, k);
    CHECK(r.found == (present[k] != 0));
    if (present[k]) {
      CHECK((long) r.value == value[k]);
    }
  } else {
    // counts below, at most, above and at least a key, and the entry of a given rank
    size_t lt = 0, le = 0, gt = 0, ge = 0;
    for (int i = 0; i < MODEL_KEYS; i++) {
      if (present[i]) {
        lt += i < k;
        le += i <= k;
        gt += i > k;
        ge += i >= k;
      }
    }
    CHECK(tree_int_num_less(t, k) == lt && tree_int_num_less_equal(t, k) == le);
    CHECK(tree_int_num_greater(t, k) == gt && tree_int_num_greater_equal(t, k) == ge);
    size_t rank = get_rand() % (count + 1);
    key_int_value_t r = tree_int_get_rank(t, rank);
    size_t seen = 0;
    int want = -1;
    for (int i = 0; i < MODEL_KEYS && want < 0; i++) {
      if (present[i] && seen++ == rank) {
        want = i;
      }
    }
    CHECK(r.found == (want >= 0));
    if (want >= 0) {
      CHECK(r.key == want);
    }
  }
  if (step % 50 == 0) {
    CHECK(tree_ok(t, count));
  }
}
CHECK(tree_ok(t, count));
CHECK(two_child_deletes > 500);   // the splice was exercised many times

tree_int_destroy(&t);
#undef MODEL_KEYS

END_TEST

// the smallest, the largest and ranks, on an empty tree and while emptying one
START_TEST(tree_test18)

tree_int_t *t = tree_int_init();
CHECK(!tree_int_retrieve_min(t).found);
CHECK(!tree_int_retrieve_max(t).found);
CHECK(!tree_int_delete_min(t).found);
CHECK(!tree_int_delete_max(t).found);
CHECK(!tree_int_get_rank(t, 0).found);
CHECK(tree_int_size(t) == 0 && tree_int_height(t) == 0);
CHECK(tree_int_num_less(t, 5) == 0 && tree_int_num_greater_equal(t, 5) == 0);

for (int i = 1; i <= 100; i++) {
  tree_int_insert(t, i * 3, (void *) (long) i);
}
key_int_value_t lo = tree_int_retrieve_min(t);
key_int_value_t hi = tree_int_retrieve_max(t);
CHECK(lo.found && lo.key == 3 && (long) lo.value == 1);
CHECK(hi.found && hi.key == 300 && (long) hi.value == 100);
CHECK(tree_int_size(t) == 100);   // retrieving does not remove

CHECK(!tree_int_get_rank(t, 100).found);
CHECK(tree_int_get_rank(t, 99).key == 300);

// remove from both ends alternately, the structure staying valid
size_t n = 100;
int low = 3, high = 300;
while (n > 0) {
  key_int_value_t a = tree_int_delete_min(t);
  CHECK(a.found && a.key == low);
  low += 3;
  n--;
  CHECK(tree_ok(t, n));
  if (n > 0) {
    key_int_value_t b = tree_int_delete_max(t);
    CHECK(b.found && b.key == high);
    high -= 3;
    n--;
    CHECK(tree_ok(t, n));
  }
}
CHECK(t->root == NULL);
CHECK(!tree_int_delete_min(t).found);

tree_int_destroy(&t);

END_TEST

// update and value_free: what happens to the values when a key is inserted again, when a key is
// deleted, and when the tree is destroyed
START_TEST(tree_test19)

// no update and no value_free: the new value replaces the old one, and nothing is freed
tree_int_t *t = tree_int_init();
tree_free_count = 0;
tree_int_insert(t, 1, (void *) 10);
tree_int_insert(t, 1, (void *) 20);
CHECK(tree_int_size(t) == 1);
CHECK((long) tree_int_retrieve(t, 1).value == 20);
tree_int_destroy(&t);

// value_free: freed when replaced, when destroyed, and not when deleted (the caller gets the value)
t = tree_int_init();
tree_int_set_value_free(t, count_free);
for (int i = 0; i < 5; i++) {
  int *v = malloc(sizeof(int));
  *v = i;
  tree_int_insert(t, i, v);
}
int *replacement = malloc(sizeof(int));
*replacement = 99;
tree_int_insert(t, 2, replacement);     // the old value of key 2 is freed
CHECK(tree_free_count == 1);
CHECK(tree_int_size(t) == 5);
CHECK(*(int *) tree_int_retrieve(t, 2).value == 99);

key_int_value_t r = tree_int_delete(t, 3);
CHECK(r.found && tree_free_count == 1);
free(r.value);
r = tree_int_delete_min(t);
CHECK(r.found && tree_free_count == 1);
free(r.value);
r = tree_int_delete_max(t);
CHECK(r.found && tree_free_count == 1);
free(r.value);

tree_int_destroy(&t);                    // the 2 values which are left
CHECK(tree_free_count == 3);
CHECK(t == NULL);

// update: the new value is update(current, incoming), and a new node gets update(NULL, incoming)
t = tree_int_init();
tree_int_set_update(t, sum_values);
tree_int_insert(t, 7, (void *) 5);      // new: update(NULL, 5) is 5
CHECK((long) tree_int_retrieve(t, 7).value == 5);
tree_int_insert(t, 7, (void *) 6);
tree_int_insert(t, 7, (void *) 7);
CHECK((long) tree_int_retrieve(t, 7).value == 18);
CHECK(tree_int_size(t) == 1);
tree_int_destroy(&t);

// with an update function, value_free is not applied to the replaced value
t = tree_int_init();
tree_int_set_update(t, keep_first);
tree_int_set_value_free(t, count_free);
tree_free_count = 0;
int *first = malloc(sizeof(int));
int *second = malloc(sizeof(int));
tree_int_insert(t, 1, first);
tree_int_insert(t, 1, second);
CHECK(tree_free_count == 0);
CHECK(tree_int_retrieve(t, 1).value == first);
free(second);
tree_int_destroy(&t);
CHECK(tree_free_count == 1);

// the demo's counter: add_one is update(NULL) -> new counter, update(c) -> c + 1
t = tree_int_init();
tree_int_set_update(t, add_one);
tree_int_set_value_free(t, count_free);
tree_free_count = 0;
for (int i = 0; i < 30; i++) {
  tree_int_insert(t, i % 3, NULL);
}
CHECK(tree_int_size(t) == 3);
CHECK(*(int *) tree_int_retrieve(t, 0).value == 10);
tree_int_destroy(&t);
CHECK(tree_free_count == 3);

END_TEST

// no comparison function: insert refuses, the other calls cannot find anything, none crash
START_TEST(tree_test20)

tree_nc_t *t = tree_nc_init();
CHECK(t != NULL);
nocomp_t k = {1};
CHECK(tree_nc_insert(t, k, NULL) == -1);
CHECK(tree_nc_size(t) == 0);
CHECK(!tree_nc_delete(t, k).found);
CHECK(!tree_nc_retrieve(t, k).found);
CHECK(!tree_nc_delete_min(t).found);
CHECK(!tree_nc_retrieve_min(t).found);
CHECK(tree_nc_num_less(t, k) == 0);
CHECK(tree_nc_insert(NULL, k, NULL) == -1);
tree_nc_destroy(&t);

// a tree with entries whose comparison function is removed: delete finds nothing, and the
// entries are still freed by destroy
tree_int_t *w = tree_int_init();
for (int i = 0; i < 5; i++) {
  tree_int_insert(w, i, NULL);
}
tree_int_set_comp(w, NULL);
CHECK(!tree_int_delete(w, 2).found);
CHECK(tree_int_insert(w, 9, NULL) == -1);
CHECK(tree_int_size(w) == 5);
CHECK(tree_int_delete_min(w).found);   // these do not compare keys
tree_int_destroy(&w);
CHECK(t == NULL);

// a comparison function set later makes the tree work
tree_pair_t *p = tree_pair_init();
pair_t a = {1, 2};
CHECK(tree_pair_insert(p, a, NULL) == -1);
tree_pair_set_comp(p, comp_pair);
CHECK(tree_pair_insert(p, a, NULL) == 0);
CHECK(tree_pair_retrieve(p, a).found);
tree_pair_destroy(&p);

END_TEST

// out of memory: init fails cleanly, and a failed insert leaves the tree as it was
START_TEST(tree_test21)

oom_budget = 0;
CHECK(tree_oom_init() == NULL);
oom_budget = -1;

tree_oom_t *t = tree_oom_init();
CHECK(t != NULL);
for (int i = 0; i < 40; i++) {
  CHECK(tree_oom_insert(t, i * 2, NULL) == 0);
}
size_t before = tree_oom_size(t);

// no node can be allocated: a new key fails, and an existing key is replaced with no allocation
// (keys above, below and between the existing ones, so that the failure is passed up through
// both the left and the right branches)
int new_keys[] = {1001, -5, 1, 33, 61, 79};
oom_budget = 0;
for (int i = 0; i < 6; i++) {
  CHECK(tree_oom_insert(t, new_keys[i], NULL) == -1);
}
CHECK(tree_oom_insert(t, 4, (void *) 5) == 0);
oom_budget = -1;
CHECK(tree_oom_size(t) == before);
for (int i = 0; i < 6; i++) {
  CHECK(tree_oom_retrieve(t, new_keys[i]).found == false);
}
CHECK((long) tree_oom_retrieve(t, 4).value == 5);
for (int i = 0; i < 6; i++) {
  CHECK(tree_oom_insert(t, new_keys[i], NULL) == 0);
}
CHECK(tree_oom_size(t) == before + 6);

// the tree is still balanced and complete after the failures
CHECK(tree_oom_height(t) <= 8);
size_t seen = 0;
titer_oom_t it;
int key, prev = -1000;
for (int32_t rc = tree_oom_first(t, &it, &key, NULL); rc == 0; rc = tree_oom_next(&it, &key, NULL)) {
  CHECK(key > prev);
  prev = key;
  seen++;
}
CHECK(seen == tree_oom_size(t));

tree_oom_destroy(&t);

END_TEST

static int tree_print_next_key;
static int tree_print_calls;

static void print_node_check(int key, void *value) {
  if (key == tree_print_next_key && (long) value == key + 100) {
    tree_print_calls++;
  }
  tree_print_next_key = key + 1;
}

// print: one line per node, in order, written to standard output
START_TEST(tree_test22)

#define PRINT_FILE "tree_print_test.txt"
tree_int_t *t = tree_int_init();
for (int i = 0; i < 7; i++) {
  tree_int_insert(t, i, NULL);
}

fflush(stdout);
int saved = dup(STDOUT_FILENO);
int fd = open(PRINT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
CHECK(saved >= 0 && fd >= 0);
dup2(fd, STDOUT_FILENO);
close(fd);
tree_int_print(t, NULL);
tree_int_t *empty = tree_int_init();
tree_int_print(empty, NULL);
fflush(stdout);
dup2(saved, STDOUT_FILENO);
close(saved);

FILE *f = fopen(PRINT_FILE, "r");
CHECK(f != NULL);
char line[256];
int lines = 0;
while (fgets(line, sizeof(line), f) != NULL) {
  CHECK(strstr(line, "-- *") != NULL);
  lines++;
}
fclose(f);
remove(PRINT_FILE);
CHECK(lines == 7);   // the empty tree printed nothing

// the print callback is called once per node with its key and value, in order
tree_print_next_key = 0;
tree_print_calls = 0;
fflush(stdout);
saved = dup(STDOUT_FILENO);
fd = open(PRINT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
dup2(fd, STDOUT_FILENO);
close(fd);
for (int i = 0; i < 7; i++) {
  tree_int_insert(t, i, (void *) (long) (i + 100));
}
tree_int_print(t, print_node_check);
fflush(stdout);
dup2(saved, STDOUT_FILENO);
close(saved);
remove(PRINT_FILE);
CHECK(tree_print_calls == 7);

// the post-order walk gives the values too
titer_int_t it;
int key;
void *value;
int visited = 0;
for (int32_t rc = tree_int_post_first(t, &it, &key, &value); rc == 0; rc = tree_int_post_next(&it, &key, &value)) {
  CHECK((long) value == key + 100);
  visited++;
}
CHECK(visited == 7);

tree_int_destroy(&t);
tree_int_destroy(&empty);
#undef PRINT_FILE

END_TEST
