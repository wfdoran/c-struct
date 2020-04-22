#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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

  for (int i = 1; i <= 100; i++) {
    int rc = hash_int_put(h, i*i, i);
    assert(rc == 0);
  }

  {
    int sqrt;
    int rc;

    sqrt = 0;
    rc = hash_int_get(h, 16, &sqrt);
    printf("%d %d\n", rc, sqrt);

    sqrt = 0;
    rc = hash_int_get(h, 17, &sqrt);
    printf("%d %d\n", rc, sqrt);
  }
    
  


  printf("%ld \n", h->size);
  return 0;
}


