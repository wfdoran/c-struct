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
#define value_t array_int32_t *
#include <tree.h>
#undef value_t
#undef prefix
#undef data_t

// each insert passes an array with one element; it is moved onto the array
// which is already in the tree
array_int32_t *update(array_int32_t *cur, array_int32_t *value) {
  for (size_t i = 0; i < array_int32_size(value); i++) {
    array_int32_append(cur, array_int32_get(value, i));
  }
  array_int32_destroy(&value);
  return cur;
}

void value_free(array_int32_t *a) {
  array_int32_destroy(&a);
}

int main(void) {
  srand(time(NULL));
  tree_intarr_t *t = tree_intarr_init();
  tree_intarr_set_update(t, update);
  tree_intarr_set_value_free(t, value_free);

  for (int i = 0; i < 100; i++) {
    int32_t key = rand() % 200;
    int32_t value = rand();

    array_int32_t *single = array_int32_init();
    array_int32_append(single, value);
    tree_intarr_insert(t, key, single);
  }

  titer_intarr_t it;
  int32_t key;
  array_int32_t *value = NULL;
  for (int32_t rc = tree_intarr_first(t, &it, &key, &value); rc == 0;
       rc = tree_intarr_next(&it, &key, &value)) {
    printf("%3d :", key);
    array_int32_t *arr = value;
    for (size_t i = 0; i < array_int32_size(arr); i++) {
      printf(" %d", array_int32_get(arr, i));
    }
    printf("\n");
  }

  tree_intarr_destroy(&t);
  return 0;
}
