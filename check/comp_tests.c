#include <check.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <comp.h>
#include <hash.h>
#include <any.h>

// The containers pick up the defaults for every spelling of an integer type (before, `long long`
// had none on Linux, and `long` and `size_t` had none on macOS).
#define data_t long long
#define prefix llong
#include <array.h>
#undef data_t
#undef prefix

#define data_t unsigned long long
#define prefix ullong
#include <tree.h>
#undef data_t
#undef prefix

#define hkey_t long long
#define value_t int
#define prefix llong
#include <hash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

#define hkey_t size_t
#define value_t int
#define prefix size
#include <hash_table.h>
#undef prefix
#undef value_t
#undef hkey_t

typedef struct {
  int x;
} comp_struct_t;

// A second translation unit that includes any.h: its functions must not clash
// with the ones in any_tests.c at link time.
int comp_any_link_check(void) {
  any_t a = any_init(3.0);
  return any_get_type(a) == ANY_F64;
}

START_TEST(comp_test1)

int64_t a64 = -5, b64 = 3;
CHECK(comp_int64(&a64, &b64) < 0);
CHECK(comp_int64(&b64, &a64) > 0);
CHECK(comp_int64(&a64, &a64) == 0);

int32_t a32 = -5, b32 = 3;
CHECK(comp_int32(&a32, &b32) < 0);
CHECK(comp_int32(&b32, &a32) > 0);
CHECK(comp_int32(&a32, &a32) == 0);

int16_t a16 = -5, b16 = 3;
CHECK(comp_int16(&a16, &b16) < 0);
CHECK(comp_int16(&b16, &a16) > 0);

int8_t a8 = -5, b8 = 3;
CHECK(comp_int8(&a8, &b8) < 0);
CHECK(comp_int8(&b8, &a8) > 0);
CHECK(comp_int8(&a8, &a8) == 0);

float af = -1.5f, bf = 2.5f;
CHECK(comp_float(&af, &bf) < 0);
CHECK(comp_float(&bf, &af) > 0);
CHECK(comp_float(&af, &af) == 0);

double ad = -1.5, bd = 2.5;
CHECK(comp_double(&ad, &bd) < 0);
CHECK(comp_double(&bd, &ad) > 0);

char ac = 'a', bc = 'b';
CHECK(comp_char(&ac, &bc) < 0);
CHECK(comp_char(&bc, &ac) > 0);

END_TEST

// Unsigned types must compare as unsigned: a large value is greater.
START_TEST(comp_test2)

uint64_t a64 = 5, b64 = UINT64_C(1) << 63;
CHECK(comp_uint64(&a64, &b64) < 0);
CHECK(comp_uint64(&b64, &a64) > 0);

uint32_t a32 = 5, b32 = 4000000000u;
CHECK(comp_uint32(&a32, &b32) < 0);
CHECK(comp_uint32(&b32, &a32) > 0);

uint16_t a16 = 5, b16 = 60000;
CHECK(comp_uint16(&a16, &b16) < 0);
CHECK(comp_uint16(&b16, &a16) > 0);

uint8_t a8 = 5, b8 = 200;
CHECK(comp_uint8(&a8, &b8) < 0);
CHECK(comp_uint8(&b8, &a8) > 0);
CHECK(comp_uint8(&a8, &a8) == 0);

END_TEST

START_TEST(comp_test3)

char *a = "apple";
char *b = "banana";
CHECK(comp_str(&a, &b) < 0);
CHECK(comp_str(&b, &a) > 0);
CHECK(comp_str(&a, &a) == 0);

const char *ca = "apple";
const char *cb = "banana";
CHECK(comp_cstr(&ca, &cb) < 0);
CHECK(comp_cstr(&cb, &ca) > 0);

CHECK(comp_str_data("apple", "banana") < 0);
CHECK(comp_str_data("banana", "apple") > 0);
CHECK(comp_str_data("apple", "apple") == 0);
CHECK(comp_cstr_data("apple", "banana") < 0);
CHECK(comp_cstr_data("apple", "apple") == 0);

END_TEST

// DEFAULT_COMP finds a comparator for every supported type, whichever standard type the
// platform's int64_t and friends happen to be, and none for types it does not know.
#define CHECK_DEFAULT_COMP(T, lo, hi) \
  do { \
    T v = 0; \
    int (*cmp)(T *, T *) = DEFAULT_COMP(v); \
    CHECK(cmp != NULL); \
    T a = (lo), b = (hi); \
    CHECK(cmp(&a, &b) < 0); \
    CHECK(cmp(&b, &a) > 0); \
    CHECK(cmp(&a, &a) == 0); \
  } while (0)

START_TEST(comp_test4)

CHECK_DEFAULT_COMP(int64_t, -5, 3);
CHECK_DEFAULT_COMP(int32_t, -5, 3);
CHECK_DEFAULT_COMP(int16_t, -5, 3);
CHECK_DEFAULT_COMP(int8_t, -5, 3);
CHECK_DEFAULT_COMP(uint64_t, 1, UINT64_MAX);
CHECK_DEFAULT_COMP(uint32_t, 1, UINT32_MAX);
CHECK_DEFAULT_COMP(uint16_t, 1, UINT16_MAX);
CHECK_DEFAULT_COMP(uint8_t, 1, UINT8_MAX);
CHECK_DEFAULT_COMP(signed char, -5, 3);
CHECK_DEFAULT_COMP(short, -5, 3);
CHECK_DEFAULT_COMP(int, -5, 3);
CHECK_DEFAULT_COMP(long, -5, 3);
CHECK_DEFAULT_COMP(long long, -5, 3);
CHECK_DEFAULT_COMP(unsigned char, 1, UCHAR_MAX);
CHECK_DEFAULT_COMP(unsigned short, 1, USHRT_MAX);
CHECK_DEFAULT_COMP(unsigned int, 1, UINT_MAX);
CHECK_DEFAULT_COMP(unsigned long, 1, ULONG_MAX);
CHECK_DEFAULT_COMP(unsigned long long, 1, ULLONG_MAX);
CHECK_DEFAULT_COMP(size_t, 1, SIZE_MAX);
CHECK_DEFAULT_COMP(ssize_t, -5, 3);
CHECK_DEFAULT_COMP(_Bool, 0, 1);
CHECK_DEFAULT_COMP(float, -1.5f, 2.5f);
CHECK_DEFAULT_COMP(double, -1.5, 2.5);
CHECK_DEFAULT_COMP(char, 'a', 'b');

char *s = NULL;
const char *cs = NULL;
comp_struct_t st = {0};
long double ld = 0;
int32_t i32 = 0;

CHECK(DEFAULT_COMP(s) == &comp_str);
CHECK(DEFAULT_COMP(cs) == &comp_cstr);
CHECK(DEFAULT_COMP(st) == NULL);
CHECK(DEFAULT_COMP(ld) == NULL);

CHECK(DEFAULT_COMP_TYPE(s) == &comp_str_data);
CHECK(DEFAULT_COMP_TYPE(cs) == &comp_cstr_data);
CHECK(DEFAULT_COMP_TYPE(i32) == NULL);
CHECK(DEFAULT_COMP_TYPE(st) == NULL);

// the fixed width names are still available
int64_t a64 = 1, b64 = 2;
CHECK(comp_int64(&a64, &b64) < 0);
uint8_t a8 = 1, b8 = 2;
CHECK(comp_uint8(&b8, &a8) > 0);

END_TEST

START_TEST(comp_test5)

CHECK(hash_uint64_t(5) == 5);
CHECK(hash_int32_t(-1) == (uint64_t) -1);
CHECK(hash_str("abc") == hash_str("abc"));
CHECK(hash_str("abc") != hash_str("abd"));
CHECK(hash_str("") == hash_str(""));

// equal keys hash equally, and different keys almost always differ
CHECK(hash_double(0.0) == hash_double(-0.0));
CHECK(hash_double(1.5) != hash_double(2.5));
CHECK(hash_float(1.5f) != hash_float(2.5f));

// the mixer spreads keys that differ only in high bits or that are multiples of a power of two
uint64_t seen_low = 0;
for (int i = 0; i < 64; i++) {
  seen_low |= UINT64_C(1) << (hash_mix64((uint64_t) i << 40) & 63);
}
CHECK(__builtin_popcountll(seen_low) > 30);

CHECK(hash_mix64(1) != hash_mix64(2));
CHECK(hash_mix64(0) == hash_mix64(0));

char *s = "x";
const char *cs = "x";
int32_t i = 0;
double d = 0;
// each string key type gets a function whose parameter is exactly that type
CHECK(DEFAULT_HASH(s) == &hash_str_mutable);
CHECK(DEFAULT_HASH(cs) == &hash_str);
CHECK(hash_str_mutable(s) == hash_str(cs));
CHECK(DEFAULT_HASH(d) == &hash_double);
CHECK(DEFAULT_HASH(i) != NULL);

// every integer type has a default hash, whichever standard type int64_t and friends are
#define CHECK_DEFAULT_HASH(T) \
  do { \
    T v = 7; \
    uint64_t (*h)(T) = DEFAULT_HASH(v); \
    CHECK(h != NULL); \
    CHECK(h(v) == 7); \
  } while (0)

CHECK_DEFAULT_HASH(int64_t);
CHECK_DEFAULT_HASH(int32_t);
CHECK_DEFAULT_HASH(int16_t);
CHECK_DEFAULT_HASH(int8_t);
CHECK_DEFAULT_HASH(uint64_t);
CHECK_DEFAULT_HASH(uint32_t);
CHECK_DEFAULT_HASH(uint16_t);
CHECK_DEFAULT_HASH(uint8_t);
CHECK_DEFAULT_HASH(signed char);
CHECK_DEFAULT_HASH(short);
CHECK_DEFAULT_HASH(int);
CHECK_DEFAULT_HASH(long);
CHECK_DEFAULT_HASH(long long);
CHECK_DEFAULT_HASH(unsigned char);
CHECK_DEFAULT_HASH(unsigned short);
CHECK_DEFAULT_HASH(unsigned int);
CHECK_DEFAULT_HASH(unsigned long);
CHECK_DEFAULT_HASH(unsigned long long);
CHECK_DEFAULT_HASH(size_t);
CHECK_DEFAULT_HASH(char);
{
  _Bool one = 1;
  CHECK(DEFAULT_HASH(one) != NULL && DEFAULT_HASH(one)(one) == 1);
  comp_struct_t st = {0};
  CHECK(DEFAULT_HASH(st) == NULL);
}

END_TEST

START_TEST(comp_test6)

array_llong_t *a = array_llong_init();
for (long long i = 5; i > 0; i--) {
  array_llong_append(a, i * 1000000000000LL);
}
CHECK(array_llong_sort(a) == 0);
CHECK(array_llong_get(a, 0) == 1000000000000LL);
CHECK(array_llong_bisect(a, 3000000000000LL) == 2);
array_llong_destroy(&a);

tree_ullong_t *t = tree_ullong_init();
CHECK(tree_ullong_insert(t, ULLONG_MAX, NULL) == 0);
CHECK(tree_ullong_insert(t, 1, NULL) == 0);
CHECK(tree_ullong_delete_min(t).key == 1);
tree_ullong_destroy(&t);

htable_llong_t *h = hash_llong_init(0);
CHECK(hash_llong_put(h, -5, 50) == 0);
CHECK(hash_llong_put(h, LLONG_MIN, 7) == 0);
int v = 0;
CHECK(hash_llong_get(h, -5, &v) == 1 && v == 50);
CHECK(hash_llong_get(h, LLONG_MIN, &v) == 1 && v == 7);
hash_llong_destroy(&h);

htable_size_t *z = hash_size_init(0);
CHECK(hash_size_put(z, (size_t) 12345, 1) == 0);
CHECK(hash_size_get(z, (size_t) 12345, &v) == 1 && v == 1);
CHECK(hash_size_get(z, (size_t) 12346, &v) == 0);
hash_size_destroy(&z);

END_TEST
