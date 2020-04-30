#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#define key_t char*
#define value_t int
#define prefix count
#include <hash_table.h>
#undef prefix
#undef value_t
#undef key_t

int update(int current, int adder) {
  return current + adder;
}

int main() {
  htable_count_t *h = hash_count_init(0);
  hash_count_set_update(h, &update);

  hash_count_put(h, "Alice", 3);
  hash_count_put(h, "Bill", 2);
  hash_count_put(h, "Charlie", 1);
  hash_count_put(h, "David", 3);
  hash_count_put(h, "Alice", 4);
  hash_count_put(h, "Bill", 2);


  hiter_count_t *iter;
  char *name;
  int count;
  for (hash_count_first(h, &iter, &name, &count); iter != NULL; hash_count_next(&iter, &name, &count)) {
    printf("%-20s %d\n", name, count);
  }

  hash_count_destroy(&h);
  return 0;
}


