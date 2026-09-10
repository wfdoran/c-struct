#include <stdlib.h>
#include <stdio.h>

#define data_t double
#define prefix double
#include <pqueue.h>
#undef prefix
#undef data_t

/* k-way merge of several already sorted arrays using a priority
   queue.  Each entry pushed onto the queue is a small heap-allocated
   record tracking which source array a value came from and its
   index within that array, so that once a value is popped we can
   push the next value from the same source. */

typedef struct {
    int source;
    size_t index;
} source_pos_t;

int main(void) {
    double a[] = { 1.0, 4.0, 7.0, 10.0 };
    double b[] = { 2.0, 3.0, 11.0 };
    double c[] = { 0.5, 5.5, 6.0, 6.5, 20.0 };

    double *streams[3] = { a, b, c };
    size_t lengths[3] = { 4, 3, 5 };

    /* min-heap: smallest value first */
    pqueue_double_t *q = pqueue_double_init();
    pqueue_double_set_value_free(q, free);

    for (int s = 0; s < 3; s++) {
        source_pos_t *pos = malloc(sizeof(source_pos_t));
        pos->source = s;
        pos->index = 0;
        double neg_key = -streams[s][0];
        pqueue_double_push(q, neg_key, pos);
    }

    printf("merged order: ");
    while (!pqueue_double_is_empty(q)) {
        pqkv_double_t top = pqueue_double_pop(q);
        source_pos_t *pos = top.value;
        double value = streams[pos->source][pos->index];
        printf("%.1f ", value);

        pos->index++;
        if (pos->index < lengths[pos->source]) {
            double neg_key = -streams[pos->source][pos->index];
            pqueue_double_push(q, neg_key, pos);
        } else {
            free(pos);
        }
    }
    printf("\n");

    pqueue_double_destroy(&q);

    return 0;
}
