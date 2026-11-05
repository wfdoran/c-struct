#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define data_t int
#define prefix int
#define value_t int
#include <tree.h>
#undef value_t
#undef prefix
#undef data_t

// counts the occurrences of a key: each insert passes 1
int update(int prev, int curr) {
  return prev + curr;
}

int main(void) {
  srand(time(NULL));

  tree_int_t *t = tree_int_init();
  tree_int_set_update(t, &update);

  int n = 100;
  int samples = 1000;

  for (int i = 0; i < samples; i++) {
    int v = rand() % n;
    tree_int_insert(t, v, 1);
  }
  printf("Tree Size: %zu\n", tree_int_size(t));
  printf("Tree Height: %d\n", tree_int_height(t));
  printf("\n");

  titer_int_t it;
  int key;
  int value = 0;
  for (int32_t rc = tree_int_first(t, &it, &key, &value); rc == 0;
       rc = tree_int_next(&it, &key, &value)) {
    printf("%5d %5d\n", key, value);
  }
  printf("\n");

  tree_int_destroy(&t);
  return 0;
}
