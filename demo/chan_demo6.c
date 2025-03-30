#include <stdint.h>
#include <stdio.h>

#define data_t int64_t
#define prefix int64
#include <chan.h>
#undef prefix
#undef data_t

int main(void) {

  int32_t capacity = 5;
  chan_int64_t *ch = chan_int64_init(capacity);

  
  int64_t n = 1000;
  int64_t v = 1;
  int64_t temp;

  select_int64_t s[] = {
    {.c = ch, .select_type = SELECT_SEND, .value = &v},
    {.c = ch, .select_type = SELECT_RECV, .value = &temp},
  };
  int32_t num_select = sizeof(s) / sizeof(s[0]);

  int64_t sum = 0;
  bool done = false;
  while (!done) {
    switch(select_int64_one(num_select, s)) {
        case 0:
            if (v == n) {
                select_int64_option_done(s, 0);
            } else {
                v++;
            }    
            break;
        case 1:
            sum += temp * temp;            
            break;
        case 2:
            break;
        case SELECT_DONE:
            done = true;
            break;
        default:
            printf("Error\n");
    }
  }
  
  printf("%ld\n", sum);
  printf("%ld\n", (n * (n + 1) * (2 * n + 1)) / 6);
}
