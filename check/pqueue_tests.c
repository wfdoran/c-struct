#include <check.h>
#include <stdlib.h>

#define data_t int
#define prefix pqi
#include <pqueue.h>
#undef prefix
#undef data_t

#define data_t uint64_t
#define prefix pqu
#include <pqueue.h>
#undef prefix
#undef data_t

typedef struct {
  int x;
  int y;
} pqpair_t;

#define data_t pqpair_t
#define prefix pqp
#include <pqueue.h>
#undef prefix
#undef data_t

static int freed_count = 0;

static void count_free(void *p) {
  freed_count++;
  free(p);
}

static int min_first(int *a, int *b) {
  return (*a < *b) - (*a > *b);
}

static int comp_pair(pqpair_t *a, pqpair_t *b) {
  if (a->x != b->x) {
    return (a->x > b->x) - (a->x < b->x);
  }
  return (a->y > b->y) - (a->y < b->y);
}

static int comp_desc(const void *a, const void *b) {
  int x = *(const int *) a;
  int y = *(const int *) b;
  return (x < y) - (x > y);
}

#define data_t int
#define prefix dn
#include <pqueue.h>
#undef prefix
#undef data_t

START_TEST(pqueue_test1)

pqueue_pqi_t *q = pqueue_pqi_init();
CHECK(q != NULL);
CHECK(pqueue_pqi_size(q) == 0);
CHECK(pqueue_pqi_is_empty(q));

pqueue_pqi_destroy(&q);
CHECK(q == NULL);

pqueue_pqi_destroy(&q);
CHECK(q == NULL);

END_TEST

START_TEST(pqueue_test2)

pqueue_pqi_t *q = pqueue_pqi_init();

CHECK(!pqueue_pqi_pop(q).found);
CHECK(!pqueue_pqi_peek(q).found);

CHECK(pqueue_pqi_push(q, 3, NULL) == 0);
CHECK(pqueue_pqi_push(q, 9, NULL) == 0);
CHECK(pqueue_pqi_push(q, 5, NULL) == 0);
CHECK(pqueue_pqi_size(q) == 3);
CHECK(!pqueue_pqi_is_empty(q));

pqkv_pqi_t top = pqueue_pqi_peek(q);
CHECK(top.found);
CHECK(top.key == 9);
CHECK(pqueue_pqi_size(q) == 3);

CHECK(pqueue_pqi_pop(q).key == 9);
CHECK(pqueue_pqi_pop(q).key == 5);
CHECK(pqueue_pqi_pop(q).key == 3);
CHECK(!pqueue_pqi_pop(q).found);
CHECK(pqueue_pqi_is_empty(q));

CHECK(pqueue_pqi_push(NULL, 1, NULL) == -1);

pqueue_pqi_destroy(&q);

END_TEST

// Keys come out in comparator order, and values stay attached to their keys.
START_TEST(pqueue_test3)

seed_rand(3);

for (int round = 0; round < 100; round++) {
  int n = get_rand() % 300;
  int *keys = malloc((n + 1) * sizeof(int));
  pqueue_pqi_t *q = pqueue_pqi_init();

  for (int i = 0; i < n; i++) {
    keys[i] = get_rand() % 50;
    int *value = malloc(sizeof(int));
    *value = keys[i] * 10 + 1;
    CHECK(pqueue_pqi_push(q, keys[i], value) == 0);
  }
  qsort(keys, n, sizeof(int), comp_desc);

  for (int i = 0; i < n; i++) {
    pqkv_pqi_t r = pqueue_pqi_pop(q);
    CHECK(r.found);
    CHECK(r.key == keys[i]);
    CHECK(*(int *) r.value == r.key * 10 + 1);
    free(r.value);
  }
  CHECK(!pqueue_pqi_pop(q).found);

  pqueue_pqi_destroy(&q);
  free(keys);
}

END_TEST

START_TEST(pqueue_test4)

pqueue_pqi_t *q = pqueue_pqi_init();

CHECK(pqueue_pqi_set_comp(q, min_first) == 0);
CHECK(pqueue_pqi_set_comp(NULL, min_first) == -1);
CHECK(pqueue_pqi_set_comp(q, NULL) == -1);

int keys[] = {5, 1, 9, 3, 7, 1, 8};
for (int i = 0; i < 7; i++) {
  pqueue_pqi_push(q, keys[i], NULL);
}

int expected[] = {1, 1, 3, 5, 7, 8, 9};
for (int i = 0; i < 7; i++) {
  CHECK(pqueue_pqi_pop(q).key == expected[i]);
}

pqueue_pqi_destroy(&q);

END_TEST

START_TEST(pqueue_test5)

pqueue_pqi_t *q = pqueue_pqi_init();

int n = 100000;
for (int i = 0; i < n; i++) {
  CHECK(pqueue_pqi_push(q, 42, NULL) == 0);
}
CHECK(pqueue_pqi_size(q) == n);
for (int i = 0; i < n; i++) {
  pqkv_pqi_t r = pqueue_pqi_pop(q);
  CHECK(r.found);
  CHECK(r.key == 42);
}
CHECK(pqueue_pqi_is_empty(q));

pqueue_pqi_destroy(&q);

END_TEST

START_TEST(pqueue_test6)

freed_count = 0;

pqueue_pqi_t *q = pqueue_pqi_init();
pqueue_pqi_set_value_free(q, count_free);

for (int i = 0; i < 10; i++) {
  int *v = malloc(sizeof(int));
  *v = i;
  pqueue_pqi_push(q, i, v);
}

pqkv_pqi_t r = pqueue_pqi_pop(q);
CHECK(r.key == 9);
free(r.value);
CHECK(freed_count == 0);

pqueue_pqi_destroy(&q);
CHECK(freed_count == 9);

END_TEST

// Unsigned keys above 2^63 must sort as large.
START_TEST(pqueue_test7)

pqueue_pqu_t *q = pqueue_pqu_init();

uint64_t big = UINT64_C(1) << 63;
CHECK(pqueue_pqu_push(q, 5, NULL) == 0);
CHECK(pqueue_pqu_push(q, big, NULL) == 0);
CHECK(pqueue_pqu_push(q, big + 1, NULL) == 0);
CHECK(pqueue_pqu_push(q, 0, NULL) == 0);

CHECK(pqueue_pqu_pop(q).key == big + 1);
CHECK(pqueue_pqu_pop(q).key == big);
CHECK(pqueue_pqu_pop(q).key == 5);
CHECK(pqueue_pqu_pop(q).key == 0);

pqueue_pqu_destroy(&q);

END_TEST

// A struct key has no default comparator, so pushing is refused until the
// user provides one.
START_TEST(pqueue_test8)

pqueue_pqp_t *q = pqueue_pqp_init();

pqpair_t a = {1, 2};
pqpair_t b = {1, 3};
CHECK(pqueue_pqp_push(q, a, NULL) == -1);
CHECK(pqueue_pqp_size(q) == 0);

CHECK(pqueue_pqp_set_comp(q, comp_pair) == 0);
CHECK(pqueue_pqp_push(q, a, NULL) == 0);
CHECK(pqueue_pqp_push(q, b, NULL) == 0);
CHECK(pqueue_pqp_pop(q).key.y == 3);
CHECK(pqueue_pqp_pop(q).key.y == 2);

pqueue_pqp_destroy(&q);

END_TEST

// Interleaved pushes and pops against a sorted-array model.
START_TEST(pqueue_test9)

seed_rand(9);

for (int round = 0; round < 50; round++) {
  pqueue_pqi_t *q = pqueue_pqi_init();
  int model[2000];
  int count = 0;

  for (int step = 0; step < 3000; step++) {
    if (get_rand() % 3 != 0 || count == 0) {
      int k = get_rand() % 100;
      if (count < 2000) {
        CHECK(pqueue_pqi_push(q, k, NULL) == 0);
        model[count++] = k;
      }
    } else {
      int best = 0;
      for (int i = 1; i < count; i++) {
        if (model[i] > model[best]) {
          best = i;
        }
      }
      pqkv_pqi_t r = pqueue_pqi_pop(q);
      CHECK(r.found);
      CHECK(r.key == model[best]);
      model[best] = model[--count];
    }
    CHECK(pqueue_pqi_size(q) == (size_t) count);
  }

  pqueue_pqi_destroy(&q);
}

END_TEST

// destroy accepts a NULL pointer and an already destroyed container
START_TEST(pqueue_test10)

pqueue_dn_t *q = NULL;
pqueue_dn_destroy(NULL);
pqueue_dn_destroy(&q);
CHECK(q == NULL);
q = pqueue_dn_init();
pqueue_dn_destroy(&q);
pqueue_dn_destroy(&q);
CHECK(q == NULL);

END_TEST
