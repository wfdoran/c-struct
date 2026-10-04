#include <check.h>

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
