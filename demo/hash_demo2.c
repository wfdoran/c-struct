#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

typedef struct {
  int32_t x[3];
} triple_t;

#define key_t triple_t
#define value_t bool
#define prefix trip
#include <hash_table.h>
#undef prefix
#undef value_t
#undef key_t

uint64_t triple_hash(triple_t t) {
  uint64_t rv = UINT64_C(0x5555555555555555);
  uint64_t mult = UINT64_C(6364136223846793005);
  for (int i = 0; i < 3; i++) {
    rv += (uint64_t) t.x[i];
    rv *= mult;
  }
  return rv;
}

int main() {
  htable_trip_t *h = hash_trip_init(0);
  hash_trip_set_hash(h, &triple_hash);


  int32_t n = 100;

  for (int32_t a = 1; a < n; a++) {
    for (int32_t b = a + 1; b < n; b++) {
      for (int32_t c = b + 1; c < n; c++) {
	if (a * a + b * b == c * c) {
	  triple_t t = {.x[0] = a, .x[1] = b, .x[2] = c};
	  hash_trip_put(h, t, true);
	}
      }
    }
  }

  hiter_trip_t *iter;
  triple_t t;
  for (hash_trip_first(h, &iter, &t, NULL); iter != NULL; hash_trip_next(&iter, &t, NULL)) {
    printf("%8d %8d %8d\n", t.x[0], t.x[1], t.x[2]);
  }

  hash_trip_destroy(&h);
  return 0;
}


