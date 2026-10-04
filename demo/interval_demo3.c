/* Integer and interval powers. */

#include <stdio.h>
#include <interval.h>

int main(void) {
  if (!interval_rounding_is_honored()) {
    fprintf(stderr, "interval: rounding modes are being ignored; compile with "
                    "-frounding-math\n");
    return 1;
  }

  interval_t x = interval_from_double(1.7);
  interval_t y = interval_pow(x, 10);
  interval_t z = interval_from_double(10.0);
  interval_t w = interval_pow(x, z);
  interval_print(y);
  interval_print(w);
  printf("\n");

  return 0;
}
