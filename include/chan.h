#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef data_t
#error "data_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif


#ifndef CHAN_CONSTS
#define CHAN_CONSTS

#define CHAN_SUCCESS (0)
#define CHAN_CLOSED (1)

#define CHAN_ERROR (-1)
#define CHAN_FULL (2)
#define CHAN_EMPTY (3)
#endif



#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x,y)
#define GLUE3(x, y, z) GLUE(GLUE(x,y), z)

#define CHAN GLUE3(chan_, prefix, _t)

typedef struct CHAN {
    data_t *data;
    _Atomic int64_t capacity;
    _Atomic int64_t occupancy;
    int64_t read_pos;
    int64_t write_pos;
    atomic_bool closed;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} CHAN;
      
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

  pthread_mutex_init(&(c->lock), NULL);
  pthread_cond_init(&(c->cond), NULL);

  return c;
}

void GLUE3(chan_, prefix, _destroy) (CHAN **c_ptr) {
  CHAN *c = *c_ptr;
  if (c == NULL) {
    return;
  }

  pthread_mutex_lock(&(c->lock));
  pthread_mutex_unlock(&(c->lock));
  free(c->data);
  pthread_mutex_destroy(&(c->lock));
  pthread_cond_destroy(&(c->cond));
  free(c);
  c_ptr = NULL;
}

int32_t GLUE3(chan_, prefix, _send) (CHAN *c, data_t value) {
  if (c == NULL) {
    return CHAN_ERROR;
  }

  pthread_mutex_lock(&(c->lock));

  if (c->closed) {
    pthread_cond_broadcast(&(c->cond));
    pthread_mutex_unlock(&(c->lock));
    return CHAN_CLOSED;
  }

  while (c->occupancy == c->capacity) {
    pthread_cond_wait(&(c->cond), &(c->lock));

    if (c->closed) {
      pthread_cond_broadcast(&(c->cond));
      pthread_mutex_unlock(&(c->lock));
      return CHAN_CLOSED;
    }      
  }

  c->data[c->write_pos] = value;
  c->write_pos = (c->write_pos + 1) % c->capacity;
  c->occupancy++;
  pthread_cond_broadcast(&(c->cond));
  pthread_mutex_unlock(&(c->lock));
  return CHAN_SUCCESS;
}

int32_t GLUE3(chan_, prefix, _trysend) (CHAN *c, data_t value) {
  if (c == NULL) {
    return CHAN_ERROR;
  }

  pthread_mutex_lock(&(c->lock));

  if (c->closed) {
    pthread_mutex_unlock(&(c->lock));
    return CHAN_CLOSED;
  }

  int32_t rc;
  if (c->occupancy < c->capacity) {
      c->data[c->write_pos] = value;
      c->write_pos = (c->write_pos + 1) % c->capacity;
      c->occupancy++;
      rc = CHAN_SUCCESS;
  } else {
    rc = CHAN_FULL;
  }

  pthread_cond_broadcast(&(c->cond));
  pthread_mutex_unlock(&(c->lock));
  return rc;
}

int32_t GLUE3(chan_, prefix, _recv) (CHAN *c, data_t *value) {
  if (c == NULL) {
    return CHAN_ERROR;
  }

  pthread_mutex_lock(&(c->lock));

  while (c->occupancy == 0) {
    if (c->closed) {
      pthread_cond_broadcast(&(c->cond));
      pthread_mutex_unlock(&(c->lock));
      return CHAN_CLOSED;
    }
    pthread_cond_wait(&(c->cond), &(c->lock));
  }

  *value = c->data[c->read_pos];
  c->read_pos = (c->read_pos + 1) % c->capacity;
  c->occupancy--;
  pthread_cond_broadcast(&(c->cond));
  pthread_mutex_unlock(&(c->lock));
  return CHAN_SUCCESS;
}

int32_t GLUE3(chan_, prefix, _tryrecv) (CHAN *c, data_t *value) {
  if (c == NULL) {
    return CHAN_ERROR;
  }

  pthread_mutex_lock(&(c->lock));

  int32_t rc;
  if (c->occupancy > 0) {
    *value = c->data[c->read_pos];
    c->read_pos = (c->read_pos + 1) % c->capacity;
    c->occupancy--;
    rc = CHAN_SUCCESS;
    pthread_mutex_unlock(&(c->lock));
    return CHAN_SUCCESS;
  } else if (c->closed) {
    rc = CHAN_CLOSED;
  } else {
    rc = CHAN_EMPTY;
  }

  pthread_cond_broadcast(&(c->cond)); 
  pthread_mutex_unlock(&(c->lock));
  return rc;
}


int32_t GLUE3(chan_, prefix, _close) (CHAN *c) {
  if (c == NULL) {
    return CHAN_ERROR;
  }
  pthread_mutex_lock(&(c->lock));
  int32_t rc = c->closed ? CHAN_CLOSED : CHAN_SUCCESS;
  c->closed = true;
  
  pthread_cond_broadcast(&(c->cond)); 
  pthread_mutex_unlock(&(c->lock));
  return rc;
}

#undef GLUE3
#undef GLUE
#undef GLUE_HELPER
