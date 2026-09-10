#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Min-priority queue: reverse the default ordering so that
   pqueue_int_pop() returns the smallest key (lowest priority number)
   first. */
static int min_first_comp(int32_t *a, int32_t *b) {
    if (*a < *b) {
        return 1;
    }
    if (*a > *b) {
        return -1;
    }
    return 0;
}

#define data_t int32_t
#define prefix int32
#include <pqueue.h>
#undef prefix
#undef data_t

int main(void) {
    pqueue_int32_t *q = pqueue_int32_init();
    pqueue_int32_set_comp(q, min_first_comp);
    pqueue_int32_set_value_free(q, free);

    pqueue_int32_push(q, 3, strdup("write report"));
    pqueue_int32_push(q, 1, strdup("put out fire"));
    pqueue_int32_push(q, 5, strdup("water plants"));
    pqueue_int32_push(q, 2, strdup("answer email"));
    pqueue_int32_push(q, 1, strdup("call ambulance"));

    printf("processing tasks in priority order (1 = most urgent):\n");
    while (!pqueue_int32_is_empty(q)) {
        pqkv_int32_t task = pqueue_int32_pop(q);
        printf("  [priority %d] %s\n", task.key, (char *) task.value);
        free(task.value);
    }

    pqueue_int32_destroy(&q);

    return 0;
}
