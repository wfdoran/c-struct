#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>


#define data_t int
#define prefix int
#include <array.h>
#undef data_t
#undef prefix

#define data_t char*
#define prefix str
#define default_null_value NULL
#include <array.h>
#undef default_null_value
#undef data_t
#undef prefix


typedef struct {
  int x;
  int y;
} pair_t;
    

#define data_t pair_t
#define prefix pair
#include <array.h>
#undef prefix
#undef data_t

// records with a key and a tag, compared by key only, to see whether a sort is stable
typedef struct {
  int k;
  int tag;
} rec_t;

#define data_t rec_t
#define prefix rec
#include <array.h>
#undef prefix
#undef data_t

static int comp_rec(rec_t *a, rec_t *b) {
  return (a->k > b->k) - (a->k < b->k);
}

// An instantiation whose allocations can be made to fail, for the out of memory paths.
static long aoom_budget = -1;   // allocations which may still succeed; -1 for no limit

static __attribute__((noinline)) void *aoom_malloc(size_t n) {
  if (aoom_budget == 0) {
    return NULL;
  }
  if (aoom_budget > 0) {
    aoom_budget--;
  }
  return malloc(n);
}

static __attribute__((noinline)) void *aoom_realloc(void *p, size_t n) {
  if (aoom_budget == 0) {
    return NULL;
  }
  if (aoom_budget > 0) {
    aoom_budget--;
  }
  return realloc(p, n);
}

#define malloc aoom_malloc
#define realloc aoom_realloc
#define data_t int
#define prefix oom
#include <array.h>
#undef prefix
#undef data_t
#undef realloc
#undef malloc

static int comp_pair(pair_t *a, pair_t *b) {
  if (a->x < b->x) {
    return -1;
  }
  if (a->x > b->x) {
    return 1;
  }
  if (a->y < b->y) {
    return -1;
  }
  if (a->y > b->y) {
    return 1;
  }
  return 0;
}


char* free_str(char *s) {
  free(s);
  return NULL;
}

START_TEST(array_test1)

array_int_t *a = array_int_init();
CHECK(a != NULL);
array_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(array_test2)

array_int_t *a = array_int_init();
CHECK(a != NULL);
array_int_append(a, 5);
array_int_append(a, 6);
CHECK(array_int_size(a) == 2);
CHECK(array_int_pop(a) == 6);
CHECK(array_int_pop(a) == 5);
CHECK(array_int_size(a) == 0);
array_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(array_test3)

int default_value = -1;
size_t size = 8;
array_int_t *a = array_int_init2(size, default_value);
CHECK(a != NULL);
CHECK(array_int_size(a) == size);
for (int i = 0; i < size; i++) {
  CHECK(array_int_get(a, i) == default_value);
}
array_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(array_test4)

array_int_t *a = array_int_init();
CHECK(a != NULL);
array_int_append(a, 1);
array_int_append(a, 2);
array_int_t *b = array_int_clone(a);
CHECK(b != NULL);
CHECK(array_int_size(a) == array_int_size(b));
for (int i = 0; i < array_int_size(a); i++) {
  CHECK(array_int_get(a, i) == array_int_get(b, i));
}
array_int_set(a, 5, 0);
CHECK(array_int_get(b, 0) == 1);
array_int_destroy(&a);
CHECK(a == NULL);
CHECK(b != NULL);
array_int_destroy(&b);
CHECK(b == NULL);

END_TEST


START_TEST(array_test5)

array_str_t *a = array_str_init();
CHECK(a != NULL);
char h[] = "Hello";
array_str_append(a, h);
array_str_t *b = array_str_clone(a);
CHECK(b != NULL);
array_str_t *c = array_str_deep_clone(a, strdup);

h[0] = 'B';

CHECK(strcmp(array_str_get(a, 0), "Bello") == 0);
CHECK(strcmp(array_str_get(b, 0), "Bello") == 0);
CHECK(strcmp(array_str_get(c, 0), "Hello") == 0);

array_str_destroy(&a);
CHECK(a == NULL);
array_str_destroy(&b);
CHECK(b == NULL);

array_str_map(c, free_str);
array_str_destroy(&c);
CHECK(c == NULL);

END_TEST

START_TEST(array_test6)

array_int_t *a = array_int_init();
CHECK(a != NULL);
for (int i = 0; i < 5; i++) {
  array_int_append(a, i);
}
array_int_t *b = array_int_slice(a, 1, 4);
CHECK(b != NULL);

for (int i = 0; i < 5; i++) {
  array_int_set(a, i, -1);
}
for (int i = 0; i < 3; i++) {
  CHECK(array_int_get(b, i) == i + 1);
}
array_int_destroy(&a);
CHECK(a == NULL);
array_int_destroy(&b);
CHECK(b == NULL);

END_TEST

START_TEST(array_test7)

char s1[] = "hello";
char s2[] = "world";
char s3[] = "all!";

array_str_t *a = array_str_init();
CHECK(a != NULL);
array_str_append(a, s1);
array_str_append(a, s2);
array_str_append(a, s3);

array_str_t *b = array_str_slice(a, 1, 2);
CHECK(b != NULL);

array_str_t *c = array_str_deep_slice(a, 1, 2, strdup);
CHECK(c != NULL);

s2[0] = 'W';

CHECK(strcmp(array_str_get(b,0), "World") == 0);
CHECK(strcmp(array_str_get(c,0), "world") == 0);

array_str_destroy(&a);
array_str_destroy(&b);
array_str_map(c, free_str);
array_str_destroy(&c);

CHECK(a == NULL);
CHECK(b == NULL);
CHECK(c == NULL);

END_TEST

START_TEST(array_test8)

array_int_t *a = array_int_init();
CHECK(a != NULL);
int null_value = -1;
array_int_set_null_value(a, null_value);
CHECK(array_int_pop(a) == null_value);
CHECK(array_int_pop_first(a) == null_value);
array_int_destroy(&a);
CHECK(a == NULL);

array_str_t *b = array_str_init();
CHECK(b != NULL);
CHECK(array_str_pop(b) == NULL);
CHECK(array_str_pop_first(b) == NULL);
array_str_destroy(&b);
CHECK(b == NULL);

END_TEST

START_TEST(array_test9)

array_int_t *a = array_int_init();
CHECK(a != NULL);

int n = 10;
seed_rand(1);
for (int i = 0; i < n; i++) {
  int v = (get_rand() >> 8) & 0xffff;
  array_int_append(a, v);
}
array_int_sort(a);
for (int i = 0; i < n - 1; i++) {
  CHECK(array_int_get(a, i) <= array_int_get(a, i + 1));
}  
array_int_destroy(&a);
CHECK(a == NULL);

END_TEST


START_TEST(array_test10)

array_pair_t *a = array_pair_init();
CHECK(a != NULL);

int n = 10;
seed_rand(2);
for (int i = 0; i < n; i++) {
  pair_t q = {
    .x = (get_rand() >> 4) & 0xf,
    .y = (get_rand() >> 8) & 0xffff,
  };
  array_pair_append(a, q);
}
array_pair_set_comp(a, &comp_pair);
array_pair_sort(a);
for (int i = 0; i < n - 1; i++) {
  pair_t s = array_pair_get(a, i);
  pair_t t = array_pair_get(a, i + 1);
  CHECK(s.x < t.x || (s.x == t.x && s.y <= t.y));
}

array_pair_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(array_test11)

array_int_t *a = array_int_init();
CHECK(a != NULL);

int n = 10;
seed_rand(3);
for (int i = 0; i < n; i++) {
  int v = (get_rand() >> 8) & 0xffff;
  if ((v % 5) == 0) {
    v++;
  }
  array_int_append(a, v);
}
array_int_sort(a);

for (int i = 0; i < n; i++) {
  int v = array_int_get(a, i);
  CHECK(array_int_bisect(a, v) == i);
}

int mid = (array_int_get(a, 0) + array_int_get(a, n - 1)) / 2;
mid += (5 - (mid % 5));
CHECK(array_int_bisect(a, mid) == -1);

array_int_destroy(&a);
CHECK(a == NULL);
     
END_TEST

int square(int x) {
  return x * x;
}
int add(int x, int y) {
  return x + y;
}

START_TEST(array_test12)

array_int_t *a = array_int_init();
CHECK(a != NULL);

int n = 10;
for (int i = 1; i <= n; i++) {
  array_int_append(a, i);
}
array_int_map(a, square);
int sum_sqrs = array_int_fold(a, add);

CHECK(sum_sqrs == n * (n + 1) * (2 * n + 1) / 6);

array_int_destroy(&a);
CHECK(a == NULL);

END_TEST

START_TEST(array_test13)

array_int_t *a = array_int_init();
CHECK(a != NULL);

array_int_t *b = array_int_init();
CHECK(a != NULL);


int n = 10;
seed_rand(4);
for (int i = 0; i < n; i++) {
  int v = (get_rand() >> 8) & 0xffff;
  array_int_append(a, v);
  array_int_heappush(b, v);
}
array_int_sort(a);

for (int i = 0; i < n; i++) {
  int v1 = array_int_get(a, n - 1 - i);
  int v2 = array_int_heappop(b);
  CHECK(v1 == v2);
}

CHECK(array_int_size(b) == 0);

array_int_destroy(&b);
array_int_destroy(&a);

END_TEST

static int plus_one(int x) {
  return x + 1;
}

static int sum2(int acc, const int x) {
  return acc + x;
}

static int running_sum(int prev, int cur) {
  return prev + cur;
}

static int dup_int(const int x) {
  return x;
}

// sorting: sizes around the insertion sort cutoff and above it, with many equal keys; the sort
// must be stable, already sorted and reversed input must work, and a missing comparison
// function, a NULL array and short arrays are handled
START_TEST(array_test14)

int sizes[] = {0, 1, 2, 3, 15, 16, 17, 18, 31, 32, 33, 64, 100, 257, 1000, 5000};
seed_rand(14);
for (int si = 0; si < 16; si++) {
  int n = sizes[si];
  for (int shape = 0; shape < 4; shape++) {   // random, sorted, reversed, all equal
    array_rec_t *a = array_rec_init();
    CHECK(array_rec_set_comp(a, comp_rec) == 0);
    for (int i = 0; i < n; i++) {
      int k = shape == 0 ? (int) (get_rand() % 20) : shape == 1 ? i / 3 : shape == 2 ? (n - i) / 3 : 7;
      rec_t r = {k, i};
      CHECK(array_rec_append(a, r) == 0);
    }
    CHECK(array_rec_sort(a) == 0);
    CHECK(array_rec_size(a) == (size_t) n);
    for (int i = 1; i < n; i++) {
      rec_t p = array_rec_get(a, i - 1);
      rec_t q = array_rec_get(a, i);
      CHECK(p.k < q.k || (p.k == q.k && p.tag < q.tag));   // in order, and equal keys in their original order
    }
    // every record is still there exactly once
    long tag_sum = 0;
    for (int i = 0; i < n; i++) {
      tag_sum += array_rec_get(a, i).tag;
    }
    CHECK(tag_sum == (long) n * (n - 1) / 2);
    // sorting a sorted array changes nothing
    array_rec_t *b = array_rec_clone(a);
    CHECK(array_rec_sort(b) == 0);
    for (int i = 0; i < n; i++) {
      CHECK(array_rec_get(a, i).tag == array_rec_get(b, i).tag);
    }
    array_rec_destroy(&b);
    array_rec_destroy(&a);
  }
}

// no comparison function, NULL array
array_rec_t *c = array_rec_init();
rec_t one = {1, 1};
array_rec_append(c, one);
array_rec_append(c, one);
CHECK(array_rec_sort(c) == -1);
CHECK(array_rec_size(c) == 2);
array_rec_destroy(&c);
CHECK(array_rec_sort(NULL) == -1);

END_TEST

// pop_first and append together make a queue which reuses the room at the front
START_TEST(array_test15)

array_int_t *a = array_int_init();
CHECK(a != NULL);
for (int i = 0; i < 8; i++) {
  array_int_append(a, i);
}
CHECK(array_int_pop_first(a) == 0);
CHECK(array_int_pop_first(a) == 1);
CHECK(array_int_size(a) == 6);
CHECK(array_int_get(a, 0) == 2);
CHECK(array_int_get(a, 5) == 7);

// a long run of appends and pop_firsts with about 8 entries live: the memory must not grow
seed_rand(15);
int next_in = 8, next_out = 2;
size_t max_slots = 0;
for (int step = 0; step < 100000; step++) {
  if (array_int_size(a) < 8 || (get_rand() & 3) != 0) {
    CHECK(array_int_append(a, next_in++) == 0);
  } else {
    CHECK(array_int_pop_first(a) == next_out++);
  }
  size_t slots = (size_t) (a->data - a->alloc) + array_int_capacity(a);
  if (slots > max_slots) {
    max_slots = slots;
  }
  CHECK(array_int_size(a) <= array_int_capacity(a));
}
CHECK(max_slots < 4 * (array_int_size(a) + 16) + 1024);   // a few thousand slots at most
for (size_t i = 0; i < array_int_size(a); i++) {
  CHECK(array_int_get(a, i) == next_out + (int) i);
}

// a full array with room at the front slides down instead of growing
array_int_t *b = array_int_init();
for (int i = 0; i < 16; i++) {
  array_int_append(b, i);
}
size_t cap = array_int_capacity(b);
for (int i = 0; i < 12; i++) {
  array_int_pop_first(b);
}
while (array_int_size(b) < array_int_capacity(b)) {
  array_int_append(b, 100);
}
int *alloc_before = b->alloc;
CHECK(array_int_append(b, 101) == 0);          // full now: slides down
CHECK(b->alloc == alloc_before && b->data == b->alloc);
CHECK(array_int_capacity(b) >= cap);
CHECK(array_int_get(b, 0) == 12 && array_int_get(b, array_int_size(b) - 1) == 101);
array_int_destroy(&b);

// the null value is what an empty array and an index out of range give
CHECK(array_int_set_null_value(a, -77) == 0);
while (array_int_size(a) > 0) {
  array_int_pop(a);
}
CHECK(array_int_pop(a) == -77);
CHECK(array_int_pop_first(a) == -77);
CHECK(array_int_get(a, 3) == -77);
CHECK(array_int_heappop(a) == -77);
array_int_append(a, 5);
CHECK(array_int_get(a, 1) == -77);
CHECK(array_int_pop_first(a) == 5);
CHECK(array_int_size(a) == 0);
CHECK(array_int_pop_first(a) == -77);

array_int_destroy(&a);

END_TEST

// arguments which are NULL or out of range are refused, never dereferenced
START_TEST(array_test16)

CHECK(array_int_set_comp(NULL, NULL) == -1);
array_int_t *a = array_int_init();
CHECK(array_int_set_comp(a, NULL) == -1);
CHECK(a->comp != NULL);          // the default comparison function is still there
CHECK(array_int_set_null_value(NULL, 1) == -1);
CHECK(array_int_append(NULL, 1) == -1);
CHECK(array_int_set(NULL, 1, 0) == -1);
CHECK(array_int_set(a, 1, 0) == -1);   // empty: index out of range
CHECK(array_int_map(NULL, plus_one) == -1);
CHECK(array_int_map(a, NULL) == -1);
CHECK(array_int_scan(NULL, running_sum) == -1);
CHECK(array_int_heappush(NULL, 1) == -1);
CHECK(array_int_heapify(NULL) == -1);
CHECK(array_int_destroy(NULL) == -1);
array_int_t *none = NULL;
CHECK(array_int_destroy(&none) == -1);
CHECK(array_int_clone(NULL) == NULL);
CHECK(array_int_slice(NULL, 0, 0) == NULL);

for (int i = 0; i < 5; i++) {
  array_int_append(a, i * 10);
}
CHECK(array_int_set(a, 99, 4) == 0 && array_int_get(a, 4) == 99);
CHECK(array_int_set(a, 1, 5) == -1);
CHECK(array_int_size(a) == 5);

// slices: [left, right) with the empty slice allowed, and bad ranges refused
array_int_t *s = array_int_slice(a, 1, 4);
CHECK(s != NULL && array_int_size(s) == 3 && array_int_get(s, 0) == 10 && array_int_get(s, 2) == 30);
array_int_destroy(&s);
s = array_int_slice(a, 2, 2);
CHECK(s != NULL && array_int_size(s) == 0);
array_int_destroy(&s);
s = array_int_slice(a, 0, 5);
CHECK(s != NULL && array_int_size(s) == 5);
array_int_destroy(&s);
CHECK(array_int_slice(a, 3, 2) == NULL);
CHECK(array_int_slice(a, 0, 6) == NULL);
CHECK(array_int_slice(a, 6, 6) == NULL);

// map, scan and the folds
CHECK(array_int_map(a, plus_one) == 0);
CHECK(array_int_get(a, 0) == 1 && array_int_get(a, 4) == 100);
array_int_t *r = array_int_init();
for (int i = 1; i <= 5; i++) {
  array_int_append(r, i);
}
CHECK(array_int_scan(r, running_sum) == 0);
int expect[] = {1, 3, 6, 10, 15};
for (int i = 0; i < 5; i++) {
  CHECK(array_int_get(r, i) == expect[i]);
}
CHECK(array_int_fold(r, sum2) == 35);
CHECK(array_int_fold2(r, 100, sum2) == 135);
array_int_t *empty = array_int_init();
CHECK(array_int_fold2(empty, 7, sum2) == 7);
CHECK(array_int_scan(empty, running_sum) == 0);

// a deep clone and a deep slice apply the function to every entry
array_int_t *d = array_int_deep_clone(r, dup_int);
CHECK(d != NULL && array_int_size(d) == 5 && array_int_get(d, 4) == 15);
array_int_t *ds = array_int_deep_slice(r, 1, 3, dup_int);
CHECK(ds != NULL && array_int_size(ds) == 2 && array_int_get(ds, 0) == 3);

// clones are independent, keep the comparison function and the null value
array_int_set_null_value(r, -9);
array_int_t *c = array_int_clone(r);
CHECK(array_int_get(c, 100) == -9);
CHECK(c->comp == r->comp);
array_int_set(c, 0, 0);
CHECK(array_int_get(r, 0) == 1);

array_int_destroy(&c);
array_int_destroy(&ds);
array_int_destroy(&d);
array_int_destroy(&empty);
array_int_destroy(&r);
array_int_destroy(&a);

END_TEST

// searching a sorted array with duplicates, against a linear model
START_TEST(array_test17)

array_int_t *a = array_int_init();
CHECK(array_int_bisect(a, 5) == -1);                 // empty
CHECK(array_int_bisect_upper(a, 5) == -1);
CHECK(array_int_bisect_lower(a, 5) == 0);
CHECK(array_int_index(a, 5) == -1);

int values[] = {2, 2, 4, 4, 4, 7, 9, 9, 12};
int n = 9;
for (int i = 0; i < n; i++) {
  array_int_append(a, values[i]);
}
for (int v = 0; v <= 14; v++) {
  ssize_t lower = 0, upper = -1;
  int found = 0;
  for (int i = 0; i < n; i++) {
    if (values[i] < v) {
      lower = i + 1;
    }
    if (values[i] <= v) {
      upper = i;
    }
    if (values[i] == v) {
      found = 1;
    }
  }
  CHECK(array_int_bisect_lower(a, v) == lower);
  CHECK(array_int_bisect_upper(a, v) == upper);
  ssize_t b = array_int_bisect(a, v);
  CHECK((b >= 0) == (found != 0));
  if (b >= 0) {
    CHECK(values[b] == v);
  }
  ssize_t x = array_int_index(a, v);
  if (found) {
    CHECK(x >= 0 && values[x] == v && (x == 0 || values[x - 1] != v));   // the first one
  } else {
    CHECK(x == -1);
  }
}

// the loop which visits the entries between two values, inclusive
int count = 0;
for (ssize_t i = array_int_bisect_lower(a, 4); i <= array_int_bisect_upper(a, 9); i++) {
  count++;
}
CHECK(count == 6);   // 4 4 4 7 9 9
count = 0;
for (ssize_t i = array_int_bisect_lower(a, 5); i <= array_int_bisect_upper(a, 6); i++) {
  count++;
}
CHECK(count == 0);   // nothing between 5 and 6

array_int_destroy(&a);

END_TEST

// heapify and the heap functions, with duplicates and against a sorted copy
START_TEST(array_test18)

seed_rand(18);
for (int n = 0; n <= 300; n += 13) {
  array_int_t *a = array_int_init();
  array_int_t *sorted = array_int_init();
  for (int i = 0; i < n; i++) {
    int v = (int) (get_rand() % 40);
    array_int_append(a, v);
    array_int_append(sorted, v);
  }
  array_int_sort(sorted);
  CHECK(array_int_heapify(a) == 0);
  CHECK(array_int_size(a) == (size_t) n);
  for (int i = 0; i < n; i++) {
    CHECK(array_int_heappop(a) == array_int_get(sorted, n - 1 - i));
  }
  CHECK(array_int_size(a) == 0);
  array_int_destroy(&a);
  array_int_destroy(&sorted);
}

// pushes and pops interleaved
array_int_t *h = array_int_init();
array_int_t *model = array_int_init();
for (int step = 0; step < 3000; step++) {
  if (array_int_size(h) == 0 || (get_rand() % 3) != 0) {
    int v = (int) (get_rand() % 100);
    CHECK(array_int_heappush(h, v) == 0);
    array_int_append(model, v);
    array_int_sort(model);
  } else {
    int top = array_int_heappop(h);
    CHECK(top == array_int_pop(model));   // the model is sorted: its last entry is the largest
  }
  CHECK(array_int_size(h) == array_int_size(model));
}

// no comparison function: heappush and heapify refuse
array_rec_t *r = array_rec_init();
rec_t x = {1, 1};
CHECK(array_rec_heappush(r, x) == -1);
CHECK(array_rec_heapify(r) == -1);
CHECK(array_rec_size(r) == 0);
array_rec_destroy(&r);

array_int_destroy(&h);
array_int_destroy(&model);

END_TEST

// out of memory: every allocation can fail, and a failure leaves what exists intact
START_TEST(array_test19)

// init: the struct, then the storage
for (long budget = 0; budget < 2; budget++) {
  aoom_budget = budget;
  CHECK(array_oom_init() == NULL);
}
aoom_budget = -1;

// init2: both allocations, and a size too large to allocate at all
for (long budget = 0; budget < 2; budget++) {
  aoom_budget = budget;
  CHECK(array_oom_init2(5, 0) == NULL);
}
aoom_budget = -1;
CHECK(array_oom_init2(SIZE_MAX / 2, 0) == NULL);
CHECK(array_oom_init2(SIZE_MAX / sizeof(int) + 1, 0) == NULL);
array_oom_t *z = array_oom_init2(0, 0);
CHECK(z != NULL && array_oom_size(z) == 0);
array_oom_destroy(&z);

array_oom_t *a = array_oom_init();
for (int i = 0; i < 10; i++) {
  CHECK(array_oom_append(a, i) == 0);
}

// clone and slice: the struct, then the storage
for (long budget = 0; budget < 2; budget++) {
  aoom_budget = budget;
  CHECK(array_oom_clone(a) == NULL);
  aoom_budget = budget;
  CHECK(array_oom_deep_clone(a, NULL) == NULL);
  aoom_budget = budget;
  CHECK(array_oom_slice(a, 1, 5) == NULL);
  aoom_budget = budget;
  CHECK(array_oom_deep_slice(a, 1, 5, NULL) == NULL);
}
aoom_budget = -1;

// append when the array is full needs memory: a failure leaves the array as it was
while (array_oom_size(a) < array_oom_capacity(a)) {
  array_oom_append(a, 50);
}
size_t size = array_oom_size(a);
aoom_budget = 0;
CHECK(array_oom_append(a, 77) == -1);
CHECK(array_oom_size(a) == size);
CHECK(array_oom_get(a, 0) == 0 && array_oom_get(a, 9) == 9);
CHECK(array_oom_heappush(a, 77) == -1);
CHECK(array_oom_size(a) == size);
aoom_budget = -1;
CHECK(array_oom_append(a, 77) == 0);

// sort needs a buffer: a failure leaves the order as it was
array_oom_t *u = array_oom_init();
for (int i = 20; i > 0; i--) {
  array_oom_append(u, i);
}
aoom_budget = 0;
CHECK(array_oom_sort(u) == -1);
aoom_budget = -1;
CHECK(array_oom_get(u, 0) == 20 && array_oom_get(u, 19) == 1);
CHECK(array_oom_sort(u) == 0);
CHECK(array_oom_get(u, 0) == 1 && array_oom_get(u, 19) == 20);

// the size limits of append are checked before any allocation: with a capacity which could not
// be doubled, append refuses
array_oom_t *big = array_oom_init();
size_t keep_capacity = big->capacity;
big->size = big->capacity = SIZE_MAX / 2 / sizeof(int) + 1;
CHECK(array_oom_append(big, 1) == -1);
big->size = big->capacity = SIZE_MAX / 2 + 2;   // doubling would wrap around to a tiny number
CHECK(array_oom_append(big, 1) == -1);
big->capacity = SIZE_MAX / 8;   // doubling would fit, but not with the room at the front
big->size = big->capacity;
big->data = big->alloc + 2;
CHECK(array_oom_append(big, 1) == -1);
big->data = big->alloc;
big->size = 0;
big->capacity = keep_capacity;
array_oom_destroy(&big);

// reading a file needs an array, and a failure to make one is reported
// (the struct, the first storage, then the buffer for the entries, can each fail)
CHECK(array_oom_serialize(a, "array_test19.bin") == 0);
for (long budget = 0; budget < 3; budget++) {
  aoom_budget = budget;
  CHECK(array_oom_deserialize("array_test19.bin") == NULL);
}
aoom_budget = -1;
array_oom_t *loaded = array_oom_deserialize("array_test19.bin");
CHECK(loaded != NULL && array_oom_size(loaded) == array_oom_size(a));
array_oom_destroy(&loaded);
remove("array_test19.bin");

array_oom_destroy(&u);
array_oom_destroy(&a);

END_TEST

// files: a file which is damaged or does not belong to this array type is refused
START_TEST(array_test20)

#define ARRAY_FILE "array_test20.bin"
array_int_t *a = array_int_init();
for (int i = 0; i < 10; i++) {
  array_int_append(a, i * i);
}
CHECK(array_int_serialize(a, ARRAY_FILE) == 0);
CHECK(array_int_serialize(NULL, ARRAY_FILE) == -1);
CHECK(array_int_serialize(a, NULL) == -1);
CHECK(array_int_serialize(a, "no_such_directory/array.bin") == -1);
// a write error is reported (writing to a full device)
if (access("/dev/full", W_OK) == 0) {
  CHECK(array_int_serialize(a, "/dev/full") == -1);
}

array_int_t *b = array_int_deserialize(ARRAY_FILE);
CHECK(b != NULL && array_int_size(b) == 10);
for (int i = 0; i < 10; i++) {
  CHECK(array_int_get(b, i) == i * i);
}
array_int_destroy(&b);

// the same file read as an array of another type, and a file which does not exist
CHECK(array_str_deserialize(ARRAY_FILE) == NULL);
CHECK(array_int_deserialize("no_such_file.bin") == NULL);
CHECK(array_int_deserialize(NULL) == NULL);

// the file is cut short in the middle of the entries
FILE *f = fopen(ARRAY_FILE, "rb");
fseek(f, 0, SEEK_END);
long len = ftell(f);
fclose(f);
CHECK(truncate(ARRAY_FILE, len - 3) == 0);
CHECK(array_int_deserialize(ARRAY_FILE) == NULL);

// the count of entries is more than a size_t of entries could hold
CHECK(array_int_serialize(a, ARRAY_FILE) == 0);
f = fopen(ARRAY_FILE, "r+b");
size_t huge = SIZE_MAX;
fseek(f, 8 + 1 + 3 + 8, SEEK_SET);   // header, the length and text of "int", the size of an entry
fwrite(&huge, sizeof(huge), 1, f);
fclose(f);
CHECK(array_int_deserialize(ARRAY_FILE) == NULL);
// (a count which fits in a size_t but cannot be allocated is tested in array_test19)

// a file with the wrong header
f = fopen(ARRAY_FILE, "wb");
fwrite("NotAnArray", 1, 10, f);
fclose(f);
CHECK(array_int_deserialize(ARRAY_FILE) == NULL);

// a file which is right except for its header
CHECK(array_int_serialize(a, ARRAY_FILE) == 0);
f = fopen(ARRAY_FILE, "r+b");
fwrite("Xrray===", 1, 8, f);
fclose(f);
CHECK(array_int_deserialize(ARRAY_FILE) == NULL);
CHECK(array_int_serialize(a, ARRAY_FILE) == 0);
b = array_int_deserialize(ARRAY_FILE);
CHECK(b != NULL && array_int_size(b) == 10);
array_int_destroy(&b);

// an empty array makes a file which reads back as an empty array
array_int_t *e = array_int_init();
CHECK(array_int_serialize(e, ARRAY_FILE) == 0);
array_int_t *e2 = array_int_deserialize(ARRAY_FILE);
CHECK(e2 != NULL && array_int_size(e2) == 0);

remove(ARRAY_FILE);
array_int_destroy(&e2);
array_int_destroy(&e);
array_int_destroy(&a);
#undef ARRAY_FILE

END_TEST
