#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>


typedef struct {
	int x[3];
} triple_t;

int comp(triple_t *a, triple_t *b) {
  for (int i = 0; i < 3; i++) {
    if (a->x[i] < b->x[i]) {
      return -1;
    }
    if (a->x[i] > b->x[i]) {
      return 1;
    }
  }
  return 0;
}

#define data_t triple_t
#define prefix trip
#include <tree.h>
#undef prefix
#undef data_t

int main(void) {
    srand(time(NULL));
    
    tree_trip_t *t = tree_trip_init();
    tree_trip_set_comp(t, comp);
    tree_trip_set_value_free(t, free);
	
    int n = 10;
    for (int i = 0; i < n; i++) {
      triple_t key = {.x[0] = rand(), .x[1] = rand(), .x[2] = rand()};
      double *value = malloc(sizeof(double));
      *value = (double) rand() / (1 << 15);
      tree_trip_insert(t, key, value);
    }
    
    void *state;
    tree_trip_walk_init(t, &state);
    while (state != NULL) {
      key_trip_value_t q = tree_trip_walk_next(&state);
      printf("%12d %12d %12d  %12.4f \n", q.key.x[0], q.key.x[1], q.key.x[2], *(double*)q.value);
    }	
    
    tree_trip_destroy(&t);
    return 0;
}
