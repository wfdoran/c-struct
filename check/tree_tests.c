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

void *state;
int prev = -1;
tree_int_walk_init(a, &state);
while (state != NULL) {
  key_int_value_t kv = tree_int_walk_next(&state);
  CHECK(kv.found);
  CHECK(kv.key > prev);
  prev = kv.key;
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
