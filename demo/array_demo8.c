#include <math.h>
#include <stdio.h>

typedef struct {
  int x;
  float y;
} pair_t;

#define data_t pair_t
#define prefix pair
#include <array.h>
#undef prefix
#undef data_t

int main(void) {
  pair_t x = {.x = 0, .y = 0.0};
  array_pair_t *a = array_pair_init2(20, x);

  for (int i = 0; i < 20; i += 2) {
    pair_t y = {.x = i, .y = sqrt(i)};
    array_pair_set(a, y, i);
  }

  for (int i = 0; i < 20; i++) {
    pair_t z = array_pair_get(a, i);
    printf("%2d %8.4f\n", z.x, z.y);
  }
 
  array_pair_destroy(&a);
  return 0;
}

