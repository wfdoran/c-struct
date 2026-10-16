#include <check.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>

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

#define hkey_t int
#define value_t int
#define prefix dn
#include <phash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

START_TEST(phash_table_test1)

phtable_pi_t *h = phash_pi_init(0);
CHECK(h != NULL);
CHECK(phash_pi_size(h) == 0);
CHECK(phash_pi_capacity(h) == 16);

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
CHECK(phash_pi_put(h, 7, 71) == 1);   // the key was there: replaced
CHECK(phash_pi_get(h, 7, &v) == 1);
CHECK(v == 71);
CHECK(phash_pi_size(h) == 1);

CHECK(phash_pi_remove(h, 8, &v) == 0);
CHECK(phash_pi_remove(h, 7, &v) == 1);
CHECK(v == 71);
CHECK(phash_pi_remove(h, 7, &v) == 0);
CHECK(phash_pi_size(h) == 0);

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
CHECK(phash_pi_size(h) == (size_t) n);
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
CHECK(phash_pi_size(h) == 0);

phash_pi_put(h, 5, 1);
phash_pi_remove(h, 5, NULL);
phash_pi_put(h, 5, 2);
CHECK(phash_pi_size(h) == 1);

// Deleted markers must not make lookups of absent keys loop forever, and
// removing an absent key (including 0) must not touch the marker.
alarm(60);
for (int i = 0; i < 100000; i++) {
  phash_pi_put(h, i * 7919 + 1, i);
  phash_pi_remove(h, i * 7919 + 1, NULL);
}
CHECK(phash_pi_size(h) == 1);
CHECK(phash_pi_get(h, -1, NULL) == 0);
CHECK(phash_pi_remove(h, 0, NULL) == 0);
CHECK(phash_pi_remove(h, -1, NULL) == 0);
alarm(0);

phash_pi_destroy(&h);

END_TEST

START_TEST(phash_table_test5)

phtable_pi_t *h = phash_pi_init(100);
CHECK(phash_pi_capacity(h) == 256);

phash_pi_set_update(h, add);
CHECK(phash_pi_put(h, 1, 10) == 0);
CHECK(phash_pi_put(h, 1, 5) == 1);   // combined with the update function
int v;
CHECK(phash_pi_get(h, 1, &v) == 1);
CHECK(v == 15);

CHECK(phash_pi_atomic_update(h, 1, 1, add) == 1);
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

CHECK(phash_pi_size(h) == (size_t) keys);
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
CHECK(phash_pi_size(h) == (size_t) n);
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

CHECK(phash_pi_size(h) == (size_t) (n / 2));
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

// destroy accepts a NULL pointer and an already destroyed container
START_TEST(phash_table_test10)

phtable_dn_t *h = NULL;
phash_dn_destroy(NULL);
phash_dn_destroy(&h);
CHECK(h == NULL);
h = phash_dn_init(0);
phash_dn_destroy(&h);
phash_dn_destroy(&h);
CHECK(h == NULL);

END_TEST

// an absurd expected size is refused (the scaling used to overflow int64_t), a negative one is
// treated as 0
START_TEST(phash_table_test11)

phtable_dn_t *t = phash_dn_init(INT64_MAX);
CHECK(t == NULL);
t = phash_dn_init(INT64_MAX / 2);
CHECK(t == NULL);
t = phash_dn_init(INT64_C(5000000000000000000));
CHECK(t == NULL);
t = phash_dn_init(INT64_C(1) << 62);
CHECK(t == NULL);

t = phash_dn_init(INT64_MIN);
CHECK(t != NULL);
CHECK(phash_dn_capacity(t) == 16);
phash_dn_destroy(&t);

t = phash_dn_init(-5);
CHECK(t != NULL);
CHECK(phash_dn_put(t, 1, 2) == 0);
phash_dn_destroy(&t);

END_TEST

// size and capacity do not take the lock.  While threads insert distinct keys,
// a reader sees a size which never decreases and never exceeds the capacity,
// and a capacity which is a power of two and never decreases.
START_TEST(phash_table_test12)

int n = 200000;
phtable_pi_t *h = phash_pi_init(0);
int ok = 1;
int writers = 0;   // threads which insert: all but the reader
int finished = 0;  // writers which are done

#pragma omp parallel num_threads(4)
{
  #pragma omp single
  writers = omp_get_num_threads() - 1;  // implicit barrier: every thread sees it

  int tid = omp_get_thread_num();
  if (writers == 0) {
    // only one thread: insert, then read once
    for (int i = 0; i < n; i++) {
      phash_pi_put(h, i, i);
    }
  } else if (tid == 0) {
    size_t last_size = 0, last_cap = 0;
    for (;;) {
      int f;
      #pragma omp atomic read
      f = finished;
      size_t s = phash_pi_size(h);
      size_t c = phash_pi_capacity(h);
      if (s < last_size || c < last_cap || c < 16 || (c & (c - 1)) != 0 || s > c) {
        ok = 0;
      }
      last_size = s;
      last_cap = c;
      if (f == writers) {
        break;
      }
    }
  } else {
    for (int i = tid - 1; i < n; i += writers) {
      phash_pi_put(h, i, i);
    }
    #pragma omp atomic update
    finished++;
  }
}

CHECK(ok);
CHECK(phash_pi_size(h) == (size_t) n);
phash_pi_destroy(&h);

END_TEST

static uint64_t hash_other_int(int k) {
  return ~(uint64_t) k;
}

// the hash function can be set while the table is empty; with entries it is refused, since the
// entries are stored where the old function put them (and the lock is not left held)
START_TEST(phash_table_test13)

phtable_pi_t *h = phash_pi_init(0);
CHECK(h != NULL);

CHECK(phash_pi_set_hash(NULL, hash_other_int) == -1);
CHECK(phash_pi_set_hash(h, hash_other_int) == 0);            // empty: allowed
for (int i = 0; i < 100; i++) {
  CHECK(phash_pi_put(h, i, i * 3) == 0);
}

CHECK(phash_pi_set_hash(h, hash_other_int) == -1);
// the failed call released the lock; checked first, since a get would release a lock left held
// by this thread (it ignores the failure of its own read lock, then unlocks)
CHECK(pthread_rwlock_trywrlock(&(h->rwlock)) == 0);
pthread_rwlock_unlock(&(h->rwlock));
for (int i = 0; i < 100; i++) {
  int v = -1;
  CHECK(phash_pi_get(h, i, &v) == 1 && v == i * 3);
}

for (int i = 0; i < 100; i++) {
  CHECK(phash_pi_remove(h, i, NULL) == 1);
}
CHECK(phash_pi_set_hash(h, hash_other_int) == 0);
CHECK(phash_pi_put(h, 7, 70) == 0);
int v = 0;
CHECK(phash_pi_get(h, 7, &v) == 1 && v == 70);
phash_pi_destroy(&h);

END_TEST


static int phash_blocks(const phtable_pi_t *h) {
  int n = 0;
  for (const pool_phn_pi_t *p = h->pool; p != NULL; p = p->next) {
    n++;
  }
  return n;
}

// The space of removed entries is used again by later puts, also when several threads put and
// remove at once, and an empty table has no pool.
START_TEST(phash_table_test14)

phtable_pi_t *h = phash_pi_init(0);
CHECK(h->pool == NULL);
CHECK(phash_pi_remove(h, 1, NULL) == 0);
CHECK(h->pool == NULL);

enum { THREADS = 4, PER = 4000, N = THREADS * PER };
int bad = 0;
#pragma omp parallel num_threads(THREADS)
{
  const int me = omp_get_thread_num();
  for (int i = 0; i < PER; i++) {
    if (phash_pi_put(h, me * PER + i, i) != 0) {
#pragma omp atomic
      bad++;
    }
  }
}
CHECK(bad == 0 && phash_pi_size(h) == N);
const int blocks = phash_blocks(h);
CHECK(blocks > 1);

for (int round = 0; round < 5; round++) {
#pragma omp parallel num_threads(THREADS)
  {
    const int me = omp_get_thread_num();
    for (int i = 0; i < PER; i++) {
      if (phash_pi_remove(h, me * PER + i, NULL) != 1) {
#pragma omp atomic
        bad++;
      }
    }
    for (int i = 0; i < PER; i++) {
      if (phash_pi_put(h, me * PER + i, i) != 0) {
#pragma omp atomic
        bad++;
      }
    }
  }
  CHECK(bad == 0);
  CHECK(phash_pi_size(h) == N);
  CHECK(phash_blocks(h) == blocks);
}
for (int k = 0; k < N; k++) {
  int v = -1;
  CHECK(phash_pi_get(h, k, &v) == 1 && v == k % PER);
}
phash_pi_destroy(&h);

END_TEST
