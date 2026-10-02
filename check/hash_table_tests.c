#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define hkey_t int
#define value_t int
#define prefix hi
#include <hash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

#define hkey_t char*
#define value_t int
#define prefix hs
#include <hash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

#define hkey_t const char*
#define value_t int
#define prefix hc
#include <hash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

#define hkey_t double
#define value_t int
#define prefix hd
#include <hash_table.h>
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

static int times_two(int key, int value) {
  return 2 * value;
}

static int add_arg(int key, int value, void *arg) {
  return value + *(int *) arg;
}

#define hkey_t int
#define value_t int
#define prefix dn
#include <hash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

START_TEST(hash_table_test1)

htable_hi_t *h = hash_hi_init(0);
CHECK(h != NULL);
CHECK(hash_hi_size(h) == 0);
CHECK(hash_hi_capacity(h) == 16);

hash_hi_destroy(&h);
CHECK(h == NULL);

hash_hi_destroy(&h);
CHECK(h == NULL);

END_TEST

START_TEST(hash_table_test2)

htable_hi_t *h = hash_hi_init(0);
CHECK(h != NULL);

int v = -1;
CHECK(hash_hi_get(h, 7, &v) == 0);
CHECK(hash_hi_put(h, 7, 70) == 0);
CHECK(hash_hi_get(h, 7, &v) == 1);
CHECK(v == 70);
CHECK(hash_hi_get(h, 7, NULL) == 1);
CHECK(hash_hi_size(h) == 1);

CHECK(hash_hi_put(h, 7, 71) == 0);
CHECK(hash_hi_get(h, 7, &v) == 1);
CHECK(v == 71);
CHECK(hash_hi_size(h) == 1);

CHECK(hash_hi_remove(h, 8, &v) == 0);
CHECK(hash_hi_remove(h, 7, &v) == 1);
CHECK(v == 71);
CHECK(hash_hi_remove(h, 7, &v) == 0);
CHECK(hash_hi_get(h, 7, &v) == 0);
CHECK(hash_hi_size(h) == 0);

CHECK(hash_hi_put(NULL, 1, 1) == -1);
CHECK(hash_hi_get(NULL, 1, &v) == -1);
CHECK(hash_hi_remove(NULL, 1, &v) == -1);

hash_hi_destroy(&h);
CHECK(h == NULL);

END_TEST

START_TEST(hash_table_test3)

htable_hi_t *h = hash_hi_init(0);
hash_hi_set_update(h, add);

for (int i = 0; i < 5; i++) {
  hash_hi_put(h, 3, 10);
}

int v;
CHECK(hash_hi_get(h, 3, &v) == 1);
CHECK(v == 50);
CHECK(hash_hi_size(h) == 1);

hash_hi_destroy(&h);

END_TEST

START_TEST(hash_table_test4)

int n = 10000;
htable_hi_t *h = hash_hi_init(0);

for (int i = 0; i < n; i++) {
  CHECK(hash_hi_put(h, i * 3, i) == 0);
}

CHECK(hash_hi_size(h) == (size_t) n);

int64_t cap = hash_hi_capacity(h);
CHECK((cap & (cap - 1)) == 0);
CHECK(cap * 3 >= (int64_t) n * 4);

for (int i = 0; i < n; i++) {
  int v;
  CHECK(hash_hi_get(h, i * 3, &v) == 1);
  CHECK(v == i);
  CHECK(hash_hi_get(h, i * 3 + 1, &v) == 0);
}

hash_hi_destroy(&h);

END_TEST

// Removing then re-inserting must keep the size exact.
START_TEST(hash_table_test5)

htable_hi_t *h = hash_hi_init(0);

hash_hi_put(h, 5, 1);
hash_hi_remove(h, 5, NULL);
hash_hi_put(h, 5, 2);
CHECK(hash_hi_size(h) == 1);

hash_hi_put(h, 6, 1);
hash_hi_put(h, 7, 1);
hash_hi_remove(h, 6, NULL);
hash_hi_put(h, 6, 1);
CHECK(hash_hi_size(h) == 3);

for (int round = 0; round < 20; round++) {
  for (int i = 0; i < 12; i++) {
    hash_hi_put(h, 100 + i, i);
  }
  for (int i = 0; i < 12; i++) {
    hash_hi_remove(h, 100 + i, NULL);
  }
  CHECK(hash_hi_size(h) == 3);
}

hash_hi_destroy(&h);

END_TEST

// Lots of insert/remove churn leaves many deleted markers; lookups of
// absent keys must still terminate.  The alarm turns a hang into a failure.
START_TEST(hash_table_test6)

alarm(60);

htable_hi_t *h = hash_hi_init(0);
for (int i = 0; i < 200000; i++) {
  hash_hi_put(h, i * 7919, i);
  hash_hi_remove(h, i * 7919, NULL);
}

CHECK(hash_hi_size(h) == 0);
CHECK(hash_hi_get(h, -1, NULL) == 0);
CHECK(hash_hi_remove(h, -1, NULL) == 0);
CHECK(hash_hi_put(h, -1, 1) == 0);
CHECK(hash_hi_get(h, -1, NULL) == 1);

hash_hi_destroy(&h);

alarm(0);

END_TEST

// Removing an absent key must not confuse a deleted marker with a real entry,
// including for key 0.
START_TEST(hash_table_test7)

htable_hi_t *h = hash_hi_init(0);

// a deleted marker sits exactly where key 0 starts probing
int k0 = key_at_slot0();
CHECK(hash_hi_put(h, k0, 1) == 0);
CHECK(hash_hi_remove(h, k0, NULL) == 1);
CHECK(hash_hi_remove(h, 0, NULL) == 0);
CHECK(hash_hi_get(h, 0, NULL) == 0);
CHECK(hash_hi_size(h) == 0);

for (int i = 1; i <= 12; i++) {
  hash_hi_put(h, i * 16, i);
}
for (int i = 1; i <= 12; i++) {
  CHECK(hash_hi_remove(h, i * 16, NULL) == 1);
}

for (int k = 0; k < 200; k++) {
  if (k % 16 != 0 || k == 0) {
    CHECK(hash_hi_remove(h, k, NULL) == 0);
  }
}
CHECK(hash_hi_size(h) == 0);

CHECK(hash_hi_put(h, 0, 5) == 0);
int v = -1;
CHECK(hash_hi_remove(h, 0, &v) == 1);
CHECK(v == 5);
CHECK(hash_hi_size(h) == 0);

hash_hi_destroy(&h);

END_TEST

START_TEST(hash_table_test8)

htable_hs_t *h = hash_hs_init(0);
char *names[] = {"a", "b", "c", "d", "e", "f"};
for (int i = 0; i < 6; i++) {
  hash_hs_put(h, names[i], i);
}
hash_hs_remove(h, "a", NULL);
hash_hs_remove(h, "c", NULL);

htable_hs_t *c = hash_hs_clone(h);
CHECK(c != NULL);
CHECK(hash_hs_size(c) == 4);

for (int i = 0; i < 6; i++) {
  int in_h = hash_hs_get(h, names[i], NULL);
  int in_c = hash_hs_get(c, names[i], NULL);
  CHECK(in_h == in_c);
}

int v;
CHECK(hash_hs_get(c, "b", &v) == 1);
CHECK(v == 1);

hash_hs_put(c, "z", 99);
CHECK(hash_hs_get(h, "z", NULL) == 0);
CHECK(hash_hs_size(h) == 4);
CHECK(hash_hs_size(c) == 5);

hash_hs_destroy(&c);
hash_hs_destroy(&h);

END_TEST

// Keys that are equal but stored at different addresses.
START_TEST(hash_table_test9)

htable_hs_t *h = hash_hs_init(0);
htable_hc_t *g = hash_hc_init(0);

char a1[] = "hello";
char a2[] = "hello";
char b[] = "world";

hash_hs_put(h, a1, 1);
hash_hs_put(h, b, 2);
int v = 0;
CHECK(hash_hs_get(h, a2, &v) == 1);
CHECK(v == 1);
hash_hs_put(h, a2, 3);
CHECK(hash_hs_size(h) == 2);

hash_hc_put(g, a1, 1);
hash_hc_put(g, b, 2);
v = 0;
CHECK(hash_hc_get(g, a2, &v) == 1);
CHECK(v == 1);
hash_hc_put(g, a2, 3);
CHECK(hash_hc_size(g) == 2);
CHECK(hash_hc_get(g, "absent", NULL) == 0);
CHECK(g->comp != NULL);

hash_hs_destroy(&h);
hash_hc_destroy(&g);

END_TEST

START_TEST(hash_table_test10)

htable_hd_t *h = hash_hd_init(0);

hash_hd_put(h, 0.0, 1);
hash_hd_put(h, -0.0, 2);
CHECK(hash_hd_size(h) == 1);
int v = 0;
CHECK(hash_hd_get(h, 0.0, &v) == 1);
CHECK(v == 2);
CHECK(hash_hd_get(h, -0.0, &v) == 1);

CHECK(hash_double(NAN) == hash_double(-NAN));
CHECK(hash_float(0.0f) == hash_float(-0.0f));
CHECK(hash_double(1.5) != hash_double(2.5));

hash_hd_destroy(&h);

END_TEST

// Integer valued doubles have no low mantissa bits; without mixing the hash
// table degenerates (17 seconds for this many keys).  A working table needs a
// few hundredths of a second, so the alarm turns the slowdown into a failure.
START_TEST(hash_table_test11)

alarm(4);

int n = 200000;
htable_hd_t *h = hash_hd_init(0);
for (int i = 0; i < n; i++) {
  CHECK(hash_hd_put(h, (double) i, i) == 0);
}
for (int i = 0; i < n; i++) {
  int v;
  CHECK(hash_hd_get(h, (double) i, &v) == 1);
  CHECK(v == i);
}
CHECK(hash_hd_size(h) == (size_t) n);

hash_hd_destroy(&h);

alarm(0);

END_TEST

START_TEST(hash_table_test12)

htable_hi_t *h = hash_hi_init(0);
int n = 500;
for (int i = 0; i < n; i++) {
  hash_hi_put(h, i, i * 2);
}
for (int i = 0; i < n; i += 5) {
  hash_hi_remove(h, i, NULL);
}

int expected = n - n / 5;
int count = 0;
int key_sum = 0;
int key, value;
hiter_hi_t *iter = NULL;
int rc = hash_hi_first(h, &iter, &key, &value);
while (rc == 0) {
  CHECK(value == 2 * key);
  CHECK(key % 5 != 0);
  key_sum += key;
  count++;
  rc = hash_hi_next(&iter, &key, &value);
}
CHECK(iter == NULL);
CHECK(count == expected);

int expected_sum = 0;
for (int i = 0; i < n; i++) {
  if (i % 5 != 0) {
    expected_sum += i;
  }
}
CHECK(key_sum == expected_sum);

htable_hi_t *empty = hash_hi_init(0);
iter = NULL;
CHECK(hash_hi_first(empty, &iter, &key, &value) == 1);
CHECK(iter == NULL);

hash_hi_destroy(&empty);
hash_hi_destroy(&h);

END_TEST

START_TEST(hash_table_test13)

htable_hi_t *h = hash_hi_init(0);
for (int i = 0; i < 100; i++) {
  hash_hi_put(h, i, i);
}

hash_hi_apply(h, times_two);
int extra = 7;
hash_hi_apply_r(h, add_arg, &extra);

for (int i = 0; i < 100; i++) {
  int v;
  CHECK(hash_hi_get(h, i, &v) == 1);
  CHECK(v == 2 * i + 7);
}

hash_hi_destroy(&h);

END_TEST

// Random puts, removes and gets against a plain array model.
START_TEST(hash_table_test14)

seed_rand(14);

for (int round = 0; round < 20; round++) {
  htable_hi_t *h = hash_hi_init(get_rand() % 200);
  char present[1000] = {0};
  int model[1000];
  int count = 0;

  for (int step = 0; step < 5000; step++) {
    int k = get_rand() % 1000;
    int op = get_rand() % 4;

    if (op <= 1) {
      int v = get_rand() % 100000;
      CHECK(hash_hi_put(h, k, v) == 0);
      if (!present[k]) {
        present[k] = 1;
        count++;
      }
      model[k] = v;
    } else if (op == 2) {
      int v = -1;
      int rc = hash_hi_remove(h, k, &v);
      CHECK(rc == present[k]);
      if (present[k]) {
        CHECK(v == model[k]);
        present[k] = 0;
        count--;
      }
    } else {
      int v = -1;
      int rc = hash_hi_get(h, k, &v);
      CHECK(rc == present[k]);
      if (present[k]) {
        CHECK(v == model[k]);
      }
    }
    CHECK(hash_hi_size(h) == (size_t) count);
  }

  htable_hi_t *c = hash_hi_clone(h);
  CHECK(c != NULL);
  CHECK(hash_hi_size(c) == (size_t) count);
  for (int k = 0; k < 1000; k++) {
    int v = -1;
    CHECK(hash_hi_get(c, k, &v) == present[k]);
    if (present[k]) {
      CHECK(v == model[k]);
    }
  }

  hash_hi_destroy(&c);
  hash_hi_destroy(&h);
}

END_TEST

// destroy accepts a NULL pointer and an already destroyed container
START_TEST(hash_table_test15)

htable_dn_t *h = NULL;
hash_dn_destroy(NULL);
hash_dn_destroy(&h);
CHECK(h == NULL);
h = hash_dn_init(0);
hash_dn_destroy(&h);
hash_dn_destroy(&h);
CHECK(h == NULL);

END_TEST
