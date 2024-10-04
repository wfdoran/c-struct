#include <check.h>

#define data_t int
#define prefix int
#include <tree.h>
#undef data_t
#undef prefix


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


