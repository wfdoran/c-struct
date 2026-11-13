#include <check.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// int keys and values, with the default hash
#define hkey_t int
#define value_t int
#define prefix li
#include <lru_cache.h>
#undef prefix
#undef value_t
#undef hkey_t

// long keys, used with a hash which puts every key in one of 4 classes, so
// that the chains are long and the compare function is needed
#define hkey_t long
#define value_t int
#define prefix ll
#include <lru_cache.h>
#undef prefix
#undef value_t
#undef hkey_t

// keys and values which the cache owns
#define hkey_t char *
#define value_t char *
#define prefix ls
#include <lru_cache.h>
#undef prefix
#undef value_t
#undef hkey_t

// a key type which hash.h does not know: no default hash
typedef struct {
  int a;
  int b;
} pair_t;

#define hkey_t pair_t
#define value_t int
#define prefix lp
#include <lru_cache.h>
#undef prefix
#undef value_t
#undef hkey_t

// the internal macros of the header do not leak into the includer
#ifdef CS_KEYEQ
#error "lru_cache.h leaked CS_KEYEQ"
#endif
#ifdef CS_HASH
#error "lru_cache.h leaked CS_HASH"
#endif
#ifdef GLUE
#error "lru_cache.h leaked GLUE"
#endif

static uint64_t hash_class(long k) {
  return (uint64_t) (k % 4);
}
static int comp_l(long a, long b) {
  return a != b;
}

// records the calls of the evict function
static int evict_count = 0;
static long evict_last = -1;
static void evict_l(long k, int v) {
  (void) v;
  evict_count++;
  evict_last = k;
}
static void evict_i(int k, int v) {
  (void) v;
  evict_count++;
  evict_last = k;
}

static int sum_i(int previous, int current) {
  return previous + current;
}

// A model test: the cache against a list of the keys, most recently used
// first.  After every operation the recency list and the hash chains of the
// cache are checked against the model.
static int lru_model(int64_t capacity, long keys, int iters, uint32_t seed) {
  lru_ll_t *c = lru_ll_init(capacity);
  CHECK(c != NULL);
  CHECK(lru_ll_set_hash(c, hash_class) == 0);
  CHECK(lru_ll_set_comp(c, comp_l) == 0);
  CHECK(lru_ll_set_evict(c, evict_l) == 0);

  long *order = malloc((size_t) capacity * sizeof(long));
  int *values = malloc((size_t) capacity * sizeof(int));
  CHECK(order != NULL && values != NULL);
  int64_t n = 0;

  seed_rand(seed);
  for (int it = 0; it < iters; it++) {
    long k = (long) (get_rand() % (uint32_t) keys);
    int v = (int) (get_rand() % 1000);
    int64_t pos = -1;
    for (int64_t j = 0; j < n; j++) {
      if (order[j] == k) {
        pos = j;
      }
    }

    uint32_t op = get_rand() % 3;
    if (op == 2) {
      // peek: found like get, but the order stays as it is
      int out = -1;
      CHECK(lru_ll_peek(c, k, &out) == (pos >= 0 ? 1 : 0));
      if (pos >= 0) {
        CHECK(out == values[pos]);
      }
      pos = -1;
    } else if (op == 0) {
      evict_count = 0;
      CHECK(lru_ll_put(c, k, v) == (pos >= 0 ? 1 : 0));
      if (pos >= 0) {
        CHECK(evict_count == 0);
      } else if (n < capacity) {
        CHECK(evict_count == 0);
        pos = n++;
      } else {
        CHECK(evict_count == 1 && evict_last == order[capacity - 1]);
        pos = capacity - 1;
      }
    } else {
      int out = -1;
      bool want = get_rand() % 4 != 0; // a NULL value also counts as a use
      CHECK(lru_ll_get(c, k, want ? &out : NULL) == (pos >= 0 ? 1 : 0));
      if (pos >= 0) {
        if (want) {
          CHECK(out == values[pos]);
        }
        v = values[pos];
      }
    }
    // a use moves the key to the front of the model
    if (pos >= 0) {
      for (int64_t j = pos; j > 0; j--) {
        order[j] = order[j - 1];
        values[j] = values[j - 1];
      }
      order[0] = k;
      values[0] = v;
    }
    CHECK(lru_ll_size(c) == (size_t) n);

    // the recency list, in both directions
    int64_t j = 0;
    lru_node_ll_t *prev = NULL;
    for (lru_node_ll_t *x = c->head; x != NULL; prev = x, x = x->next, j++) {
      CHECK(j < n);
      CHECK(x->prev == prev);
      CHECK(x->key == order[j] && x->value == values[j]);
    }
    CHECK(j == n && c->tail == prev);

    // every node is on exactly one chain, with the right back links
    int64_t chained = 0;
    for (int64_t b = 0; b < (INT64_C(1) << c->log2_hash_size); b++) {
      lru_node_ll_t *hp = NULL;
      for (lru_node_ll_t *x = c->htable[b]; x != NULL; hp = x, x = x->hnext) {
        CHECK(x->hprev == hp);
        chained++;
      }
    }
    CHECK(chained == n);
  }

  // destroy evicts every entry left
  evict_count = 0;
  lru_ll_destroy(&c);
  CHECK(c == NULL);
  CHECK(evict_count == n);
  free(order);
  free(values);
  return 0;
}

START_TEST(lru_test1)
// the basic operations and the eviction order
lru_li_t *c = lru_li_init(3);
CHECK(c != NULL);
CHECK(lru_li_set_evict(c, evict_i) == 0);
evict_count = 0;
CHECK(lru_li_put(c, 1, 10) == 0);
CHECK(lru_li_put(c, 2, 20) == 0);
CHECK(lru_li_put(c, 3, 30) == 0);
CHECK(evict_count == 0);

int v = -1;
CHECK(lru_li_get(c, 1, &v) == 1 && v == 10); // order: 1 3 2
CHECK(lru_li_get(c, 4, &v) == 0);

CHECK(lru_li_put(c, 4, 40) == 0); // full: 2 is the least recently used
CHECK(evict_count == 1 && evict_last == 2);
CHECK(lru_li_get(c, 2, &v) == 0);

CHECK(lru_li_get(c, 3, NULL) == 1); // a use, even with no value: order 3 4 1
CHECK(lru_li_put(c, 5, 50) == 0);
CHECK(evict_count == 2 && evict_last == 1);

// put of a key already present replaces the value, and is a use
CHECK(lru_li_put(c, 4, 41) == 1); // order: 4 5 3
CHECK(evict_count == 2);
CHECK(lru_li_put(c, 6, 60) == 0);
CHECK(evict_count == 3 && evict_last == 3);
CHECK(lru_li_get(c, 4, &v) == 1 && v == 41);
CHECK(lru_li_get(c, 5, &v) == 1 && v == 50);
CHECK(lru_li_get(c, 6, &v) == 1 && v == 60);

evict_count = 0;
lru_li_destroy(&c);
CHECK(c == NULL);
CHECK(evict_count == 3);
lru_li_destroy(&c); // a second destroy does nothing
END_TEST

START_TEST(lru_test2)
// a cache of one entry
lru_li_t *c = lru_li_init(1);
CHECK(c != NULL);
int v = -1;
for (int i = 0; i < 10; i++) {
  CHECK(lru_li_put(c, i, i * i) == 0);
  CHECK(lru_li_get(c, i, &v) == 1 && v == i * i);
  if (i > 0) {
    CHECK(lru_li_get(c, i - 1, &v) == 0);
  }
}
CHECK(lru_li_put(c, 9, 0) == 1);
CHECK(lru_li_get(c, 9, &v) == 1 && v == 0);
lru_li_destroy(&c);

// an update function combines the values of a repeated key
c = lru_li_init(4);
CHECK(c != NULL);
CHECK(lru_li_set_update(c, sum_i) == 0);
CHECK(lru_li_put(c, 5, 1) == 0);
CHECK(lru_li_put(c, 5, 2) == 1);
CHECK(lru_li_put(c, 5, 4) == 1);
CHECK(lru_li_get(c, 5, &v) == 1 && v == 7);
lru_li_destroy(&c);
END_TEST

START_TEST(lru_test3)
// the model test, for small capacities with few and many keys, and a larger one
for (int64_t capacity = 1; capacity <= 9; capacity++) {
  for (long keys = 1; keys <= 3 * capacity + 2; keys += 3) {
    CHECK(lru_model(capacity, keys, 5000, (uint32_t) (capacity * 100 + keys)) == 0);
  }
}
CHECK(lru_model(64, 100, 100000, 7) == 0);
END_TEST

// Keys and values owned by the cache.  The counts check that every string is
// freed exactly once; ASan checks that none is used after it is freed.
static int ls_frees = 0;
static int ls_clone_fail = 0;
static char *ls_evicted = NULL; // a copy of the last key evicted

static void ls_evict(char *key, char *value) {
  free(ls_evicted);
  ls_evicted = strdup(key);
  free(key);
  free(value);
  ls_frees += 2;
}
static int ls_clone(char *stored, char **out) {
  if (ls_clone_fail) {
    return -1;
  }
  *out = strdup(stored);
  return *out == NULL ? -1 : 0;
}
static char *ls_update(char *previous, char *current) {
  if (previous != current) {
    free(previous);
    ls_frees++;
  }
  return current;
}

START_TEST(lru_test4)
lru_ls_t *c = lru_ls_init(2);
CHECK(c != NULL);
CHECK(lru_ls_set_evict(c, ls_evict) == 0);
CHECK(lru_ls_set_clone(c, ls_clone) == 0);
CHECK(lru_ls_set_update(c, ls_update) == 0);
ls_frees = 0;

CHECK(lru_ls_put(c, strdup("one"), strdup("1")) == 0);
CHECK(lru_ls_put(c, strdup("two"), strdup("2")) == 0); // order: two one

// get gives a copy of its own, which outlives the entry
char *mine = NULL;
CHECK(lru_ls_get(c, "one", &mine) == 1); // order: one two
CHECK(mine != NULL && strcmp(mine, "1") == 0);

// a repeated key: the old value goes to update, the key passed in is not stored
char *again = strdup("one");
CHECK(lru_ls_put(c, again, strdup("uno")) == 1);
CHECK(ls_frees == 1);
free(again);

// the stored pointer put again is not freed ("one" is the most recent entry)
again = strdup("one");
CHECK(strcmp(c->head->key, "one") == 0);
CHECK(lru_ls_put(c, again, c->head->value) == 1);
CHECK(ls_frees == 1);
free(again);

CHECK(lru_ls_put(c, strdup("three"), strdup("3")) == 0); // evicts two
CHECK(ls_frees == 3 && strcmp(ls_evicted, "two") == 0);
CHECK(lru_ls_put(c, strdup("four"), strdup("4")) == 0); // evicts one
CHECK(ls_frees == 5 && strcmp(ls_evicted, "one") == 0);
CHECK(strcmp(mine, "1") == 0);
free(mine);

// a clone which fails: get reports an error and does not change the order
char *got = NULL;
CHECK(lru_ls_get(c, "three", &got) == 1); // order: three four
free(got);
ls_clone_fail = 1;
CHECK(lru_ls_get(c, "four", &got) == -1);
ls_clone_fail = 0;
CHECK(lru_ls_put(c, strdup("five"), strdup("5")) == 0); // still evicts four
CHECK(strcmp(ls_evicted, "four") == 0);

// default comparison of char * keys is strcmp: another pointer, the same key
char key[] = "five";
CHECK(lru_ls_get(c, key, NULL) == 1);

ls_frees = 0;
lru_ls_destroy(&c);
CHECK(ls_frees == 4);
free(ls_evicted);
ls_evicted = NULL;
END_TEST

static uint64_t hash_pair(pair_t p) {
  return ((uint64_t) (uint32_t) p.a << 32) | (uint32_t) p.b;
}
static int comp_pair(pair_t x, pair_t y) {
  return x.a != y.a || x.b != y.b;
}

START_TEST(lru_test5)
// the errors
CHECK(lru_li_init(0) == NULL);
CHECK(lru_li_init(-1) == NULL);
CHECK(lru_li_init(INT64_MAX) == NULL);

int v = 0;
CHECK(lru_li_put(NULL, 1, 1) == -1);
CHECK(lru_li_get(NULL, 1, &v) == -1);
CHECK(lru_li_set_hash(NULL, NULL) == -1);
CHECK(lru_li_set_comp(NULL, NULL) == -1);
CHECK(lru_li_set_update(NULL, NULL) == -1);
CHECK(lru_li_set_clone(NULL, NULL) == -1);
CHECK(lru_li_set_evict(NULL, NULL) == -1);
lru_li_destroy(NULL);
lru_li_t *none = NULL;
lru_li_destroy(&none);

// a key type with no default hash: put and get fail until one is set
lru_lp_t *c = lru_lp_init(8);
CHECK(c != NULL);
pair_t p = {.a = 1, .b = 2};
CHECK(lru_lp_put(c, p, 3) == -1);
CHECK(lru_lp_get(c, p, &v) == -1);
CHECK(lru_lp_set_hash(c, hash_pair) == 0);
CHECK(lru_lp_set_comp(c, comp_pair) == 0);
CHECK(lru_lp_put(c, p, 3) == 0);
CHECK(lru_lp_get(c, p, &v) == 1 && v == 3);

// the hash can not be changed once the cache has entries
CHECK(lru_lp_set_hash(c, NULL) == -1);
CHECK(lru_lp_get(c, p, &v) == 1);
lru_lp_destroy(&c);
END_TEST

START_TEST(lru_test6)
// size, capacity and peek
lru_li_t *c = lru_li_init(3);
CHECK(c != NULL);
CHECK(lru_li_capacity(c) == 3);
CHECK(lru_li_size(c) == 0);
CHECK(lru_li_set_evict(c, evict_i) == 0);
for (int i = 1; i <= 3; i++) {
  CHECK(lru_li_put(c, i, 10 * i) == 0);
  CHECK(lru_li_size(c) == (size_t) i);
}
CHECK(lru_li_put(c, 3, 31) == 1);
CHECK(lru_li_size(c) == 3); // a repeated key adds nothing

int v = -1;
CHECK(lru_li_peek(c, 1, &v) == 1 && v == 10); // order still: 3 2 1
CHECK(lru_li_peek(c, 1, NULL) == 1);
CHECK(lru_li_peek(c, 9, &v) == 0);
evict_count = 0;
CHECK(lru_li_put(c, 4, 40) == 0); // 1 is still the least recently used
CHECK(evict_count == 1 && evict_last == 1);
CHECK(lru_li_size(c) == 3 && lru_li_capacity(c) == 3);

// clear evicts every entry, most recent first, and the cache is usable again
evict_count = 0;
CHECK(lru_li_clear(c) == 0);
CHECK(evict_count == 3 && evict_last == 2);
CHECK(lru_li_size(c) == 0 && lru_li_capacity(c) == 3);
for (int i = 1; i <= 4; i++) {
  CHECK(lru_li_get(c, i, &v) == 0);
  CHECK(lru_li_peek(c, i, &v) == 0);
}
CHECK(lru_li_clear(c) == 0); // an empty cache
CHECK(evict_count == 3);
for (int i = 0; i < 10; i++) {
  CHECK(lru_li_put(c, i, i) == 0);
}
CHECK(lru_li_size(c) == 3);
CHECK(lru_li_get(c, 9, &v) == 1 && v == 9);
CHECK(lru_li_get(c, 6, &v) == 0);

// the hash can be changed again once the cache is cleared
CHECK(lru_li_set_hash(c, NULL) == -1);
CHECK(lru_li_clear(c) == 0);
CHECK(lru_li_set_hash(c, c->hash_func) == 0);
lru_li_destroy(&c);

// peek uses clone, and a failed clone is an error
lru_ls_t *s = lru_ls_init(2);
CHECK(s != NULL);
CHECK(lru_ls_set_evict(s, ls_evict) == 0);
CHECK(lru_ls_set_clone(s, ls_clone) == 0);
CHECK(lru_ls_put(s, strdup("a"), strdup("A")) == 0);
char *mine = NULL;
CHECK(lru_ls_peek(s, "a", &mine) == 1);
CHECK(mine != NULL && mine != s->head->value && strcmp(mine, "A") == 0);
free(mine);
ls_clone_fail = 1;
CHECK(lru_ls_peek(s, "a", &mine) == -1);
ls_clone_fail = 0;

// clear hands the owned keys and values to evict
ls_frees = 0;
CHECK(lru_ls_clear(s) == 0);
CHECK(ls_frees == 2 && lru_ls_size(s) == 0);
lru_ls_destroy(&s);
CHECK(ls_frees == 2);
free(ls_evicted);
ls_evicted = NULL;
END_TEST
