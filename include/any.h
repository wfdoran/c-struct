/* Some attempt at plain C version of std::any in C++

   https://en.cppreference.com/w/cpp/utility/any
 */

#ifndef ANY_H
#define ANY_H

#include <stdint.h>

typedef enum {i8, i16, i32, i64, u8, u16, u32, u64, f32, f64, ptr} any_type_t;

typedef struct {
  any_type_t type;
  union {
    int8_t i8_value;
    int16_t i16_value;
    int32_t i32_value;
    int64_t i64_value;
    uint8_t u8_value;
    uint16_t u16_value;
    uint32_t u32_value;
    uint64_t u64_value;
    float f32_value;
    double f64_value;
    void *ptr_value;
  };
} any_t;

any_t any_init_i8(int8_t v) {
  return (any_t) {.type = i8, .i8_value = v};
}

any_t any_init_i16(int16_t v) {
  return (any_t) {.type = i16, .i16_value = v};
}

any_t any_init_i32(int32_t v) {
  return (any_t) {.type = i32, .i32_value = v};
}

any_t any_init_i64(int64_t v) {
  return (any_t) {.type = i64, .i64_value = v};
}

any_t any_init_u8(uint8_t v) {
  return (any_t) {.type = i8, .u8_value = v};
}

any_t any_init_u16(uint16_t v) {
  return (any_t) {.type = i16, .u16_value = v};
}

any_t any_init_u32(uint32_t v) {
  return (any_t) {.type = i32, .u32_value = v};
}

any_t any_init_u64(uint64_t v) {
  return (any_t) {.type = i64, .u64_value = v};
}

any_t any_init_f32(float v) {
  return (any_t) {.type = f32, .f32_value = v};
}

any_t any_init_f64(double v) {
  return (any_t) {.type = f64, .f64_value = v};
}

any_t any_init_ptr(void *v) {
  return (any_t) {.type = ptr, .ptr_value = v};
}

#define any_init(X) _Generic((X),               \
    int8_t: any_init_i8,                        \
    int16_t: any_init_i16,                      \
    int32_t: any_init_i32,                      \
    int64_t: any_init_i64,                      \
    uint8_t: any_init_u8,                       \
    uint16_t: any_init_u16,                     \
    uint32_t: any_init_u32,                     \
    uint64_t: any_init_u64,                     \
    float: any_init_f32,                        \
    double: any_init_f64,                       \
    void*: any_init_ptr                         \
    )(X)

any_type_t any_get_type(any_t x) {
  return x.type;
}

int32_t* any_get_i32(any_t *x) {
  return &x->i32_value;
}

#define ANY_VALUE(x,y) x.y##_value


#endif

