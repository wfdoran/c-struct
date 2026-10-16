/* A thread safe pool shared by four threads: each takes some objects, fills
   them in, checks them and gives them back.  Build with -pthread. */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
  int64_t owner;
  int64_t value;
} cell_t;

#define data_t cell_t
#define prefix cell
#include <ppool.h>
#undef prefix
#undef data_t

#define THREADS 4
#define HELD 8
#define ROUNDS 10000

static ppool_cell_t *pool;
static int bad[THREADS];

static void *worker(void *arg) {
  int64_t me = (int64_t) (intptr_t) arg;
  for (int r = 0; r < ROUNDS; r++) {
    cell_t *c[HELD];
    for (int i = 0; i < HELD; i++) {
      c[i] = ppool_cell_get(pool);
      c[i]->owner = me;
      c[i]->value = r * HELD + i;
    }
    for (int i = 0; i < HELD; i++) {
      if (c[i]->owner != me || c[i]->value != r * HELD + i) {
        bad[me]++;
      }
      ppool_cell_put(pool, &c[i]);
    }
  }
  return NULL;
}

int main(void) {
  pool = ppool_cell_init(16);

  pthread_t t[THREADS];
  for (intptr_t i = 0; i < THREADS; i++) {
    pthread_create(&t[i], NULL, worker, (void *) i);
  }
  for (int i = 0; i < THREADS; i++) {
    pthread_join(t[i], NULL);
  }

  int total = 0;
  for (int i = 0; i < THREADS; i++) {
    total += bad[i];
  }
  printf("objects overwritten by another thread: %d\n", total);

  ppool_cell_destroy(&pool);
  return total != 0;
}
