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

int main(void) {
  array_test1();
  return 0;
}

