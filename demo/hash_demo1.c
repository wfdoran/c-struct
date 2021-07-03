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
  /* Example of using hash.h */
  char* a = "Bill Doran";
  uint64_t (*my_hash)(char*) = DEFAULT_HASH(a);
  uint64_t a_hash = my_hash(a);
  printf("hash of \"Bill Doran\" = %016lx\n", a_hash);
  printf("\n");

  /* Create a hash table which gives us inverse 
     square root for integers. */
  htable_int_t *h = hash_int_init(0);

  for (int i = 1; i <= 100; i++) {
    int rc = hash_int_put(h, i*i, i);
    assert(rc == 0);
  }
  printf("hash table size = %ld  capacity = %ld\n",
	 hash_int_get_size(h), hash_int_get_capacity(h));
  printf("\n");

  /* Try getting a few values. */
  for (int v = 1; v <= 16; v++) {
    int sqrt;
    int rc = hash_int_get(h, v, &sqrt);
    if (rc == 1) {
      printf("sqrt(%d) = %d\n", v, sqrt);
    } else {
      printf("sqrt(%d) is not an integer\n", v);
    }
  }
  printf("\n");
  
  /* Remove a few values */
  {
    for (int i = 100; i <= 200; i++) {
      int rc = hash_int_remove(h, i, NULL);
      if (rc == 1) {
	printf("Removed %d\n", i);
      }
    }
  }

  printf("hash table size = %ld  capacity = %ld\n",
	 hash_int_get_size(h), hash_int_get_capacity(h));
  printf("\n");
  
  hiter_int_t *iter;
  int key;
  int value;
  for (hash_int_first(h, &iter, &key, &value); iter != NULL; hash_int_next(&iter, &key, &value)) {
    printf("sqrt(%8d) =  %8d\n", key, value); 
  }

  hash_int_destroy(&h);
  return 0;
}


