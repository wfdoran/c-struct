#include <stdio.h>
#include <stdint.h>

#define START_TEST(a)				\
  int a() {					\
    {

#define END_TEST				\
  }						\
  printf("%s passed\n", __func__);		\
  return 0;					\
}

#define CHECK(a)				\
  do {						\
  if (!(a)) {					\
  printf("%s failed: %s\n", __func__, #a);	\
  return 1;					\
  } \
  } while(0)


uint32_t _rng_state = 0;

// https://en.wikipedia.org/wiki/Linear_congruential_generator
uint32_t get_rand() {
  uint32_t a = 1664525;
  uint32_t c = 1013904223;

  _rng_state = (a * _rng_state) + c;
  uint32_t rv = (a >> 16);

  _rng_state = (a * _rng_state) + c;
  rv = (_rng_state & 0xffff) ^ rv;

  return rv;
}

void seed_rand(uint32_t seed) {
  _rng_state = seed;
}

