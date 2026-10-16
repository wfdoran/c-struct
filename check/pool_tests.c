#include <check.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

// the smallest data_t: the entry is much larger than the data
#define data_t char
#define prefix ch
#include <pool.h>
#undef prefix
#undef data_t

// An instantiation whose allocations can be made to fail.
static long poom_budget = -1;   // allocations which may still succeed; -1 for no limit

static __attribute__((noinline)) void *poom_malloc(size_t n) {
  if (poom_budget == 0) {
    return NULL;
  }
  if (poom_budget > 0) {
    poom_budget--;
  }
  return malloc(n);
}

#define malloc poom_malloc
#define data_t long
#define prefix oom
#include <pool.h>
#undef prefix
#undef data_t
#undef malloc

static int compare_pointers(const void *a, const void *b) {
  uintptr_t x = (uintptr_t) *(circle_t *const *) a;
  uintptr_t y = (uintptr_t) *(circle_t *const *) b;
  return (x > y) - (x < y);
}

// Takes n objects from the pool, writes a different value to each, and checks that they are
// all different, aligned, and still hold what was written.
static int take_and_check(pool_circ_t *pool, circle_t **v, size_t n, double base) {
  for (size_t i = 0; i < n; i++) {
    v[i] = pool_circ_get(pool);
    CHECK(v[i] != NULL);
    CHECK((uintptr_t) v[i] % _Alignof(circle_t) == 0);
    v[i]->x = base + (double) i;
    v[i]->y = -v[i]->x;
    v[i]->r = 0.5 * v[i]->x;
  }
  for (size_t i = 0; i < n; i++) {
    CHECK(v[i]->x == base + (double) i && v[i]->y == -v[i]->x && v[i]->r == 0.5 * v[i]->x);
  }

  // all different: sort a copy of the pointers and compare the neighbors
  circle_t **copy = malloc(n * sizeof(circle_t *));
  CHECK(copy != NULL);
  memcpy(copy, v, n * sizeof(circle_t *));
  qsort(copy, n, sizeof(circle_t *), compare_pointers);
  for (size_t i = 1; i < n; i++) {
    CHECK(copy[i - 1] != copy[i]);
  }
  free(copy);
  return 0;
}

// the size of the next block: 1.5 times the last, rounded up
START_TEST(pool_test0)

#if CS_POOL_GROWTH_NUM == 3 && CS_POOL_GROWTH_DEN == 2
CHECK(cs_pool_next_size(1) == 2);
CHECK(cs_pool_next_size(2) == 3);
CHECK(cs_pool_next_size(3) == 5);
CHECK(cs_pool_next_size(4) == 6);
CHECK(cs_pool_next_size(5) == 8);
CHECK(cs_pool_next_size(100) == 150);
CHECK(cs_pool_next_size(101) == 152);
#endif
for (size_t n = 1; n < 100000; n++) {
  size_t m = cs_pool_next_size(n);
  CHECK(m > n);                       // it always grows
  // NUM / DEN times n, rounded up
  CHECK(m * CS_POOL_GROWTH_DEN >= n * CS_POOL_GROWTH_NUM &&
        m * CS_POOL_GROWTH_DEN < n * CS_POOL_GROWTH_NUM + CS_POOL_GROWTH_DEN);
}
// no overflow: a huge block saturates, which init then refuses
CHECK(cs_pool_next_size(SIZE_MAX) == SIZE_MAX);
CHECK(cs_pool_next_size(SIZE_MAX - 1) == SIZE_MAX);
CHECK(cs_pool_next_size(SIZE_MAX / 2) > SIZE_MAX / 2);
CHECK(cs_pool_next_size(SIZE_MAX / 2 + 1) >= SIZE_MAX / 2 + 1);
CHECK(pool_circ_init(cs_pool_next_size(SIZE_MAX)) == NULL);

END_TEST

// get gives different, writable objects, and the pool grows by blocks 1.5 times the size
START_TEST(pool_test1)

enum { N = 5000 };
static circle_t *v[N];

pool_circ_t *pool = pool_circ_init(3);
CHECK(pool != NULL);
CHECK(take_and_check(pool, v, N, 1.0) == 0);

// the blocks are 3, 5, 8, 12, ... entries, and every object is inside one of them
size_t total = 0;
size_t expect = 3;
size_t last = 0;
for (pool_circ_t *b = pool; b != NULL; b = b->next) {
  CHECK(b->pool_size == expect);
  total += b->pool_size;
  last = b->pool_size;
  expect = cs_pool_next_size(expect);
}
CHECK(total >= N && total - last < N);         // no more blocks than needed
for (size_t i = 0; i < N; i++) {
  bool inside = false;
  for (pool_circ_t *b = pool; b != NULL; b = b->next) {
    inside = inside || (v[i] >= (circle_t *) b->alloc &&
                        (char *) v[i] < (char *) b->alloc + b->pool_size * sizeof(*b->alloc));
  }
  CHECK(inside);
}

for (size_t i = 0; i < N; i++) {
  CHECK(pool_circ_put(pool, &v[i]) == 0);
}
CHECK(pool_circ_destroy(&pool) == 0);

END_TEST

// space which is given back is used again, most recent first, and nothing else is disturbed
START_TEST(pool_test2)

enum { N = 100 };
circle_t *v[N];

pool_circ_t *pool = pool_circ_init(4);
CHECK(take_and_check(pool, v, N, 10.0) == 0);

// put clears the pointer it is given
circle_t *a = v[10], *b = v[20], *c = v[30];
CHECK(pool_circ_put(pool, &v[10]) == 0 && v[10] == NULL);
CHECK(pool_circ_put(pool, &v[20]) == 0 && v[20] == NULL);
CHECK(pool_circ_put(pool, &v[30]) == 0 && v[30] == NULL);

// get returns them, the last one given back first
CHECK(pool_circ_get(pool) == c);
CHECK(pool_circ_get(pool) == b);
CHECK(pool_circ_get(pool) == a);
v[10] = a;
v[20] = b;
v[30] = c;

// the objects still in use were not touched
for (size_t i = 0; i < N; i++) {
  v[i]->x = 10.0 + (double) i;
}

// give back all and take all again: again different objects, and they can be written
for (size_t i = 0; i < N; i++) {
  CHECK(pool_circ_put(pool, &v[i]) == 0);
}
CHECK(take_and_check(pool, v, N, 500.0) == 0);

// the pool did not grow while space was available
size_t blocks = 0;
for (pool_circ_t *p = pool; p != NULL; p = p->next) {
  blocks++;
}
size_t needed = 0;                               // the blocks which N objects need, from a first block of 4
for (size_t size = 4, held = 0; held < N; size = cs_pool_next_size(size)) {
  held += size;
  needed++;
}
CHECK(blocks == needed);

CHECK(pool_circ_destroy(&pool) == 0);

END_TEST

// the arguments which cannot be used
START_TEST(pool_test3)

// a pool needs room for at least one object, and the size of the block must fit
CHECK(pool_circ_init(0) == NULL);
CHECK(pool_circ_init(SIZE_MAX) == NULL);
CHECK(pool_circ_init(SIZE_MAX / sizeof(pentry_circ_t) + 1) == NULL);
// n * sizeof(entry) wraps around to a small number: this must not be allocated and then written
CHECK(pool_circ_init(SIZE_MAX / sizeof(pentry_circ_t) + 2) == NULL);
CHECK(pool_ch_init(SIZE_MAX / sizeof(pentry_ch_t) + 2) == NULL);

CHECK(pool_circ_get(NULL) == NULL);

pool_circ_t *pool = pool_circ_init(2);
CHECK(pool != NULL);
circle_t *a = pool_circ_get(pool);
circle_t *none = NULL;
CHECK(pool_circ_put(NULL, &a) == -1 && a != NULL);        // the pointer is left alone
CHECK(pool_circ_put(pool, NULL) == -1);
CHECK(pool_circ_put(pool, &none) == -1);
CHECK(pool_circ_put(pool, &a) == 0 && a == NULL);

CHECK(pool_circ_destroy(NULL) == -1);
CHECK(pool_circ_destroy(&pool) == 0 && pool == NULL);
CHECK(pool_circ_destroy(&pool) == -1);                    // already destroyed

// destroy frees the blocks whether or not the space in them was given back
pool = pool_circ_init(2);
for (int i = 0; i < 20; i++) {
  CHECK(pool_circ_get(pool) != NULL);
}
CHECK(pool_circ_destroy(&pool) == 0);

END_TEST

// space is given back once: a second put, through a copy of the pointer, is refused
START_TEST(pool_test4)

// the pool is full: the first thing given back is the whole free list
pool_circ_t *pool = pool_circ_init(2);
circle_t *a = pool_circ_get(pool);
circle_t *b = pool_circ_get(pool);
circle_t *a2 = a;
CHECK(pool_circ_put(pool, &a) == 0);
CHECK(pool_circ_put(pool, &a2) == -1);
CHECK(a2 != NULL);                                        // refused: not cleared
circle_t *x = pool_circ_get(pool);
CHECK(x == a2);
CHECK(pool_circ_get(pool) != x);                          // not the same object again
CHECK(b != x);
CHECK(pool_circ_destroy(&pool) == 0);

// the first thing given back is the last of the free list
pool = pool_circ_init(2);
a = pool_circ_get(pool);
b = pool_circ_get(pool);
a2 = a;
CHECK(pool_circ_put(pool, &a) == 0 && pool_circ_put(pool, &b) == 0);   // b -> a
CHECK(pool_circ_put(pool, &a2) == -1);
CHECK(pool_circ_destroy(&pool) == 0);

// the middle and the head of a longer free list, with unused entries after it
pool = pool_circ_init(6);
a = pool_circ_get(pool);
b = pool_circ_get(pool);
circle_t *c = pool_circ_get(pool);
a2 = a;
circle_t *b2 = b, *c2 = c;
CHECK(pool_circ_put(pool, &b) == 0 && pool_circ_put(pool, &c) == 0 && pool_circ_put(pool, &a) == 0);
CHECK(pool_circ_put(pool, &b2) == -1);                    // middle
CHECK(pool_circ_put(pool, &c2) == -1);
CHECK(pool_circ_put(pool, &a2) == -1);                    // head
CHECK(pool_circ_destroy(&pool) == 0);

// space which belongs to another pool, and space which was never handed out
pool_circ_t *p = pool_circ_init(3);
pool_circ_t *q = pool_circ_init(3);
x = pool_circ_get(q);
circle_t *keep = x;
CHECK(pool_circ_put(p, &x) == -1 && x == keep);
CHECK(pool_circ_put(q, &x) == 0 && x == NULL);
CHECK(pool_circ_destroy(&p) == 0 && pool_circ_destroy(&q) == 0);

// refused puts change nothing: everything handed out afterwards is still different
enum { N = 60 };
circle_t *v[N], *copy[N];
pool = pool_circ_init(3);
CHECK(take_and_check(pool, v, N, 1.0) == 0);
memcpy(copy, v, sizeof(v));
for (size_t i = 0; i < N; i++) {
  CHECK(pool_circ_put(pool, &v[i]) == 0);
}
for (size_t i = 0; i < N; i++) {
  CHECK(pool_circ_put(pool, &copy[i]) == -1);
}
CHECK(take_and_check(pool, v, N, 1000.0) == 0);
CHECK(pool_circ_destroy(&pool) == 0);

END_TEST

// every size of the first block: it holds exactly that many objects, and the next block is 1.5
// times as large, rounded up (the sizes next to 1, 2 and 3 are where an off by one in linking the entries shows)
START_TEST(pool_test5)

for (size_t n = 1; n <= 40; n++) {
  pool_circ_t *pool = pool_circ_init(n);
  CHECK(pool != NULL);
  circle_t *v[200];

  for (size_t i = 0; i < n; i++) {
    v[i] = pool_circ_get(pool);
    CHECK(v[i] != NULL);
    CHECK(pool->next == NULL);                  // still in the first block
  }
  CHECK(pool->head == NULL);                    // and it is now used up, no more and no less

  v[n] = pool_circ_get(pool);                   // the next one starts the second block
  CHECK(v[n] != NULL);
  const size_t s2 = cs_pool_next_size(n);
  const size_t s3 = cs_pool_next_size(s2);
  CHECK(pool->next != NULL && pool->next->pool_size == s2);
  for (size_t i = n + 1; i < n + s2; i++) {
    v[i] = pool_circ_get(pool);
    CHECK(v[i] != NULL);
  }
  CHECK(pool->next->next == NULL);              // n + s2 objects fit in the two blocks
  CHECK(pool->head == NULL);

  v[n + s2] = pool_circ_get(pool);              // the next one starts a third block
  CHECK(v[n + s2] != NULL && pool->next->next != NULL && pool->next->next->pool_size == s3);

  for (size_t i = 0; i < n + s2 + 1 && i < 200; i++) {
    for (size_t j = i + 1; j < n + s2 + 1 && j < 200; j++) {
      CHECK(v[i] != v[j]);
    }
  }
  CHECK(pool_circ_destroy(&pool) == 0);
}

// a data_t much smaller than the entry
pool_ch_t *chars = pool_ch_init(1);
char *s[100];
for (int i = 0; i < 100; i++) {
  s[i] = pool_ch_get(chars);
  CHECK(s[i] != NULL);
  *s[i] = (char) i;
}
for (int i = 0; i < 100; i++) {
  CHECK(*s[i] == (char) i);
}
CHECK(pool_ch_destroy(&chars) == 0);

END_TEST

// out of memory: a failure to grow leaves the pool as it was, and space which was given back
// can still be taken, since that needs no memory
//
// Only the first allocation of pool_init and of growth is made to fail here.  When the second
// one fails (the entries of the block) pool_init returns NULL and leaves the pool struct it had
// just allocated unfreed, which is a known leak on the error path that is not fixed.
START_TEST(pool_test6)

poom_budget = 0;
CHECK(pool_oom_init(4) == NULL);
poom_budget = -1;

pool_oom_t *pool = pool_oom_init(2);
CHECK(pool != NULL);
long *a = pool_oom_get(pool);
long *b = pool_oom_get(pool);
CHECK(a != NULL && b != NULL);
*a = 11;
*b = 22;

// the pool is used up, and growing needs memory
poom_budget = 0;
CHECK(pool_oom_get(pool) == NULL);
CHECK(pool_oom_get(pool) == NULL);                        // and again: nothing was changed
CHECK(pool->next == NULL && pool->head == NULL);
CHECK(*a == 11 && *b == 22);

// giving back and taking again need no memory
long *b2 = b;
CHECK(pool_oom_put(pool, &b) == 0);
CHECK(pool_oom_get(pool) == b2);
CHECK(pool_oom_put(pool, &b2) == 0);
long *got = pool_oom_get(pool);
CHECK(got != NULL);                                       // from the free list
CHECK(pool_oom_get(pool) == NULL);                        // used up again

// with memory the pool grows, and what was there before is intact
poom_budget = -1;
long *more[10];
for (int i = 0; i < 10; i++) {
  more[i] = pool_oom_get(pool);
  CHECK(more[i] != NULL);
  *more[i] = 100 + i;
}
CHECK(*a == 11);
for (int i = 0; i < 10; i++) {
  CHECK(*more[i] == 100 + i);
  for (int j = i + 1; j < 10; j++) {
    CHECK(more[i] != more[j]);
  }
  CHECK(more[i] != a && more[i] != got);
}
CHECK(pool_oom_destroy(&pool) == 0);

END_TEST

// a long random sequence of get and put against a record of what is in use: no two objects
// in use are the same, each keeps its own contents, and a put of space not in use is refused
START_TEST(pool_test7)

enum { MAX_LIVE = 1500 };
static circle_t *live[MAX_LIVE];
static circle_t *dead[MAX_LIVE];          // pointers which were given back (may be in use again)
size_t n_live = 0, n_dead = 0;
int refused = 0;

seed_rand(7);
pool_circ_t *pool = pool_circ_init(8);
CHECK(pool != NULL);

for (int step = 0; step < 100000; step++) {
  uint32_t op = get_rand() % 8;

  if ((op < 4 && n_live < MAX_LIVE) || n_live == 0) {
    circle_t *p = pool_circ_get(pool);
    CHECK(p != NULL);
    p->x = (double) step;                 // a value of its own
    p->y = (double) (step % 97);
    live[n_live++] = p;
  } else if (op < 7) {
    size_t i = get_rand() % n_live;
    circle_t *p = live[i];
    live[i] = live[--n_live];
    if (n_dead < MAX_LIVE) {
      dead[n_dead++] = p;
    } else {
      dead[get_rand() % MAX_LIVE] = p;
    }
    circle_t *again = p;
    CHECK(pool_circ_put(pool, &p) == 0 && p == NULL);
    if (get_rand() % 4 == 0) {
      // right after it was given back it is certainly not in use: a second put is refused
      CHECK(pool_circ_put(pool, &again) == -1 && again != NULL);
      refused++;
    }
  } else if (n_dead > 0) {
    // try to give back a pointer which is not in use now: it must be refused
    circle_t *q = dead[get_rand() % n_dead];
    bool in_use = false;
    for (size_t i = 0; i < n_live; i++) {
      in_use = in_use || live[i] == q;
    }
    if (!in_use) {
      CHECK(pool_circ_put(pool, &q) == -1 && q != NULL);
      refused++;
    }
  }

  if (step % 5000 == 0) {
    // everything in use has its own value (a shared object would have been overwritten)
    for (size_t i = 0; i < n_live; i++) {
      CHECK(live[i]->y == (double) (((int) live[i]->x) % 97));
      for (size_t j = i + 1; j < n_live; j += 53) {
        CHECK(live[i] != live[j]);
      }
    }
  }
}

CHECK(refused > 1000);                    // the check ran often enough to mean something
for (size_t i = 0; i < n_live; i++) {
  for (size_t j = i + 1; j < n_live; j++) {
    CHECK(live[i] != live[j]);
  }
}
CHECK(pool_circ_destroy(&pool) == 0);

END_TEST
