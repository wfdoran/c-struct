#include <stdio.h>

#define data_t int
#define prefix int
#include <array.h>
#undef prefix
#undef data_t

int main(void) {
  array_int_t *a = array_int_init();

  array_int_append(a, 1);
  array_int_append(a, 2);
  array_int_append(a, 4);
  array_int_append(a, 4);
  array_int_append(a, 4);
  array_int_append(a, 7);
  array_int_append(a, 9);

  printf("%ld\n", array_int_bisect_upper(a, -100));
  printf("%ld\n", array_int_bisect_upper(a, 4));
  printf("%ld\n", array_int_bisect_upper(a, 5));
  printf("%ld\n", array_int_bisect_upper(a, 7));
  printf("%ld\n", array_int_bisect_upper(a, 100));

  printf("%ld\n", array_int_bisect_lower(a, -100));
  printf("%ld\n", array_int_bisect_lower(a, 4));
  printf("%ld\n", array_int_bisect_lower(a, 5));
  printf("%ld\n", array_int_bisect_lower(a, 7));
  printf("%ld\n", array_int_bisect_lower(a, 100));

  return 0;
}
