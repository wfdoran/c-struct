#include <check.h>
#include <string.h>
#include <comp.h>
#include <hash.h>
#include <any.h>

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

// DEFAULT_COMP finds a comparator for every supported type, and none for
// types it does not know.
START_TEST(comp_test4)

int64_t i64 = 0;
int32_t i32 = 0;
int16_t i16 = 0;
int8_t i8v = 0;
uint64_t u64 = 0;
uint32_t u32 = 0;
uint16_t u16 = 0;
uint8_t u8v = 0;
float f = 0;
double d = 0;
char c = 0;
char *s = NULL;
const char *cs = NULL;
comp_struct_t st = {0};

CHECK(DEFAULT_COMP(i64) == &comp_int64);
CHECK(DEFAULT_COMP(i32) == &comp_int32);
CHECK(DEFAULT_COMP(i16) == &comp_int16);
CHECK(DEFAULT_COMP(i8v) == &comp_int8);
CHECK(DEFAULT_COMP(u64) == &comp_uint64);
CHECK(DEFAULT_COMP(u32) == &comp_uint32);
CHECK(DEFAULT_COMP(u16) == &comp_uint16);
CHECK(DEFAULT_COMP(u8v) == &comp_uint8);
CHECK(DEFAULT_COMP(f) == &comp_float);
CHECK(DEFAULT_COMP(d) == &comp_double);
CHECK(DEFAULT_COMP(c) == &comp_char);
CHECK(DEFAULT_COMP(s) == &comp_str);
CHECK(DEFAULT_COMP(cs) == &comp_cstr);
CHECK(DEFAULT_COMP(st) == NULL);

CHECK(DEFAULT_COMP_TYPE(s) == &comp_str_data);
CHECK(DEFAULT_COMP_TYPE(cs) == &comp_cstr_data);
CHECK(DEFAULT_COMP_TYPE(i32) == NULL);
CHECK(DEFAULT_COMP_TYPE(st) == NULL);

// the chosen comparator can be used through the generic pointer type
int (*cmp)(int32_t *, int32_t *) = DEFAULT_COMP(i32);
int32_t x = 1, y = 2;
CHECK(cmp(&x, &y) < 0);

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
CHECK(DEFAULT_HASH(i) == &hash_int32_t);
CHECK(DEFAULT_HASH(d) == &hash_double);

END_TEST
