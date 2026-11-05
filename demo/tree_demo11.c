#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define data_t int32_t
#define prefix int32
#define value_t int
#include <tree.h>
#undef value_t
#undef prefix
#undef data_t

void node_print(int32_t x, int unused) {
  (void) unused;
  printf("%d", x);
}

int main(void) {
  tree_int32_t *t = tree_int32_init();

  tree_int32_insert(t, 4, 0);
  tree_int32_insert(t, 2, 0);
  tree_int32_insert(t, 6, 0);
  tree_int32_insert(t, 1, 0);
  tree_int32_insert(t, 3, 0);
  tree_int32_insert(t, 5, 0);
  tree_int32_insert(t, 7, 0);

  tree_int32_print(t, node_print);

  printf("\n");

  tree_int32_delete(t, 4);
  tree_int32_print(t, node_print);

  tree_int32_destroy(&t);

  return 0;
}
