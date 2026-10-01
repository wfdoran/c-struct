#include <stdio.h>
#include <stdint.h>

#define START_TEST(a)				\
  int a(void) {					\
    {

#define END_TEST				\
  }						\
  printf("passed %s\n", __func__);		\
  return 0;					\
}

#define CHECK(a)				\
  do {						\
  if (!(a)) {					\
  printf("failed %s: %s\n", __func__, #a);	\
  return 1;					\
  } \
  } while(0)


static uint32_t _rng_state = 0;

// https://en.wikipedia.org/wiki/Linear_congruential_generator
// The low bits of an LCG have short periods, so each 16 bit half of the
// result comes from the high bits of the state.
static inline uint32_t get_rand(void) {
  const uint32_t a = 1664525;
  const uint32_t c = 1013904223;

  _rng_state = (a * _rng_state) + c;
  uint32_t hi = _rng_state >> 16;

  _rng_state = (a * _rng_state) + c;
  uint32_t lo = _rng_state >> 16;

  return (hi << 16) | lo;
}

static inline void seed_rand(uint32_t seed) {
  _rng_state = seed;
}

