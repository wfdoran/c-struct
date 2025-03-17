#include <stdio.h>
#include <stdlib.h>

#define data_t double
#define prefix float
#include <option.h>
#undef prefix
#undef data_t


option_float_t add(option_float_t a, option_float_t b) {
  if (option_float_is_set(a) && option_float_is_set(b)) {
    return option_float_init(option_float_force_get(a) +
                             option_float_force_get(b));
  } else {
    return option_float_init_empty();
  }

}

int main(void) {
  option_float_t *a = malloc(sizeof(option_float_t));
  *a = option_float_init(2.3);

  option_float_t *b = malloc(sizeof(option_float_t));
  *b = option_float_init(3.4);

  option_float_t *c = malloc(sizeof(option_float_t));
  *c = add(*a, *b);

  printf("%8.4f\n", option_float_force_get(*c));
  

  free(a);
  return 0;
}
  
