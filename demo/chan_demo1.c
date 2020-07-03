#include <stdio.h>


#define data_t int
#define prefix int
#include <chan.h>
#undef prefix
#undef data_t

int main(void) {
  chan_int_t *c = chan_int_init(10);

  for (int i = 0; i < 5; i++) {
    chan_int_send(c, i);
  }
  chan_int_close(c);
  

  int value;
  while (chan_int_recv(c, &value) == CHAN_SUCCESS) {
    printf("%d\n", value);
  }
    
  chan_int_destroy(&c);

  return 0;
}
