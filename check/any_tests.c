#include <check.h>
#include <any.h>

int comp_any_link_check(void);

START_TEST(any_test1)

// each initializer records the type of its argument
CHECK(any_get_type(any_init((int8_t) -5)) == ANY_I8);
CHECK(any_get_type(any_init((int16_t) -5)) == ANY_I16);
CHECK(any_get_type(any_init((int32_t) -5)) == ANY_I32);
CHECK(any_get_type(any_init((int64_t) -5)) == ANY_I64);
CHECK(any_get_type(any_init((uint8_t) 200)) == ANY_U8);
CHECK(any_get_type(any_init((uint16_t) 60000)) == ANY_U16);
CHECK(any_get_type(any_init((uint32_t) 4000000000u)) == ANY_U32);
CHECK(any_get_type(any_init((uint64_t) 1 << 63)) == ANY_U64);
CHECK(any_get_type(any_init(1.5f)) == ANY_F32);
CHECK(any_get_type(any_init(2.5)) == ANY_F64);

int x = 0;
CHECK(any_get_type(any_init((void *) &x)) == ANY_PTR);
CHECK(any_get_type(any_init((char *) "text")) == ANY_PTR);
CHECK(any_get_type(any_init(5)) == ANY_I32);

END_TEST

START_TEST(any_test2)

any_t a = any_init((int8_t) -5);
any_t b = any_init((uint8_t) 200);
any_t c = any_init((int64_t) -1234567890123);
any_t d = any_init((uint64_t) 1 << 63);
any_t e = any_init(2.5);
any_t f = any_init(1.5f);

CHECK(ANY_VALUE(a, i8) == -5);
CHECK(ANY_VALUE(b, u8) == 200);
CHECK(ANY_VALUE(c, i64) == -1234567890123);
CHECK(ANY_VALUE(d, u64) == (uint64_t) 1 << 63);
CHECK(ANY_VALUE(e, f64) == 2.5);
CHECK(ANY_VALUE(f, f32) == 1.5f);

char *text = "text";
any_t p = any_init(text);
CHECK((char *) ANY_VALUE(p, ptr) == text);

// the argument of ANY_VALUE may be an expression
any_t arr[2] = {any_init(7), any_init(8)};
CHECK(ANY_VALUE(arr[1], i32) == 8);

END_TEST

START_TEST(any_test3)

any_t a = any_init(7);
any_t b = any_init((uint8_t) 200);

int32_t *p = any_get_i32(&a);
CHECK(p != NULL);
CHECK(*p == 7);

*p = 9;
CHECK(ANY_VALUE(a, i32) == 9);

CHECK(any_get_i32(&b) == NULL);
CHECK(any_get_i32(NULL) == NULL);

END_TEST

// Another translation unit includes any.h too; linking both must work.
START_TEST(any_test4)

CHECK(comp_any_link_check() == 1);

END_TEST
