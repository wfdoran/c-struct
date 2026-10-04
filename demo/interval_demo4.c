/* The square root of 2: squaring the result brings back an interval which contains 2. */

#include <stdio.h>
#include <interval.h>

int main(void) {
  if (!interval_rounding_is_honored()) {
    fprintf(stderr, "interval: rounding modes are being ignored; compile with -frounding-math\n");
    return 1;
  }

  interval_t a = interval_from_double(2.0);
  interval_t b = interval_sqrt(a);
  interval_t c = interval_mul(b,b);
  interval_t d = interval_sub(c, a);
  interval_t e = interval_fabs(d);
  interval_print(b);
  interval_print(c);
  interval_print(d);
  interval_print(e);
  printf("\n");

  return 0;
}
