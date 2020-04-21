#include <stdlib.h>
#include <stdio.h>

#define key_t int
#define value_t int
#define prefix int
#include <hash_table.h>
#undef prefix
#undef value_t
#undef key_t

int main() {
  char* a = "Bill Doran";
  uint64_t (*my_hash)(char*) = DEFAULT_HASH(a);
  uint64_t a_hash = my_hash(a);
  printf("%016lx\n", a_hash);

  htable_int_t *h = hash_int_init(0);
  
  return 0;
}


