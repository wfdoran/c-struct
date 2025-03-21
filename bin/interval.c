#include <stdio.h>
#include <stdbool.h>
#include <fenv.h>
#include <math.h>
#include <float.h>

typedef struct {
  double lo;
  double hi;
  bool valid;
} interval_t;

interval_t
interval_from_double(double x) {
  interval_t rv = {.lo = nextafter(x, -DBL_MAX), .hi = nextafter(x,DBL_MAX), .valid = true};
  return rv;
}

interval_t
interval_add(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
        
  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = a.hi + b.hi;

  fesetround(FE_DOWNWARD);
  rv.lo = a.lo + b.lo;

  fesetround(save);

  rv.valid = true;
  return rv;
}

interval_t
interval_sub(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;
  
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = a.hi - b.lo;

  fesetround(FE_DOWNWARD);
  rv.lo = a.lo - b.hi;

  fesetround(save);

  rv.valid = true;
  return rv;
}

interval_t
interval_mul(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  
  double temp[4];

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  temp[0] = a.lo * b.lo;
  temp[1] = a.lo * b.hi;
  temp[2] = a.hi * b.lo;
  temp[3] = a.hi * b.hi;
  rv.hi = fmax(fmax(fmax(temp[0],temp[1]), temp[2]), temp[3]);

  fesetround(FE_DOWNWARD);
  temp[0] = a.lo * b.lo;
  temp[1] = a.lo * b.hi;
  temp[2] = a.hi * b.lo;
  temp[3] = a.hi * b.hi;
  rv.lo = fmin(fmin(fmin(temp[0],temp[1]), temp[2]), temp[3]);

  fesetround(save);

  rv.valid = true;
  return rv;  
}

interval_t
interval_div(interval_t a, interval_t b) {
  if (!a.valid || !b.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  if (b.lo <= 0.0 && b.hi >= 0.0) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  
  double temp[4];

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  temp[0] = a.lo / b.lo;
  temp[1] = a.lo / b.hi;
  temp[2] = a.hi / b.lo;
  temp[3] = a.hi / b.hi;
  rv.hi = fmax(fmax(fmax(temp[0],temp[1]), temp[2]), temp[3]);

  fesetround(FE_DOWNWARD);
  temp[0] = a.lo / b.lo;
  temp[1] = a.lo / b.hi;
  temp[2] = a.hi / b.lo;
  temp[3] = a.hi / b.hi;
  rv.lo = fmin(fmin(fmin(temp[0],temp[1]), temp[2]), temp[3]);

  fesetround(save);

  rv.valid = true;
  return rv;  
}


void
interval_print(interval_t a) {
  if (!a.valid) {
    printf("[invalid]\n");
  } else {
    printf("[%.20f, %.20f]\n", a.lo, a.hi);
  }
}

int main(void) {
  interval_t a = interval_from_double(4.0);
  interval_t b = interval_from_double(2.1);
  interval_t c = interval_add(a, b);
  interval_print(c);
  interval_t d = interval_sub(a,b);
  interval_print(d);
  interval_t e = interval_mul(a,b);
  interval_print(e);
  interval_t f = interval_div(a,b);
  interval_print(f);

  return 0;
}
