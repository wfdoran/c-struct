//  gcc interval.c -I ../include -o interval -lm

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

interval_t
interval_exp(interval_t a) {
  if (!a.valid) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = exp(a.hi);

  fesetround(FE_DOWNWARD);
  rv.lo = exp(a.lo);

  fesetround(save);

  rv.valid = true;
  return rv;
}

interval_t
interval_log(interval_t a) {
  if (!a.valid || a.lo <= 0.0) {
    interval_t bad = {.lo = 0.0, .hi = 0.0, .valid = false};
    return bad;
  }

  interval_t rv;
  int save = fegetround();

  fesetround(FE_UPWARD);
  rv.hi = log(a.hi);

  fesetround(FE_DOWNWARD);
  rv.lo = log(a.lo);

  fesetround(save);

  rv.valid = true;
  return rv;
}

  
    
interal_neg(interval_t a) {
  interval_t rv = {.lo = -a.hi, .hi = -a.lo, .valid = true};
  return rv;
}

#define data_t double
#define prefix ival
#include <tree.h>
#undef prefix
#undef data_t

static double 
interval_get_key(interval_t a) {
  return fabs(a.lo + (a.hi - a.lo) / 2.0);
}

void
interval_print(interval_t a) {
  if (!a.valid) {
    printf("[invalid]\n");
  } else {
    printf("[%.20f, %.20f]\n", a.lo, a.hi);
  }
}


interval_t
interval_add_many(int n, interval_t *a) {
  if (n <= 0 || a == NULL) {
    interval_t bad = {.lo = 0, .hi = 0, .valid = false};
    return bad;
  }
  
  for (int i = 0; i < n; i++) {
    if (!a[i].valid) {
      interval_t bad = {.lo = 0, .hi = 0, .valid = false};
      return bad;
    }
  }

  if (n == 1) {
    return a[0];
  }
  
  tree_ival_t *t = tree_ival_init();
  for (int i = 0; i < n; i++) {
    tree_ival_insert(t, interval_get_key(a[i]), &a[i]);
  }

  interval_t temp[n - 1];
  for (int i = 0; i < n-1; i++) {
    key_ival_value_t x = tree_ival_delete_min(t);
    key_ival_value_t y = tree_ival_delete_min(t);

    interval_t xx = *(interval_t*) x.value;
    interval_t yy = *(interval_t*) y.value;
    temp[i] = interval_add(xx, yy);

    tree_ival_insert(t, interval_get_key(temp[i]), &temp[i]);
  }
  
  tree_ival_destroy(&t);
  return temp[n - 2];
}


int main(void) {
  int n = 10;

  interval_t a[n];
  for (int i = 0; i < n; i++) {
    a[i] = interval_from_double(1.0 + i * i);
  }

  interval_t sum = interval_add_many(n, a);
  interval_print(sum);

  for (int i = 1; i < n; i++) {
    a[0] = interval_add(a[0], a[i]);
  }
  interval_print(a[0]);
}


#if(0)

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

  interval_t x = interval_from_double(10.0);
  interval_t y = interval_from_double(100.0);
  interval_t z = interval_from_double(1000.0);

  interval_t w1 = interval_add(x, interval_add(y, z));
  interval_t w2 = interval_add(interval_add(x, y), z);
  interval_t w3 = interval_add(interval_add(y, x), z);
  interval_print(w1);
  interval_print(w2);
  interval_print(w3);


  {
    interval_t g = interval_from_double(3.3838);
    for (int i = 0; i < 10; i++) {
      g = interval_log(interval_exp(g));
      interval_print(g);
    }
  }

  return 0;
}

#endif
