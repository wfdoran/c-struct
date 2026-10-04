#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define hkey_t int
#define value_t int
#define prefix int
#include <hash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

int main(void) {
  htable_int_t *h = hash_int_init(14);

  hash_int_put(h, 1, 1);
  hash_int_put(h, 2, 4);
  hash_int_put(h, 3, 9);

  htable_int_t *q = hash_int_clone(h);

  hash_int_put(h, 4, 16);
  hash_int_put(q, 4, 11);

  hiter_int_t iter;
  int value;
  for (int32_t rc = hash_int_first(h, &iter, NULL, &value); rc == 0;
       rc = hash_int_next(&iter, NULL, &value)) {
    printf("%d ", value);
  }
  printf("\n");

  for (int32_t rc = hash_int_first(q, &iter, NULL, &value); rc == 0;
       rc = hash_int_next(&iter, NULL, &value)) {
    printf("%d ", value);
  }
  printf("\n");

  hash_int_destroy(&h);
  hash_int_destroy(&q);
  return 0;
}
