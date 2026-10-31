#ifndef COMP_H
#define COMP_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* Comparators for the arithmetic types, in the form the containers use
   (pointers to the elements). The _Generic in DEFAULT_COMP below is written
   over the standard types: int64_t and friends are typedefs of them, and which
   one depends on the platform (int64_t is long on Linux, long long on macOS),
   so listing the typedef names would leave some spellings of the same type
   without a default.  The fixed width names are kept as functions for callers
   which use them directly. */
#define COMP_DEFINE(name, T)                                                   \
  static inline int comp_##name(T *a, T *b) {                                  \
    return (*a > *b) - (*a < *b);                                              \
  }

COMP_DEFINE(schar, signed char)
COMP_DEFINE(short, short)
COMP_DEFINE(int, int)
COMP_DEFINE(long, long)
COMP_DEFINE(llong, long long)
COMP_DEFINE(uchar, unsigned char)
COMP_DEFINE(ushort, unsigned short)
COMP_DEFINE(uint, unsigned int)
COMP_DEFINE(ulong, unsigned long)
COMP_DEFINE(ullong, unsigned long long)
COMP_DEFINE(bool, _Bool)

/* A total order for floating point, which sorting and the ordered containers
   need: every NaN is equal to every other NaN and greater than every number,
   and -0.0 equals 0.0 (as hash_double() assumes).  With the plain comparison a
   NaN would compare equal to every key. */
#define COMP_DEFINE_FLOAT(name, T)                                             \
  static inline int comp_##name(T *a, T *b) {                                  \
    if (*a < *b) {                                                             \
      return -1;                                                               \
    }                                                                          \
    if (*a > *b) {                                                             \
      return 1;                                                                \
    }                                                                          \
    if (*a == *b) {                                                            \
      return 0;                                                                \
    }                                                                          \
    return (*a != *a) - (*b != *b); /* unordered: a NaN is the larger */       \
  }

COMP_DEFINE_FLOAT(float, float)
COMP_DEFINE_FLOAT(double, double)
#undef COMP_DEFINE_FLOAT

COMP_DEFINE(char, char)

COMP_DEFINE(int64, int64_t)
COMP_DEFINE(int32, int32_t)
COMP_DEFINE(int16, int16_t)
COMP_DEFINE(int8, int8_t)
COMP_DEFINE(uint64, uint64_t)
COMP_DEFINE(uint32, uint32_t)
COMP_DEFINE(uint16, uint16_t)
COMP_DEFINE(uint8, uint8_t)

#undef COMP_DEFINE

static inline int comp_str(char **a, char **b) {
  return strcmp(*a, *b);
}

static inline int comp_cstr(const char **a, const char **b) {
  return strcmp(*a, *b);
}

#define DEFAULT_COMP(x)                                                        \
  _Generic((x), \
    signed char: &comp_schar, \
    short: &comp_short, \
    int: &comp_int, \
    long: &comp_long, \
    long long: &comp_llong, \
    unsigned char: &comp_uchar, \
    unsigned short: &comp_ushort, \
    unsigned int: &comp_uint, \
    unsigned long: &comp_ulong, \
    unsigned long long: &comp_ullong, \
    _Bool: &comp_bool, \
    float: &comp_float, \
    double: &comp_double, \
    char: &comp_char, \
    char*: &comp_str, \
    const char*: &comp_cstr, \
    default: NULL)

static inline int comp_str_data(char *a, char *b) {
  return strcmp(a, b);
}

static inline int comp_cstr_data(const char *a, const char *b) {
  return strcmp(a, b);
}

#define DEFAULT_COMP_TYPE(x)                                                   \
  _Generic((x),	\
    char*: &comp_str_data, \
    const char*: &comp_cstr_data, \
    default: NULL)

#endif
