#include <stdio.h>
#include <stdlib.h>

/* By default the largest key comes out first.  With data_less the order is
   compiled in, so pqueue_prefix_set_comp is not needed; here it is reversed,
   which gives a min-heap. */

#define data_t int
#define prefix int
#define data_less(a, b) ((a) > (b))
#include <pqueue.h>
#undef prefix
#undef data_t

int main(void) {
  pqueue_int_t *q = pqueue_int_init();

  for (int i = 0; i < 10; i++) {
    pqueue_int_push(q, (i * 7) % 10, NULL);
  }

  while (!pqueue_int_is_empty(q)) {
    pqkv_int_t top = pqueue_int_pop(q);
    printf("%d ", top.key);
  }
  printf("\n");

  pqueue_int_destroy(&q);
  return 0;
}
