#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef enum {
  TYPE_INT64,
  TYPE_DOUBLE,
} val_type_t;


typedef struct {
  val_type_t type;
  union {
    int64_t i_value;
    double d_value;
  };
} vals_t;

#define key_t char*
#define value_t vals_t
#define prefix vals
#include <hash_table.h>
#undef prefix
#undef value_t
#undef key_t
  

int main(void) {
  htable_vals_t *h = hash_vals_init(0);
  
  vals_t x = { .type = TYPE_INT64, .i_value = INT64_C(5)};
  vals_t y = { .type = TYPE_DOUBLE, .d_value = 4.5};

  hash_vals_put(h, "x", x);
  hash_vals_put(h, "y", y);

  vals_t v;
  int rc = hash_vals_get(h, "x", &v);
  assert(rc == 1);
  printf("x = %ld\n", v.i_value);
  rc = hash_vals_get(h, "y", &v);
  assert(rc == 1);
  printf("y = %.4f\n", v.d_value);

  hash_vals_destroy(&h);
  return 0;
}
