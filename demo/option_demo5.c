#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

typedef enum {i64, f64} data_type_t;

typedef struct {
  data_type_t type;
  union {
    int64_t i64_value;
    double f64_value;
  };
} generic_t;

#define data_t generic_t
#define prefix generic
#include <option.h>
#undef prefix
#undef data_t

void option_generic_print(option_generic_t x) {
  if (!option_generic_is_set(x)) {
    printf("empty\n");
  } else {
    generic_t v = option_generic_force_get(x);

    switch(v.type) {
    case i64:
      printf("%ld\n", v.i64_value);
      break;
    case f64:
      printf("%.8f\n", v.f64_value);
      break;
    default:
      printf("unknown\n");
    }
  }
}

generic_t f64_to_i64(generic_t v) {
  assert(v.type == f64);
  generic_t rv = {.type = i64, .i64_value = lrint(v.f64_value)};
  return rv;
}

generic_t i64_to_f64(generic_t v) {
  assert(v.type == i64);
  generic_t rv = {.type = f64, .f64_value = (double) v.i64_value};
  return rv;
}

int main(void) {
  generic_t v = {.type = f64, .f64_value = 3.5};

  option_generic_t x = option_generic_init(v);
  option_generic_print(x);

  x = option_generic_deep_clone(x, f64_to_i64);
  option_generic_print(x);

  x = option_generic_deep_clone(x, i64_to_f64);
  option_generic_print(x);

  return 0;
}
    
