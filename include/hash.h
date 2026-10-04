#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* Finalizer (splitmix64) applied by the hash tables to every hash value, so
   that identity-like hashes (integers, doubles) still spread over the low bits.
 */
static inline uint64_t hash_mix64(uint64_t x) {
  x ^= x >> 30;
  x *= UINT64_C(0xbf58476d1ce4e5b9);
  x ^= x >> 27;
  x *= UINT64_C(0x94d049bb133111eb);
  x ^= x >> 31;
  return x;
}

/* Hash functions for the integer types, written over the standard types for the
   same reason as the comparators in comp.h; the fixed width names are kept for
   callers which use them directly. */
#define HASH_DEFINE(name, T)                                                   \
  static inline uint64_t hash_##name(T x) {                                    \
    return (uint64_t) x;                                                       \
  }

HASH_DEFINE(schar, signed char)
HASH_DEFINE(short, short)
HASH_DEFINE(int, int)
HASH_DEFINE(long, long)
HASH_DEFINE(llong, long long)
HASH_DEFINE(uchar, unsigned char)
HASH_DEFINE(ushort, unsigned short)
HASH_DEFINE(uint, unsigned int)
HASH_DEFINE(ulong, unsigned long)
HASH_DEFINE(ullong, unsigned long long)
HASH_DEFINE(bool, _Bool)
HASH_DEFINE(char, char)

HASH_DEFINE(int64_t, int64_t)
HASH_DEFINE(int32_t, int32_t)
HASH_DEFINE(int16_t, int16_t)
HASH_DEFINE(int8_t, int8_t)
HASH_DEFINE(uint64_t, uint64_t)
HASH_DEFINE(uint32_t, uint32_t)
HASH_DEFINE(uint16_t, uint16_t)
HASH_DEFINE(uint8_t, uint8_t)

#undef HASH_DEFINE

static inline uint64_t hash_str(const char *s) {
  uint64_t rv = UINT64_C(0x5555555555555555);
  uint64_t mult = UINT64_C(6364136223846793005);

  for (; *s != 0; s++) {
    rv += (uint64_t) *s;
    rv *= mult;
  }
  return rv;
}

/* The same for a key type of char* (not const char*): the hash tables store a
   function pointer whose parameter has the key's exact type, and a function
   taking const char* is not a compatible pointer type. */
static inline uint64_t hash_str_mutable(char *s) {
  return hash_str(s);
}

/* Equal keys must hash equally: fold -0.0 into +0.0 and give every NaN the same
 * hash. */
static inline uint64_t hash_double(double x) {
  if (x == 0.0) {
    return 0;
  }
  if (x != x) {
    return UINT64_C(0x7ff8000000000000);
  }
  uint64_t bits = 0;
  memcpy(&bits, &x, sizeof(double));
  return bits;
}

static inline uint64_t hash_float(float x) {
  if (x == 0.0f) {
    return 0;
  }
  if (x != x) {
    return UINT64_C(0x7fc00000);
  }
  uint64_t bits = 0;
  memcpy(&bits, &x, sizeof(float));
  return bits;
}

#define DEFAULT_HASH(x)                                                        \
  _Generic((x),		\
    signed char: &hash_schar, \
    short: &hash_short, \
    int: &hash_int, \
    long: &hash_long, \
    long long: &hash_llong, \
    unsigned char: &hash_uchar, \
    unsigned short: &hash_ushort, \
    unsigned int: &hash_uint, \
    unsigned long: &hash_ulong, \
    unsigned long long: &hash_ullong, \
    _Bool: &hash_bool, \
    char: &hash_char, \
    double: &hash_double, \
    float: &hash_float, \
    char*: &hash_str_mutable, \
    const char*: &hash_str, \
    default: NULL)

#endif
