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
