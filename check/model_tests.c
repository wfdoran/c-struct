#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// Model-based tests: a long run of random operations is applied to a container and
// to a plain model of it (an array, or a table of flags), and the two are compared
// after every operation, with a full check of the structure every so often.  The
// seeds are fixed, so a run is the same on every machine.  These find what the
// hand-written tests do not think of: the tree invariants after thousands of
// deletes, the links of a list after every kind of edit, the tombstone accounting
// of the hash tables, and the front of an array used as a queue.

#define data_t int
#define prefix mtree
#include <tree.h>
#undef prefix
#undef data_t

// the same tree with the ordering compiled in (data_less)
#define data_t int
#define prefix mtreel
#define data_less(x, y) ((x) < (y))
#include <tree.h>
#undef prefix
#undef data_t

#define data_t int
#define prefix mll
#define null_value -1
#include <linked_list.h>
#undef prefix
#undef data_t

#define hkey_t int
#define value_t int
#define prefix mh
#include <hash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

#define hkey_t int
#define value_t int
#define prefix mp
#include <phash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

#define data_t int
#define prefix ma
#include <array.h>
#undef prefix
#undef data_t

#define data_t int
#define prefix mq
#include <pqueue.h>
#undef prefix
#undef data_t

// ---------------------------------------------------------------------------
// tree

// Checks one subtree: the order of the keys (all in (lo, hi)), the parent links,
// the balance, and the height and size stored in every node.  Returns 0 if all is
// well; *count, *height and *size describe the subtree.
#define MODEL_TREE_VERIFY(P, NODE)                                                  \
  static int verify_##P(NODE *n, NODE *parent, int lo, int hi, size_t *count,      \
                        int *height, size_t *size) {                               \
    if (n == NULL) {                                                               \
      *height = 0;                                                                 \
      *size = 0;                                                                   \
      return 0;                                                                    \
    }                                                                              \
    CHECK(n->parent == parent);                                                    \
    CHECK(n->key > lo && n->key < hi);                                             \
    (*count)++;                                                                    \
    int hl, hr;                                                                    \
    size_t sl, sr;                                                                 \
    if (verify_##P(n->left, n, lo, n->key, count, &hl, &sl) != 0) {                \
      return 1;                                                                    \
    }                                                                              \
    if (verify_##P(n->right, n, n->key, hi, count, &hr, &sr) != 0) {               \
      return 1;                                                                    \
    }                                                                              \
    CHECK(hl - hr <= 1 && hr - hl <= 1);                                           \
    *height = 1 + (hl > hr ? hl : hr);                                             \
    *size = 1 + sl + sr;                                                           \
    CHECK(n->height == *height);                                                   \
    CHECK(n->size == *size);                                                       \
    return 0;                                                                      \
  }

MODEL_TREE_VERIFY(mtree, tnode_mtree_t)
MODEL_TREE_VERIFY(mtreel, tnode_mtreel_t)

#define TREE_KEYS 2000

static int run_tree_model(uint32_t seed) {
  seed_rand(seed);
  tree_mtree_t *ta = tree_mtree_init();
  tree_mtreel_t *tb = tree_mtreel_init();
  CHECK(ta != NULL && tb != NULL);

  static char present[TREE_KEYS];
  memset(present, 0, sizeof(present));
  size_t n = 0;

  for (int i = 0; i < 100000; i++) {
    int k = (int) (get_rand() % TREE_KEYS);
    uint32_t op = get_rand() % 8;

    if (op < 3) {
      CHECK(tree_mtree_insert(ta, k, NULL) == 0);
      CHECK(tree_mtreel_insert(tb, k, NULL) == 0);
      if (!present[k]) {
        present[k] = 1;
        n++;
      }
    } else if (op < 5) {
      bool fa = tree_mtree_delete(ta, k).found;
      bool fb = tree_mtreel_delete(tb, k).found;
      CHECK(fa == (bool) present[k] && fb == (bool) present[k]);
      if (present[k]) {
        present[k] = 0;
        n--;
      }
    } else if (op == 5) {
      CHECK(tree_mtree_retrieve(ta, k).found == (bool) present[k]);
      CHECK(tree_mtreel_retrieve(tb, k).found == (bool) present[k]);
    } else if (op == 6) {
      // delete the smallest or the largest key
      int want = -1;
      bool smallest = (get_rand() & 1) != 0;
      for (int j = 0; j < TREE_KEYS; j++) {
        int key = smallest ? j : TREE_KEYS - 1 - j;
        if (present[key]) {
          want = key;
          break;
        }
      }
      key_mtree_value_t ra = smallest ? tree_mtree_delete_min(ta) : tree_mtree_delete_max(ta);
      key_mtreel_value_t rb = smallest ? tree_mtreel_delete_min(tb) : tree_mtreel_delete_max(tb);
      CHECK(ra.found == (want >= 0) && rb.found == (want >= 0));
      if (want >= 0) {
        CHECK(ra.key == want && rb.key == want);
        present[want] = 0;
        n--;
      }
    } else {
      // the rank queries
      size_t less = 0, greater = 0;
      for (int j = 0; j < TREE_KEYS; j++) {
        if (present[j] && j < k) {
          less++;
        }
        if (present[j] && j > k) {
          greater++;
        }
      }
      size_t eq = present[k] ? 1 : 0;
      CHECK(tree_mtree_num_less(ta, k) == less && tree_mtreel_num_less(tb, k) == less);
      CHECK(tree_mtree_num_less_equal(ta, k) == less + eq);
      CHECK(tree_mtree_num_greater(ta, k) == greater);
      CHECK(tree_mtreel_num_greater_equal(tb, k) == greater + eq);
      if (present[k]) {
        key_mtree_value_t r = tree_mtree_get_rank(ta, less);
        CHECK(r.found && r.key == k);
      }
    }

    CHECK(tree_mtree_size(ta) == n && tree_mtreel_size(tb) == n);

    if (i % 2500 == 0) {
      size_t ca = 0, cb = 0, sa, sb;
      int ha, hb;
      CHECK(verify_mtree(ta->root, NULL, -1, TREE_KEYS, &ca, &ha, &sa) == 0);
      CHECK(verify_mtreel(tb->root, NULL, -1, TREE_KEYS, &cb, &hb, &sb) == 0);
      CHECK(ca == n && cb == n);

      // the walk visits exactly the keys in the model, in increasing order
      titer_mtree_t it;
      int key = -1, expect = 0;
      size_t visited = 0;
      for (int rc = tree_mtree_first(ta, &it, &key, NULL); rc == 0;
           rc = tree_mtree_next(&it, &key, NULL)) {
        while (expect < TREE_KEYS && !present[expect]) {
          expect++;
        }
        CHECK(key == expect);
        expect++;
        visited++;
      }
      CHECK(visited == n);
    }
  }

  tree_mtree_destroy(&ta);
  tree_mtreel_destroy(&tb);
  return 0;
}

START_TEST(model_test1)

CHECK(run_tree_model(1) == 0);
CHECK(run_tree_model(2) == 0);

END_TEST

// ---------------------------------------------------------------------------
// linked list

#define LIST_CAP 1000

static int cmp_int(const void *x, const void *y) {
  int a = *(const int *) x, b = *(const int *) y;
  return (a > b) - (a < b);
}

// The list against the model m[0..n): values, forward and backward links, head,
// tail and size.
static int verify_list(llist_mll_t *l, const int *m, size_t n) {
  CHECK(llist_mll_size(l) == n);
  lnode_mll_t *p = NULL, *prev = NULL;
  size_t i = 0;
  int value;
  for (int rc = llist_mll_first(l, &p, &value); rc == 0; rc = llist_mll_next(&p, &value)) {
    CHECK(i < n && value == m[i]);
    CHECK(p->prev == prev);
    prev = p;
    i++;
  }
  CHECK(i == n);
  CHECK(l->tail == prev);
  CHECK((n == 0) == (l->head == NULL));

  i = n;
  for (int rc = llist_mll_last(l, &p, &value); rc == 0; rc = llist_mll_prev(&p, &value)) {
    CHECK(i > 0 && value == m[i - 1]);
    i--;
  }
  CHECK(i == 0);
  return 0;
}

static int run_list_model(uint32_t seed) {
  seed_rand(seed);
  llist_mll_t *l = llist_mll_init();
  CHECK(l != NULL);
  int m[LIST_CAP + 2];
  size_t n = 0;

  for (int it = 0; it < 60000; it++) {
    uint32_t op = get_rand() % 10;
    int v = (int) (get_rand() % 1000);
    if (n >= LIST_CAP && op != 2 && op != 3) {
      op = 2 + (get_rand() & 1); // keep the list short enough for the O(n) edits
    }

    if (op == 0) {
      CHECK(llist_mll_add_start(l, v) == 0);
      memmove(m + 1, m, n * sizeof(int));
      m[0] = v;
      n++;
    } else if (op == 1) {
      CHECK(llist_mll_add_end(l, v) == 0);
      m[n++] = v;
    } else if (op == 2) {
      int r = llist_mll_remove_start(l);
      if (n == 0) {
        CHECK(r == -1);
      } else {
        CHECK(r == m[0]);
        memmove(m, m + 1, (n - 1) * sizeof(int));
        n--;
      }
    } else if (op == 3) {
      int r = llist_mll_remove_end(l);
      if (n == 0) {
        CHECK(r == -1);
      } else {
        CHECK(r == m[n - 1]);
        n--;
      }
    } else if (op == 9) {
      if (it % 500 == 0) {
        CHECK(llist_mll_msort(l) == 0);
        qsort(m, n, sizeof(int), cmp_int);
      } else {
        CHECK(llist_mll_add_end(l, v) == 0);
        m[n++] = v;
      }
    } else if (n > 0) {
      // the editing functions, at a random node
      size_t k = get_rand() % n;
      lnode_mll_t *p = NULL;
      CHECK(llist_mll_first(l, &p, NULL) == 0);
      for (size_t j = 0; j < k; j++) {
        CHECK(llist_mll_next(&p, NULL) == 0);
      }
      CHECK(p->data == m[k]);

      if (op == 4) {
        CHECK(llist_mll_insert_before(l, &p, v) == 0);
        memmove(m + k + 1, m + k, (n - k) * sizeof(int));
        m[k] = v;
        n++;
        CHECK(p->data == v); // the node pointer is at the new node
      } else if (op == 5) {
        CHECK(llist_mll_insert_after(l, &p, v) == 0);
        memmove(m + k + 2, m + k + 1, (n - k - 1) * sizeof(int));
        m[k + 1] = v;
        n++;
        CHECK(p->data == v);
      } else if (op == 6) {
        int r = llist_mll_remove_forward(l, &p);
        CHECK(r == m[k]);
        memmove(m + k, m + k + 1, (n - k - 1) * sizeof(int));
        n--;
        if (k < n) { // moved to the next node
          CHECK(p != NULL && p->data == m[k]);
        } else {
          CHECK(p == NULL);
        }
      } else { // op == 7 (op 8 is the same, to remove as often as it inserts)
        int r = llist_mll_remove_backwards(l, &p);
        CHECK(r == m[k]);
        memmove(m + k, m + k + 1, (n - k - 1) * sizeof(int));
        n--;
        if (k > 0) { // moved to the previous node
          CHECK(p != NULL && p->data == m[k - 1]);
        } else {
          CHECK(p == NULL);
        }
      }
    }

    CHECK(llist_mll_size(l) == n);
    if (it % 200 == 0 || n < 4) {
      CHECK(verify_list(l, m, n) == 0);
    }
  }

  CHECK(verify_list(l, m, n) == 0);
  llist_mll_destroy(&l);
  return 0;
}

START_TEST(model_test2)

CHECK(run_list_model(1) == 0);
CHECK(run_list_model(2) == 0);

END_TEST

// ---------------------------------------------------------------------------
// hash tables

#define HASH_KEYS 3000

static int add_values(int a, int b) {
  return a + b;
}

// hash_table and phash_table together, with the same operations on both.  With an
// update function a repeated key adds to the value instead of replacing it.
static int run_hash_model(uint32_t seed, bool use_update) {
  seed_rand(seed);
  htable_mh_t *h = hash_mh_init(seed == 1 ? 0 : 500);
  phtable_mp_t *p = phash_mp_init(0);
  CHECK(h != NULL && p != NULL);
  if (use_update) {
    hash_mh_set_update(h, add_values);
    phash_mp_set_update(p, add_values);
  }

  static int value[HASH_KEYS];
  static char present[HASH_KEYS];
  memset(value, 0, sizeof(value));
  memset(present, 0, sizeof(present));
  size_t n = 0;

  for (int it = 0; it < 150000; it++) {
    int k = (int) (get_rand() % HASH_KEYS);
    int v = (int) (get_rand() % 100);
    uint32_t op = get_rand() % 5;
    int gh = -1, gp = -1;

    if (op < 2) {
      CHECK(hash_mh_put(h, k, v) == 0);
      CHECK(phash_mp_put(p, k, v) == 0);
      value[k] = (present[k] && use_update) ? value[k] + v : v;
      if (!present[k]) {
        present[k] = 1;
        n++;
      }
    } else if (op == 2) {
      int rh = hash_mh_remove(h, k, &gh);
      int rp = phash_mp_remove(p, k, &gp);
      CHECK(rh == present[k] && rp == present[k]);
      if (present[k]) {
        CHECK(gh == value[k] && gp == value[k]);
        present[k] = 0;
        n--;
      }
    } else {
      int rh = hash_mh_get(h, k, &gh);
      int rp = phash_mp_get(p, k, &gp);
      CHECK(rh == present[k] && rp == present[k]);
      if (present[k]) {
        CHECK(gh == value[k] && gp == value[k]);
      }
    }

    CHECK(hash_mh_size(h) == n && phash_mp_size(p) == n);

    if (it % 10000 == 0) {
      // iteration visits each entry once; a clone holds the same entries
      size_t count = 0;
      hiter_mh_t iter;
      int key, val;
      for (int rc = hash_mh_first(h, &iter, &key, &val); rc == 0;
           rc = hash_mh_next(&iter, &key, &val)) {
        CHECK(key >= 0 && key < HASH_KEYS && present[key] && value[key] == val);
        count++;
      }
      CHECK(count == n);

      htable_mh_t *c = hash_mh_clone(h);
      CHECK(c != NULL && hash_mh_size(c) == n);
      for (int j = 0; j < HASH_KEYS; j++) {
        int got = -1;
        CHECK(hash_mh_get(c, j, &got) == present[j]);
        if (present[j]) {
          CHECK(got == value[j]);
        }
      }
      hash_mh_destroy(&c);
    }
  }

  hash_mh_destroy(&h);
  phash_mp_destroy(&p);
  return 0;
}

START_TEST(model_test3)

CHECK(run_hash_model(1, false) == 0);
CHECK(run_hash_model(2, true) == 0);
CHECK(run_hash_model(3, false) == 0);

END_TEST

// ---------------------------------------------------------------------------
// array, priority queue and heaps

#define ARRAY_CAP 3000
#define HEAP_KEYS 700

static int run_array_model(uint32_t seed) {
  seed_rand(seed);
  array_ma_t *a = array_ma_init();
  CHECK(a != NULL);
  int m[ARRAY_CAP + 2];
  size_t n = 0;

  for (int it = 0; it < 100000; it++) {
    uint32_t op = get_rand() % 9;
    int v = (int) (get_rand() % 500);
    if (n >= ARRAY_CAP) {
      op = 3 + (get_rand() & 1); // pop, or pop_first
    }

    if (op <= 2) {
      CHECK(array_ma_append(a, v) == 0);
      m[n++] = v;
    } else if (op == 3 && n > 0) {
      CHECK(array_ma_pop(a) == m[n - 1]);
      n--;
    } else if (op == 4 && n > 0) {
      CHECK(array_ma_pop_first(a) == m[0]);
      memmove(m, m + 1, (n - 1) * sizeof(int));
      n--;
    } else if (op == 5 && n > 0) {
      size_t i = get_rand() % n;
      CHECK(array_ma_set(a, v, i) == 0);
      m[i] = v;
    } else if (op == 6 && n > 1 && it % 40 == 0) {
      // sort, then every kind of binary search against a count
      CHECK(array_ma_sort(a) == 0);
      qsort(m, n, sizeof(int), cmp_int);
      for (int j = 0; j < 8; j++) {
        int x = (int) (get_rand() % 500);
        size_t less = 0, less_equal = 0;
        for (size_t i = 0; i < n; i++) {
          less += m[i] < x;
          less_equal += m[i] <= x;
        }
        CHECK((size_t) array_ma_bisect_lower(a, x) == less);
        CHECK(array_ma_bisect_upper(a, x) == (ssize_t) less_equal - 1);
        CHECK((array_ma_bisect(a, x) >= 0) == (less_equal > less));
      }
    } else if (op == 7 && n > 0 && it % 100 == 0) {
      size_t left = get_rand() % n;
      size_t right = left + get_rand() % (n - left + 1);
      array_ma_t *s = array_ma_slice(a, left, right);
      CHECK(s != NULL && array_ma_size(s) == right - left);
      CHECK(memcmp(s->data, m + left, (right - left) * sizeof(int)) == 0);
      array_ma_destroy(&s);
    } else if (op == 8 && it % 300 == 0) {
      array_ma_t *c = array_ma_clone(a);
      CHECK(c != NULL && array_ma_size(c) == n);
      CHECK(n == 0 || memcmp(c->data, m, n * sizeof(int)) == 0);
      array_ma_destroy(&c);
    }

    if (it % 250 == 0) {
      // room for more: whatever is in the array, in front or behind, stays
      size_t want = n + get_rand() % 200;
      CHECK(array_ma_reserve(a, want) == 0);
      CHECK(array_ma_capacity(a) >= want);
    }

    CHECK(array_ma_size(a) == n);
    if (n > 0) {
      CHECK(array_ma_get(a, 0) == m[0] && array_ma_get(a, n - 1) == m[n - 1]);
    }
    if (it % 1000 == 0) {
      CHECK(n == 0 || memcmp(a->data, m, n * sizeof(int)) == 0);
    }
  }

  array_ma_destroy(&a);
  return 0;
}

// The largest key which is still in a table of counts, or -1.
static int largest(const int *counts) {
  for (int k = HEAP_KEYS - 1; k >= 0; k--) {
    if (counts[k] > 0) {
      return k;
    }
  }
  return -1;
}

// The heap in an array (heappush, heappop, heapify) and the priority queue give the
// largest key first, like a table of counts.
static int run_heap_model(uint32_t seed) {
  seed_rand(seed);
  array_ma_t *h = array_ma_init();
  pqueue_mq_t *q = pqueue_mq_init();
  CHECK(h != NULL && q != NULL);
  int hc[HEAP_KEYS], qc[HEAP_KEYS];
  memset(hc, 0, sizeof(hc));
  memset(qc, 0, sizeof(qc));
  size_t hn = 0, qn = 0;

  for (int it = 0; it < 60000; it++) {
    uint32_t op = get_rand() % 5;
    int key = (int) (get_rand() % HEAP_KEYS);

    if (op < 3) {
      CHECK(array_ma_heappush(h, key) == 0);
      hc[key]++;
      hn++;
      CHECK(pqueue_mq_push(q, key, NULL) == 0);
      qc[key]++;
      qn++;
    } else if (op == 3) {
      if (hn > 0) {
        int want = largest(hc);
        CHECK(array_ma_heappop(h) == want);
        hc[want]--;
        hn--;
      }
      if (qn > 0) {
        int want = largest(qc);
        pqkv_mq_t top = pqueue_mq_peek(q);
        CHECK(top.found && top.key == want);
        pqkv_mq_t r = pqueue_mq_pop(q);
        CHECK(r.found && r.key == want);
        qc[want]--;
        qn--;
      }
    } else if (it % 500 == 0) {
      // append without keeping the order, then restore it
      for (int j = 0; j < 20; j++) {
        int x = (int) (get_rand() % HEAP_KEYS);
        CHECK(array_ma_append(h, x) == 0);
        hc[x]++;
        hn++;
      }
      CHECK(array_ma_heapify(h) == 0);
    }

    CHECK(array_ma_size(h) == hn && pqueue_mq_size(q) == qn);
    CHECK(pqueue_mq_is_empty(q) == (qn == 0));
  }

  // drain both: the keys come out in decreasing order
  int prev = HEAP_KEYS;
  while (array_ma_size(h) > 0) {
    int x = array_ma_heappop(h);
    CHECK(x <= prev);
    prev = x;
  }
  prev = HEAP_KEYS;
  while (!pqueue_mq_is_empty(q)) {
    pqkv_mq_t r = pqueue_mq_pop(q);
    CHECK(r.found && r.key <= prev);
    prev = r.key;
  }
  CHECK(!pqueue_mq_pop(q).found);

  array_ma_destroy(&h);
  pqueue_mq_destroy(&q);
  return 0;
}

START_TEST(model_test4)

CHECK(run_array_model(1) == 0);
CHECK(run_array_model(2) == 0);
CHECK(run_heap_model(1) == 0);
CHECK(run_heap_model(2) == 0);

END_TEST
