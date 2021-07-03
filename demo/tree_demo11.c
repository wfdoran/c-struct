#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define data_t int32_t
#define prefix int32
#include <tree.h>
#undef prefix
#undef data_t

void node_print(int32_t x, void *unused) {
  printf("%d", x);
}

int main(void) {
  tree_int32_t *t = tree_int32_init();

  tree_int32_insert(t, 4, NULL);
  tree_int32_insert(t, 2, NULL);
  tree_int32_insert(t, 6, NULL);
  tree_int32_insert(t, 1, NULL);
  tree_int32_insert(t, 3, NULL);
  tree_int32_insert(t, 5, NULL);
  tree_int32_insert(t, 7, NULL);

  tree_int32_print(t, node_print);

  printf("\n");

  tree_int32_delete(t, 4);
  tree_int32_print(t, node_print);  
  
  tree_int32_destroy(&t);
  
  return 0;
}
  
