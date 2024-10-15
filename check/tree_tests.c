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

