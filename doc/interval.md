# Interval Arithmetic

Arithmetic on intervals of `double`, with directed rounding, so that the
result of an operation is guaranteed to contain the exact answer for every
number in the argument intervals.  Rounding errors are never hidden: they just
make the interval wider.

```c
#include <interval.h>

interval_t a = interval_from_double(4.0);
interval_t b = interval_from_double(2.1);
interval_print(interval_div(a, b));       // a small interval around 1.9047...
```

Compile with `-frounding-math` and link with `-lm`:

```
gcc -O2 -frounding-math -I include prog.c -o prog -lm
```

Every function is `static inline`, so the header can be included from as many
`.c` files as you like.  Including it turns on `#pragma STDC FENV_ACCESS` for
clang in the rest of the translation unit.

## How it works

Every operation computes the upper bound with the floating point rounding
mode set to round up, and the lower bound with it set to round down.  The
compiler must not treat the two computations as the same, move them across the
`fesetround()` calls, or share them, so each result goes through a `volatile`
object (`iv_fence`).  `-frounding-math` is passed as an additional precaution.
Call `interval_rounding_is_honored()` once at startup: it returns false if the
compiler ignored the rounding mode, and the demos then refuse to run.

The math library functions (`exp`, `log`, `sin`, `cos`, `erf`) are not
guaranteed to honor the rounding mode or to be correctly rounded, so their
results are widened by one unit in the last place.

## The type

```c
typedef struct {
  double lo;
  double hi;
  bool valid;
} interval_t;
```

`valid` is `false` for the result of an operation which has no result.  Every
function returns an invalid interval when any argument is invalid, so
invalidity propagates, and `lo` and `hi` of an invalid interval must be
ignored.

## Which results are invalid

For a function with a restricted domain, the domain is taken to be the
argument interval intersected with the valid domain of the function.  The
result is valid if that is not empty, and invalid only if no part of the
argument is in the domain.  So `interval_sqrt` of `[-1, 4]` is the interval for
`sqrt` of `[0, 4]`, and `interval_sqrt` of `[-4, -1]` is invalid.

`interval_div` is the exception: it is invalid whenever the divisor contains
0 (see question 7 in `QUESTIONS.md`).  Operations which would produce a NaN
(for example `0 * infinity` in `interval_mul`) are also invalid.

## Creating and printing

### `interval_t interval_from_double(double x)`

An interval which contains `x`: the numbers one unit in the last place below
and above it.  `x` is widened even if it is exactly representable, so
`interval_from_double(0.0)` is `[-4.9e-324, 4.9e-324]`.  Infinities give
half-infinite intervals, and a NaN gives an invalid interval.

### `interval_t interval_from_int64(int64_t x)`

An interval which contains the integer `x`.  The bounds are `x` rounded down
and up, so the interval is a single point when `x` is exactly representable
(below 2^53 in magnitude) and two adjacent doubles otherwise.

### `void interval_print(interval_t a)`

Prints `[lo, hi]` with 20 digits after the decimal point and a newline, or
`[invalid]`.

### `bool interval_rounding_is_honored(void)`

True if the compiler honors the rounding mode changes (see above).

## Arithmetic

### `interval_t interval_add(interval_t a, interval_t b)`
### `interval_t interval_sub(interval_t a, interval_t b)`
### `interval_t interval_mul(interval_t a, interval_t b)`

Sum, difference and product.  `interval_mul` is invalid if a product is a NaN
(`0 * infinity`).

### `interval_t interval_div(interval_t a, interval_t b)`

Quotient.  Invalid if `b` contains 0.

### `interval_t interval_fma(interval_t a, interval_t b, interval_t c)`

`a * b + c` with one rounding, which is narrower than `interval_add` of an
`interval_mul` when the product has a large cancellation with `c`.

### `interval_t interval_add_many(int n, interval_t *a)`

The sum of `n` intervals.  It always adds the two intervals of smallest
magnitude next, using a priority queue, which keeps the rounding error smaller
than adding in order.  Invalid if `n <= 0`, `a` is `NULL`, or any interval is
invalid.

### `interval_t interval_neg(interval_t a)`
### `interval_t interval_fabs(interval_t a)`

Negation and absolute value.  `interval_fabs` of an interval which contains 0
is `[0, max(-lo, hi)]`.

### `interval_t interval_fmin(interval_t a, interval_t b)`
### `interval_t interval_fmax(interval_t a, interval_t b)`

The smallest and the largest value of a pair of numbers taken from `a` and
`b`.

### `interval_t interval_floor(interval_t a)`
### `interval_t interval_ceil(interval_t a)`

`floor` and `ceil` of both ends.  The results are exact.

## Powers and roots

### `interval_t interval_sqrt(interval_t a)`

Square root.  The domain is `[0, inf)`: a negative `lo` is replaced by 0, and
the result is invalid only if `hi < 0`.

### `interval_t interval_pow(interval_t x, e)`

A macro which chooses on the type of `e`:

* `interval_pow_int(interval_t a, int32_t e)`: for an `int` exponent.  A
  negative exponent is the reciprocal of the positive power, so it is invalid
  when that power contains 0.
* `interval_pow_uint(interval_t a, uint32_t e)`: for a `uint32_t` exponent,
  by repeated squaring.
* `interval_pow_dbl(interval_t a, interval_t b)`: for an `interval_t`
  exponent, as `exp(b * log(a))`, so the domain of `log` applies: `a` must
  have a positive part.

## Exponential functions

### `interval_t interval_exp(interval_t a)`

`e` to the power `a`.  The lower bound is never below 0.

### `interval_t interval_log(interval_t a)`

Natural logarithm.  The domain is `(0, inf)`: if `lo <= 0` the lower bound is
`-inf`, and the result is invalid only if `hi <= 0`.

### `interval_t interval_erf(interval_t a)`

The error function.  The bounds are kept within `[-1, 1]`.

## Trigonometry

### `interval_t interval_sin(interval_t a)`
### `interval_t interval_cos(interval_t a)`

Sine and cosine.  The result accounts for the extrema (`+1` and `-1`) which lie
inside the interval, for negative arguments as well.  The result is always
within `[-1, 1]`.  An interval wider than 2 pi gives `[-1, 1]`.

## Not provided yet

`cbrt`, `fmod`, `log10`, `modf`, `tan`, `acos`, `asin`, `atan`, `cosh`, `sinh`.

## Demos

`demo/interval_demo1.c` to `interval_demo8.c`: arithmetic, `add_many`, powers,
`sqrt`, `sin` and `cos`, `fma`, the continued fraction of pi (which shows how
the interval grows until it can no longer tell the next term), and integer
conversion.  Build one with `cd demo && make interval_demo1`.
