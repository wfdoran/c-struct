#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define data_t int64_t
#define prefix int64
#include <tree.h>
#undef prefix
#undef data_t

int main(void) {
  srand(time(NULL));
  int32_t num_items = 1000000;

  tree_int64_t *t = tree_int64_init();

  for (int i = 0; i < num_items; i++) {
    uint64_t bits = 0; /* unsigned: the shifts push bits off the top */
    for (int j = 0; j < 5; j++) {
      bits = (bits << 13) + (uint64_t) rand();
    }
    tree_int64_insert(t, (int64_t) bits, NULL);
  }
  printf("Tree Size: %zu\n", tree_int64_size(t));
  printf("Tree Height: %d\n", tree_int64_height(t));
  printf("\n");

  tree_int64_destroy(&t);

  return 0;
}
