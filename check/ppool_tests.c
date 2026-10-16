#include <check.h>
#include <omp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int64_t owner;   // the thread which holds the object
  int64_t serial;  // a different value for each use
} cell_t;

#define data_t cell_t
#define prefix cell
#include <ppool.h>
#undef prefix
#undef data_t

// the plain pool of the same prefix can be used next to it
#define data_t cell_t
#define prefix cell
#include <pool.h>
#undef prefix
#undef data_t

// An instantiation whose allocations can be made to fail.
static long ppoom_budget = -1;   // allocations which may still succeed; -1 for no limit

static __attribute__((noinline)) void *ppoom_malloc(size_t n) {
  if (ppoom_budget == 0) {
    return NULL;
  }
  if (ppoom_budget > 0) {
    ppoom_budget--;
  }
  return malloc(n);
}

#define malloc ppoom_malloc
#define data_t long
#define prefix oom
#include <ppool.h>
#undef prefix
#undef data_t
#undef malloc

// the basic operations, one thread
START_TEST(ppool_test1)

CHECK(ppool_cell_init(0) == NULL);
CHECK(ppool_cell_init(SIZE_MAX) == NULL);

ppool_cell_t *p = ppool_cell_init(4);
CHECK(p != NULL);

cell_t *a = ppool_cell_get(p);
cell_t *b = ppool_cell_get(p);
CHECK(a != NULL && b != NULL && a != b);
CHECK((uintptr_t) a % _Alignof(cell_t) == 0);
a->owner = 1;
b->owner = 2;
CHECK(a->owner == 1 && b->owner == 2);

// giving back sets the pointer to NULL, and the space is handed out again
cell_t *old_a = a;
CHECK(ppool_cell_put(p, &a) == 0);
CHECK(a == NULL);
cell_t *c = ppool_cell_get(p);
CHECK(c == old_a);

// a second put through a copy of the pointer is refused, and *d_ptr is left alone
cell_t *copy = c;
CHECK(ppool_cell_put(p, &c) == 0);
CHECK(ppool_cell_put(p, &copy) == -1);
CHECK(copy == old_a);

// NULL arguments
cell_t *nul = NULL;
CHECK(ppool_cell_get(NULL) == NULL);
CHECK(ppool_cell_put(NULL, &b) == -1);
CHECK(ppool_cell_put(p, NULL) == -1);
CHECK(ppool_cell_put(p, &nul) == -1);
CHECK(b != NULL);

// space of another pool is refused
ppool_cell_t *q = ppool_cell_init(4);
cell_t *other = ppool_cell_get(q);
CHECK(ppool_cell_put(p, &other) == -1);
CHECK(other != NULL);
CHECK(ppool_cell_put(q, &other) == 0);
ppool_cell_destroy(&q);
CHECK(q == NULL);

CHECK(ppool_cell_destroy(&p) == 0);
CHECK(p == NULL);
CHECK(ppool_cell_destroy(&p) == -1);
CHECK(ppool_cell_destroy(NULL) == -1);

END_TEST

static int compare_pointers(const void *a, const void *b) {
  uintptr_t x = (uintptr_t) *(cell_t *const *) a;
  uintptr_t y = (uintptr_t) *(cell_t *const *) b;
  return (x > y) - (x < y);
}

// more objects than the first block holds: all different, and the pool keeps them
START_TEST(ppool_test2)

enum { N = 1000 };
ppool_cell_t *p = ppool_cell_init(4);
static cell_t *v[N];
for (int i = 0; i < N; i++) {
  v[i] = ppool_cell_get(p);
  CHECK(v[i] != NULL);
  v[i]->owner = i;
  v[i]->serial = ~(int64_t) i;
}
for (int i = 0; i < N; i++) {
  CHECK(v[i]->owner == i && v[i]->serial == ~(int64_t) i);
}
static cell_t *sorted[N];
memcpy(sorted, v, sizeof(v));
qsort(sorted, N, sizeof(sorted[0]), compare_pointers);
for (int i = 1; i < N; i++) {
  CHECK(sorted[i] != sorted[i - 1]);
}

// give everything back and take it again: the same N places, no new block needed
for (int i = 0; i < N; i++) {
  CHECK(ppool_cell_put(p, &v[i]) == 0);
}
static cell_t *again[N];
for (int i = 0; i < N; i++) {
  again[i] = ppool_cell_get(p);
  CHECK(again[i] != NULL);
}
qsort(again, N, sizeof(again[0]), compare_pointers);
for (int i = 0; i < N; i++) {
  CHECK(again[i] == sorted[i]);
}

ppool_cell_destroy(&p);

END_TEST

// many threads take objects, write their own number into them, check that nobody else wrote to
// them, and give them back
START_TEST(ppool_test3)

enum { THREADS = 8, ROUNDS = 2000, HELD = 16 };
ppool_cell_t *p = ppool_cell_init(8);
int bad = 0;

#pragma omp parallel num_threads(THREADS)
{
  const int64_t me = omp_get_thread_num() + 1;
  cell_t *held[HELD];
  int local_bad = 0;
  for (int r = 0; r < ROUNDS; r++) {
    for (int i = 0; i < HELD; i++) {
      held[i] = ppool_cell_get(p);
      if (held[i] == NULL) {
        local_bad++;
        continue;
      }
      held[i]->owner = me;
      held[i]->serial = r * HELD + i;
    }
    for (int i = 0; i < HELD; i++) {
      if (held[i] == NULL) {
        continue;
      }
      if (held[i]->owner != me || held[i]->serial != r * HELD + i) {
        local_bad++;
      }
      if (ppool_cell_put(p, &held[i]) != 0) {
        local_bad++;
      }
    }
  }
#pragma omp atomic
  bad += local_bad;
}
CHECK(bad == 0);

// nothing was lost: THREADS * HELD objects were in use at most, so taking that many gives that
// many different ones, none of which is in use
enum { TOTAL = THREADS * HELD };
static cell_t *all[TOTAL];
for (int i = 0; i < TOTAL; i++) {
  all[i] = ppool_cell_get(p);
  CHECK(all[i] != NULL);
}
qsort(all, TOTAL, sizeof(all[0]), compare_pointers);
for (int i = 1; i < TOTAL; i++) {
  CHECK(all[i] != all[i - 1]);
}

ppool_cell_destroy(&p);

END_TEST

// space is got by one thread and given back by another
START_TEST(ppool_test4)

enum { THREADS = 4, PER = 5000, N = THREADS * PER };
ppool_cell_t *p = ppool_cell_init(16);
static cell_t *v[N];
int bad = 0;

#pragma omp parallel num_threads(THREADS)
{
  const int me = omp_get_thread_num();
  for (int i = 0; i < PER; i++) {
    cell_t *c = ppool_cell_get(p);
    if (c == NULL) {
#pragma omp atomic
      bad++;
      continue;
    }
    c->owner = me;
    c->serial = i;
    v[me * PER + i] = c;
  }
}
CHECK(bad == 0);

// the objects were not mixed up by the threads
for (int t = 0; t < THREADS; t++) {
  for (int i = 0; i < PER; i++) {
    CHECK(v[t * PER + i]->owner == t && v[t * PER + i]->serial == i);
  }
}

// each thread gives back what the next thread got
#pragma omp parallel num_threads(THREADS)
{
  const int me = (omp_get_thread_num() + 1) % THREADS;
  for (int i = 0; i < PER; i++) {
    if (ppool_cell_put(p, &v[me * PER + i]) != 0) {
#pragma omp atomic
      bad++;
    }
  }
}
CHECK(bad == 0);
for (int i = 0; i < N; i++) {
  CHECK(v[i] == NULL);
}

ppool_cell_destroy(&p);

END_TEST

// when two threads give back the same space, exactly one of the calls succeeds
START_TEST(ppool_test5)

enum { ROUNDS = 2000 };
ppool_cell_t *p = ppool_cell_init(4);
int wins = 0;

for (int r = 0; r < ROUNDS; r++) {
  cell_t *c = ppool_cell_get(p);
  CHECK(c != NULL);
  cell_t *copies[2] = {c, c};
#pragma omp parallel num_threads(2)
  {
    int me = omp_get_thread_num();
    if (ppool_cell_put(p, &copies[me]) == 0) {
#pragma omp atomic
      wins++;
    }
  }
}
CHECK(wins == ROUNDS);

ppool_cell_destroy(&p);

END_TEST

// a ppool and a pool of the same prefix do not clash
START_TEST(ppool_test6)

ppool_cell_t *p = ppool_cell_init(2);
pool_cell_t *q = pool_cell_init(2);
cell_t *a = ppool_cell_get(p);
cell_t *b = pool_cell_get(q);
CHECK(a != NULL && b != NULL);
a->owner = 1;
b->owner = 2;
CHECK(pool_cell_put(q, &b) == 0);
CHECK(ppool_cell_put(p, &a) == 0);
ppool_cell_destroy(&p);
pool_cell_destroy(&q);

END_TEST

// allocation failures
START_TEST(ppool_test7)

ppoom_budget = 0;
CHECK(ppool_oom_init(4) == NULL);
ppoom_budget = 1;   // the pool object can be allocated, its block cannot
CHECK(ppool_oom_init(4) == NULL);
ppoom_budget = -1;

// a full pool which cannot get another block: get fails, and the pool still works
ppool_oom_t *p = ppool_oom_init(2);
CHECK(p != NULL);
long *a = ppool_oom_get(p);
long *b = ppool_oom_get(p);
CHECK(a != NULL && b != NULL);
ppoom_budget = 0;
CHECK(ppool_oom_get(p) == NULL);
ppoom_budget = -1;
CHECK(ppool_oom_put(p, &a) == 0);
long *c = ppool_oom_get(p);
CHECK(c != NULL);
long *d = ppool_oom_get(p);   // a new block
CHECK(d != NULL);
ppool_oom_destroy(&p);

END_TEST
