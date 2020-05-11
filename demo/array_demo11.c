#include <stdio.h>
#include <stdint.h>

#define data_t int64_t
#define prefix int64
#include <array.h>
#undef prefix
#undef data_t

int64_t add(int64_t a, int64_t b) {
  return a + b;
}

int64_t sqr(int64_t a) {
  return a * a;
}

int main(void) {
  int n = 1000;
  array_int64_t *a = array_int64_init2(n, INT64_C(1));
  array_int64_scan(a, add);
  array_int64_map(a, sqr);
  printf("%ld\n", array_int64_fold(a, add));

  array_int64_destroy(&a);
  return 0;
}
