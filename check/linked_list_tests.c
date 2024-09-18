#include <check.h>

#define data_t int
#define prefix int
#define null_value -1
#include <linked_list.h>
#undef null_value
#undef prefix
#undef data_t

START_TEST(linked_list_test1)

llist_int_t *a = llist_int_init();
CHECK(a != NULL);
llist_int_destroy(&a);
CHECK(a == NULL);

END_TEST
