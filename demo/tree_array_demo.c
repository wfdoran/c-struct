#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define data_t int32_t
#define prefix int32
#include <array.h>
#undef prefix
#undef data_t

#define data_t int32_t
#define prefix intarr
#include <tree.h>
#undef prefix
#undef data_t

void *update(void *cur, void *value) {
  array_int32_t *arr = cur == NULL ? array_int32_init() : cur;
  int32_t v = *(int32_t *) value;
  array_int32_append(arr, v);
  return arr;
}

void value_free(void *a) {
  array_int32_t *b = a;
  array_int32_destroy(&b);
}

int main(void) {
  srand(time(NULL));
  tree_intarr_t *t = tree_intarr_init();
  tree_intarr_set_update(t, update);
  tree_intarr_set_value_free(t, value_free);

  for (int i = 0; i < 100; i++) {
    int32_t key = rand() % 200;
    int32_t value = rand();

    tree_intarr_insert(t, key, &value);
  }

  titer_intarr_t it;
  int32_t key;
  void *value;
  for (int32_t rc = tree_intarr_first(t, &it, &key, &value); rc == 0;
       rc = tree_intarr_next(&it, &key, &value)) {
    printf("%3d :", key);
    array_int32_t *arr = value;
    for (int i = 0; i < array_int32_size(arr); i++) {
      printf(" %d", array_int32_get(arr, i));
    }
    printf("\n");
  }

  tree_intarr_destroy(&t);
  return 0;
}
