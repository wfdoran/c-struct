#include <check.h>
#include <stdlib.h>


#define data_t int
#define prefix int
#include <array.h>
#undef data_t
#undef prefix

START_TEST(array_test1)

array_int_t *a = array_int_init();
CHECK(a != NULL);
array_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(array_test2)

array_int_t *a = array_int_init();
CHECK(a != NULL);
array_int_append(a, 5);
array_int_append(a, 6);
CHECK(array_int_size(a) == 2);
CHECK(array_int_pop(a) == 6);
CHECK(array_int_pop(a) == 5);
CHECK(array_int_size(a) == 0);
array_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(array_test3)

int default_value = -1;
size_t size = 8;
array_int_t *a = array_int_init2(size, default_value);
CHECK(a != NULL);
CHECK(array_int_size(a) == size);
for (int i = 0; i < size; i++) {
  CHECK(array_int_get(a, i) == default_value);
}
array_int_destroy(&a);
CHECK(a == NULL);

END_TEST

int main(void) {
  array_test1();
  array_test2();
  array_test3();
  return 0;
}

