#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define data_t int
#define prefix int
#include <array.h>
#undef prefix
#undef data_t

#define key_t int
#define value_t array_int_t*
#define prefix intp
#include <hash_table.h>
#undef prefix
#undef value_t
#undef key_t

array_int_t* update(array_int_t *curr, array_int_t *new) {
  for (int i = 0; i < array_int_size(new); i++) {
    int v = array_int_get(new,i);
    array_int_append(curr, v);
  }
  array_int_t *temp = new;
  array_int_destroy(&temp);
  return curr;
}

int main(void) {
  srand48(time(NULL));
  
  htable_intp_t *h = hash_intp_init(0);
  hash_intp_set_update(h, update);


  int v_sum = 0;
  int v_xor = 0;
  int items = 0;
  
  for (int trial = 0; trial < 3000; trial++) {
    int k = lrand48() % 1000;
    int v = lrand48() & 0xffffff;

    v_sum += v;
    v_xor ^= v;
    items++;
    
    array_int_t *new = array_int_init();
    array_int_append(new, v);

    hash_intp_put(h, k, new); 
  }

  hiter_intp_t *iter;
  int key;
  array_int_t *value;

  for (hash_intp_first(h, &iter, &key, &value); iter != NULL; hash_intp_next(&iter, &key, &value)) {
    printf("%4d %4ld :", key, array_int_size(value));
    for (int i = 0; i < array_int_size(value); i++) {
      int v = array_int_get(value, i);
      v_sum -= v;
      v_xor ^= v;
      printf("%06x ", v);
    }
    printf("\n");
    items -= array_int_size(value);
    array_int_destroy(&value);
  }

  printf("totals = %d %d %d\n", items, v_sum, v_xor);
  hash_intp_destroy(&h);
}
