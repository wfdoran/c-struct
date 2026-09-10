#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define data_t int32_t
#define prefix int32
#include <pqueue.h>
#undef prefix
#undef data_t

int main(void) {
    srand(time(NULL));
    int32_t num_items = 20;

    pqueue_int32_t *q = pqueue_int32_init();

    printf("pushing: ");
    for (int i = 0; i < num_items; i++) {
        int32_t val = (rand() >> 3) & 0xff;
        printf("%d ", val);
        pqueue_int32_push(q, val, NULL);
    }
    printf("\n");

    printf("size = %zu\n", pqueue_int32_size(q));

    printf("popping (largest first): ");
    while (!pqueue_int32_is_empty(q)) {
        pqkv_int32_t top = pqueue_int32_pop(q);
        printf("%d ", top.key);
    }
    printf("\n");

    pqueue_int32_destroy(&q);

    return 0;
}
