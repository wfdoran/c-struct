#include <stdio.h>
#include <stdint.h>

typedef struct {
  double x;
  double y;
  double r;
} circle_t;

#define data_t circle_t
#define prefix circ

#include <pool.h>

#undef prefix
#undef data_t

int main(void) {
  pool_circ_t *pool = pool_circ_init(10);

  circle_t *x[20];

  for (int32_t i = 0; i < 20; i++) {
    x[i] = pool_circ_get(pool);
  }

  for (int32_t i = 0; i < 20; i++) {
    pool_circ_put(pool, &x[i]);
  }

  pool_circ_destroy(&pool);
  return 0;
}
