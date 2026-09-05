#include <stdio.h>
#include <stdlib.h>

#define data_t double
#define prefix double
#include <option.h>
#undef prefix
#undef data_t


option_double_t add(option_double_t a, option_double_t b) {
  if (option_double_is_set(a) && option_double_is_set(b)) {
    return option_double_init(option_double_force_get(a) +
                             option_double_force_get(b));
  } else {
    return option_double_init_empty();
  }

}

int main(void) {
  option_double_t *a = malloc(sizeof(option_double_t));
  *a = option_double_init(2.3);

  option_double_t *b = malloc(sizeof(option_double_t));
  *b = option_double_init(3.4);

  option_double_t *c = malloc(sizeof(option_double_t));
  *c = add(*a, *b);

  printf("%8.4f\n", option_double_force_get(*c));
  

  free(a);
  free(b);
  free(c);
  return 0;
}
  
