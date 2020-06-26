#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define key_t int
#define value_t int
#define prefix int
#include <phash_table.h>
#undef prefix
#undef value_t
#undef key_t

int main() {

  phtable_int_t *h = phash_int_init(0);
  phash_int_put(h, 0, 0);

  for (int i = 1; i <= 100; i++) {
    int rc = phash_int_put(h, i*i, i);
    assert(rc == 0);
  }
  phash_int_put(h, 0, -5);

  {
    int sqrt;
    int rc;

    sqrt = 0;
    rc = phash_int_get(h, 16, &sqrt);
    printf("%d %d\n", rc, sqrt);

    sqrt = 0;
    rc = phash_int_get(h, 17, &sqrt);
    printf("%d %d\n", rc, sqrt);

    sqrt = 0;
    rc = phash_int_get(h, 0, &sqrt);
    printf("%d %d\n", rc, sqrt);    
  }
  
  printf("%ld \n", h->size);


  {
    for (int i = 100; i <= 200; i++) {
      phash_int_remove(h, i, NULL);
    }
  }

  printf("number of entries %ld of %ld\n", phash_int_get_size(h), phash_int_get_capacity(h));
 
  phash_int_destroy(&h);
  return 0;
}


