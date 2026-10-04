/* Fused multiply-add keeps more information than a multiply followed by an add. */

#include <stdio.h>
#include <interval.h>

int main(void) {
  if (!interval_rounding_is_honored()) {
    fprintf(stderr, "interval: rounding modes are being ignored; compile with -frounding-math\n");
    return 1;
  }

  interval_t a = interval_from_double(2000000000000000.0);
  interval_t b = interval_from_double(0.0000000000000005);
  interval_t c = interval_from_double(-1.0);

  interval_t x = interval_fma(a,b,c);
  interval_t y = interval_add(interval_mul(a,b), c);

  interval_print(x);
  interval_print(y);
  printf("\n");

  return 0;
}
