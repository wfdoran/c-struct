#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

#ifndef data_t
#error "data_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif



#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x,y)
#define GLUE3(x, y, z) GLUE(GLUE(x,y), z)

#define CHAN GLUE3(chan_, prefix, _t)

typedef struct CHAN {
    data_t *data;
    int64_t capacity;
    int64_t occupancy;
    int64_t read_pos;
    int64_t write_pos;
    bool closed;
    pthread_rwlock_t rwlock;
} CHAN;

/* 
    chan_prefix_t *chan_prefix_init(int64_t capacity);
    void chan_prefix_destroy(chan_prefix_t **c);
    int32_t chan_prefix_send(chan_prefix_t *c, data_t value);
    int32_t chan_prefix_recv(chan_prefix_t *c, data_t *value);
    int32_t chan_close(chan_prefix_t *c);
*/
    
  
CHAN *GLUE3(chan_, prefix, _init) (int64_t capacity) {
  CHAN *c = malloc(sizeof(CHAN));
  if (c == NULL) {
    return NULL;
  }
  c->capacity = capacity > 0 ? capacity : 1;
  c->occupancy = 0;
  c->read_pos = 0;
  c->write_pos = 0;
  c->closed = false;

  c->data = malloc(c->capacity * sizeof(data_t));
  if (c->data == NULL) {
    return NULL;
  }

  pthread_rwlock_init(&(c->rwlock), NULL);

  return c;
}



  

#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
