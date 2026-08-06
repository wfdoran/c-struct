#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

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

  for (int i = 0; s[i] != 0; i++) {
    rv += (uint64_t) s[i];
    rv *= mult;
  }
  return rv;
}

/* ToDo: clean with -0.0, NaN, and inf better */
static inline uint64_t hash_double(double x) {
  uint64_t bits = 0;
  memcpy(&bits, &x, sizeof(double));
  return bits;
}

static inline uint64_t hash_float(float x) {
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
    char*: &hash_str, \
    const char*: &hash_str, \
    default: NULL)

#undef GENERIC_HASH
#endif

