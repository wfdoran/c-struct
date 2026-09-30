#include <check.h>
#include <stdlib.h>
#include <unistd.h>

#define hkey_t int
#define value_t int
#define prefix pi
#include <phash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

typedef struct {
  int a;
  int b;
} phpair_t;

#define hkey_t phpair_t
#define value_t int
#define prefix pp
#include <phash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

static int add(int previous, int current) {
  return previous + current;
}

// The smallest positive key that a 16 slot table stores at slot 0 (the table
// mixes the hash, and the mixed hash of key 0 is 0, so key 0 also starts there).
static int key_at_slot0(void) {
  for (int k = 1;; k++) {
    if ((hash_mix64((uint64_t) k) & 15) == 0) {
      return k;
    }
  }
}

static uint64_t pair_hash(phpair_t p) {
  return (uint64_t) p.a * 31 + (uint64_t) p.b;
}

START_TEST(phash_table_test1)

phtable_pi_t *h = phash_pi_init(0);
CHECK(h != NULL);
CHECK(phash_pi_get_size(h) == 0);
CHECK(phash_pi_get_capacity(h) == 16);

phash_pi_destroy(&h);
CHECK(h == NULL);

phash_pi_destroy(&h);
CHECK(h == NULL);

END_TEST

START_TEST(phash_table_test2)

phtable_pi_t *h = phash_pi_init(0);

int v = -1;
CHECK(phash_pi_get(h, 7, &v) == 0);
CHECK(phash_pi_put(h, 7, 70) == 0);
CHECK(phash_pi_get(h, 7, &v) == 1);
CHECK(v == 70);
CHECK(phash_pi_put(h, 7, 71) == 0);
CHECK(phash_pi_get(h, 7, &v) == 1);
CHECK(v == 71);
CHECK(phash_pi_get_size(h) == 1);

CHECK(phash_pi_remove(h, 8, &v) == 0);
CHECK(phash_pi_remove(h, 7, &v) == 1);
CHECK(v == 71);
CHECK(phash_pi_remove(h, 7, &v) == 0);
CHECK(phash_pi_get_size(h) == 0);

CHECK(phash_pi_put(NULL, 1, 1) == -1);
CHECK(phash_pi_get(NULL, 1, &v) == -1);
CHECK(phash_pi_remove(NULL, 1, &v) == -1);

phash_pi_destroy(&h);

END_TEST

START_TEST(phash_table_test3)

int n = 10000;
phtable_pi_t *h = phash_pi_init(0);
for (int i = 0; i < n; i++) {
  CHECK(phash_pi_put(h, i, i * 2) == 0);
}
CHECK(phash_pi_get_size(h) == n);
for (int i = 0; i < n; i++) {
  int v;
  CHECK(phash_pi_get(h, i, &v) == 1);
  CHECK(v == 2 * i);
}
CHECK(phash_pi_get(h, -1, NULL) == 0);

phash_pi_destroy(&h);

END_TEST

START_TEST(phash_table_test4)

phtable_pi_t *h = phash_pi_init(0);

// a deleted marker sits exactly where key 0 starts probing
int k0 = key_at_slot0();
CHECK(phash_pi_put(h, k0, 1) == 0);
CHECK(phash_pi_remove(h, k0, NULL) == 1);
CHECK(phash_pi_remove(h, 0, NULL) == 0);
CHECK(phash_pi_get(h, 0, NULL) == 0);
CHECK(phash_pi_get_size(h) == 0);

phash_pi_put(h, 5, 1);
phash_pi_remove(h, 5, NULL);
phash_pi_put(h, 5, 2);
CHECK(phash_pi_get_size(h) == 1);

// Deleted markers must not make lookups of absent keys loop forever, and
// removing an absent key (including 0) must not touch the marker.
alarm(60);
for (int i = 0; i < 100000; i++) {
  phash_pi_put(h, i * 7919 + 1, i);
  phash_pi_remove(h, i * 7919 + 1, NULL);
}
CHECK(phash_pi_get_size(h) == 1);
CHECK(phash_pi_get(h, -1, NULL) == 0);
CHECK(phash_pi_remove(h, 0, NULL) == 0);
CHECK(phash_pi_remove(h, -1, NULL) == 0);
alarm(0);

phash_pi_destroy(&h);

END_TEST

START_TEST(phash_table_test5)

phtable_pi_t *h = phash_pi_init(100);
CHECK(phash_pi_get_capacity(h) == 256);

phash_pi_set_update(h, add);
phash_pi_put(h, 1, 10);
phash_pi_put(h, 1, 5);
int v;
CHECK(phash_pi_get(h, 1, &v) == 1);
CHECK(v == 15);

CHECK(phash_pi_atomic_update(h, 1, 1, add) == 0);
CHECK(phash_pi_atomic_update(h, 2, 40, add) == 0);
CHECK(phash_pi_get(h, 1, &v) == 1);
CHECK(v == 16);
CHECK(phash_pi_get(h, 2, &v) == 1);
CHECK(v == 40);
CHECK(phash_pi_atomic_update(h, 2, 1, NULL) == -1);
CHECK(phash_pi_atomic_update(NULL, 2, 1, add) == -1);

phash_pi_destroy(&h);

END_TEST

// A key type without a default hash function needs one from the user, and a
// missing hash function is reported instead of crashing or leaking the lock.
START_TEST(phash_table_test6)

phtable_pp_t *h = phash_pp_init(0);
CHECK(h != NULL);

phpair_t k = {1, 2};
int v = 0;
CHECK(phash_pp_put(h, k, 1) == -1);
CHECK(phash_pp_get(h, k, &v) == -1);
CHECK(phash_pp_remove(h, k, &v) == -1);

// none of the failed calls may leave the lock held
CHECK(pthread_rwlock_trywrlock(&(h->rwlock)) == 0);
pthread_rwlock_unlock(&(h->rwlock));

phash_pp_set_hash(h, pair_hash);
CHECK(phash_pp_put(h, k, 1) == 0);
CHECK(phash_pp_get(h, k, &v) == 1);
CHECK(v == 1);

phash_pp_destroy(&h);

END_TEST

// Many threads updating the same keys: no update may be lost.
START_TEST(phash_table_test7)

int keys = 100;
int per_key = 2000;
phtable_pi_t *h = phash_pi_init(0);

#pragma omp parallel for num_threads(4)
for (int i = 0; i < keys * per_key; i++) {
  phash_pi_atomic_update(h, i % keys, 1, add);
}

CHECK(phash_pi_get_size(h) == keys);
for (int k = 0; k < keys; k++) {
  int v = 0;
  CHECK(phash_pi_get(h, k, &v) == 1);
  CHECK(v == per_key);
}

phash_pi_destroy(&h);

END_TEST

// Concurrent inserts of disjoint keys, with readers running at the same time.
START_TEST(phash_table_test8)

int n = 40000;
phtable_pi_t *h = phash_pi_init(0);
int bad = 0;

#pragma omp parallel for num_threads(4)
for (int i = 0; i < n; i++) {
  if (phash_pi_put(h, i, i + 1) != 0) {
    #pragma omp atomic
    bad++;
  }
  int v = 0;
  int rc = phash_pi_get(h, i, &v);
  if (rc != 1 || v != i + 1) {
    #pragma omp atomic
    bad++;
  }
}

CHECK(bad == 0);
CHECK(phash_pi_get_size(h) == n);
for (int i = 0; i < n; i++) {
  int v;
  CHECK(phash_pi_get(h, i, &v) == 1);
  CHECK(v == i + 1);
}

phash_pi_destroy(&h);

END_TEST

// Concurrent removes and re-inserts keep the size consistent.
START_TEST(phash_table_test9)

int n = 20000;
phtable_pi_t *h = phash_pi_init(0);
for (int i = 0; i < n; i++) {
  phash_pi_put(h, i, i);
}

#pragma omp parallel for num_threads(4)
for (int i = 0; i < n; i++) {
  phash_pi_remove(h, i, NULL);
  if (i % 2 == 0) {
    phash_pi_put(h, i, -i);
  }
}

CHECK(phash_pi_get_size(h) == n / 2);
for (int i = 0; i < n; i++) {
  int v;
  int rc = phash_pi_get(h, i, &v);
  if (i % 2 == 0) {
    CHECK(rc == 1);
    CHECK(v == -i);
  } else {
    CHECK(rc == 0);
  }
}

phash_pi_destroy(&h);

END_TEST
