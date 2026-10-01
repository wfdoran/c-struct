#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* Finalizer (splitmix64) applied by the hash tables to every hash value, so
   that identity-like hashes (integers, doubles) still spread over the low bits. */
static inline uint64_t hash_mix64(uint64_t x) {
  x ^= x >> 30;
  x *= UINT64_C(0xbf58476d1ce4e5b9);
  x ^= x >> 27;
  x *= UINT64_C(0x94d049bb133111eb);
  x ^= x >> 31;
  return x;
}

static inline uint64_t hash_uint64_t(uint64_t x) {
  return x;
}

#define GENERIC_HASH(t)	    \
  static inline uint64_t hash_##t (t x) { \
    return (uint64_t) x;    \
  }

GENERIC_HASH(int64_t)
GENERIC_HASH(int32_t)
GENERIC_HASH(uint32_t)
GENERIC_HASH(int16_t)
GENERIC_HASH(uint16_t)
GENERIC_HASH(int8_t)
GENERIC_HASH(uint8_t)
GENERIC_HASH(char)


static inline uint64_t hash_str(const char *s) {
  uint64_t rv = UINT64_C(0x5555555555555555);
  uint64_t mult = UINT64_C(6364136223846793005);

  for (; *s != 0; s++) {
    rv += (uint64_t) *s;
    rv *= mult;
  }
  return rv;
}

/* The same for a key type of char* (not const char*): the hash tables store a function
   pointer whose parameter has the key's exact type, and a function taking
   const char* is not a compatible pointer type. */
static inline uint64_t hash_str_mutable(char *s) {
  return hash_str(s);
}

/* Equal keys must hash equally: fold -0.0 into +0.0 and give every NaN the same hash. */
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

#define DEFAULT_HASH(x) _Generic((x),		\
    uint64_t: &hash_uint64_t, \
    int64_t: &hash_int64_t, \
    uint32_t: &hash_uint32_t, \
    int32_t: &hash_int32_t, \
    uint16_t: &hash_uint16_t, \
    int16_t: &hash_int16_t, \
    uint8_t: &hash_uint8_t, \
    int8_t: &hash_int8_t,\
    char: &hash_char, \
    double: &hash_double, \
    float: &hash_float, \
    char*: &hash_str_mutable, \
    const char*: &hash_str, \
    default: NULL)

#undef GENERIC_HASH
#endif

