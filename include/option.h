#include <stdbool.h>

#ifndef data_t
#error "data_t not defined"
#endif

#ifndef prefix
#error "prefix not defined"
#endif

#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x,y)
#define GLUE3(x, y, z) GLUE(GLUE(x,y), z)

#define TYPE GLUE3(option_, prefix, _t)

typedef struct {
  bool set;
  data_t value;
} TYPE;

/* Initializers */

TYPE GLUE3(option_, prefix, _init) (data_t value) {
  TYPE rv = {.set = true, .value = value};
  return rv;
}

TYPE GLUE3(option_, prefix, _init_empty) (void) {
#ifdef sentinel_value
  TYPE rv = {.set = false, .value = sentinel_value};
#else
  TYPE rv = {.set = false};
#endif
  return rv;
}

TYPE GLUE3(option_, prefix, _deep_clone) (TYPE x, data_t (*f) (data_t)) {
  if (x.set && f != NULL) {
    TYPE rv = {.set = true, .value = f(x.value)};
    return rv;
  } else {
    return x;
  }
}

/* setters */

void GLUE3(option_, prefix, _set)(TYPE *x, data_t value) {
  x->value = value;
  x->set = true;
}


// Note: option_prefix_is_set(x) == option_prefix_get(x, NULL)
bool GLUE3(option_, prefix, _is_set) (TYPE x) {
  return x.set;
}

/* getters */

bool GLUE3(option_, prefix, _get)(TYPE x, data_t *value) {
  if (x.set) {
    if (value != NULL) {
      *value = x.value;
    }
    return true;
  }
  return false;
}

data_t GLUE3(option_, prefix, _force_get) (TYPE x) {
#ifdef sentinel_value
  if (!x.set) {
    return sentinel_value;
  }
#endif
  return x.value;
}

data_t GLUE3(option_, prefix, _get_or_else)(TYPE x, data_t other) {
  if (x.set) {
    return x.value;
  } else {
    return other;
  }
}

bool GLUE3(option_, prefix, _get_clear)(TYPE *x, data_t *value) {
  if (x->set) {
    if (value != NULL) {
      *value = x->value;
    }
    x->set = false;
    return true;
  }
  return false;
}

/* modifiers */

void GLUE3(option_, prefix, _map)(TYPE *x, data_t (*f) (data_t)) {
  if (x->set) {
    x->value = f(x->value);
  }
}


