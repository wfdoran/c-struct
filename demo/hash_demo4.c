#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#define key_t int
#define value_t double
#define prefix int
#include <hash_table.h>
#undef prefix
#undef value_t
#undef key_t

double f(int key, double value) {
  return key + sqrt(value);
}

int main() {
  htable_int_t *h = hash_int_init(0);
  for (int i = 1; i <= 100; i++) {
    hash_int_put(h, i, sqrt(i));
  }
  hash_int_apply(h, f);
 
  hiter_int_t *iter;
  int key;
  double value;
  for (hash_int_first(h, &iter, &key, &value); iter != NULL; hash_int_next(&iter, &key, &value)) {
    printf("%8d %12.6f\n", key, value); 
  }

  hash_int_destroy(&h);
  return 0;
}


