#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define key_t int
#define value_t int64_t
#define prefix int
#include <hash_table.h>
#undef prefix
#undef value_t
#undef key_t

int64_t sqr(int key, int64_t value) {
  return value * value;
}

int64_t sum(int key, int64_t value, void *arg) {
  int64_t *a = (int64_t*) arg;
  *a += value;
  return value;
}

int main() {
  htable_int_t *h = hash_int_init(0);
  for (int i = 1; i <= 1000; i++) {
    hash_int_put(h, i, (int64_t) i);
  }
  hash_int_apply(h, sqr);
  int64_t sumsqr = 0;
  hash_int_apply_r(h, sum, &sumsqr);
  printf("%ld\n", sumsqr);

  hash_int_destroy(&h);
  return 0;
}

