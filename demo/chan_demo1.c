#include <stdio.h>


#define data_t int
#define prefix int
#include <chan.h>
#undef prefix
#undef data_t

int main(void) {
  chan_int_t *c = chan_int_init(10);

  for (int32_t i = 0; ; i++) {
    int32_t rc = chan_int_trysend(c, i);
    if (rc != CHAN_SUCCESS) {
      break;
    }
    printf("Sending %d\n", i);
  }
  
  while (true) {
    int value = -1;
    int32_t rc = chan_int_tryrecv(c, &value);
    if (rc != CHAN_SUCCESS) {
      break;
    }
    printf("Receiving %d\n", value);
  }
    
  chan_int_destroy(&c);

  return 0;
}
