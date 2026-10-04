/* Sine and cosine of intervals which are multiples of pi/4, including ones
 * which contain an extremum. */

#include <stdio.h>
#include <interval.h>

int main(void) {
  if (!interval_rounding_is_honored()) {
    fprintf(stderr, "interval: rounding modes are being ignored; compile with "
                    "-frounding-math\n");
    return 1;
  }

  interval_t a = interval_from_double(M_PI_4);

  for (int32_t i = 0; i < 10; i++) {
    interval_t m = interval_from_double((double) i);
    interval_t b = interval_mul(a, m);
    interval_print(interval_sin(b));
  }
  printf("\n");

  for (int32_t i = 0; i < 10; i++) {
    interval_t m = interval_from_double((double) i);
    interval_t b = interval_mul(a, m);
    interval_print(interval_cos(b));
  }
  printf("\n");

  return 0;
}
