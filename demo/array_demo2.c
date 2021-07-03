#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int x;
    float y;
} pair_t;

#define data_t pair_t
#define prefix pair
#include <array.h>
#undef prefix
#undef data_t

int pair_comp(pair_t *a, pair_t *b) {
    if (a->x > b->x) {
        return 1;
    }
    if (a->x < b->x) {
        return -1;
    }
    if (a->y > b->y) {
        return 1;
    }
    if (a->y < b->y) {
        return -1;
    }
    return 0;
}
    

int main(void) {
    srand(time(NULL));
    
    array_pair_t *a = array_pair_init();
    array_pair_set_comp(a, &pair_comp);
    
    for (int i = 0; i < 10; i++) {
        pair_t p = {.x = rand() % 10, .y = (rand() % 10) / 10.0};
        array_pair_append(a, p);
    }
    
    array_pair_sort(a);
    
    for (int i = 0; i < array_pair_size(a); i++) {
        pair_t p = array_pair_get(a, i);
        printf("%2d %4.2f\n", p.x, p.y);
    }
    
    array_pair_destroy(&a);
    return 0;
}
   
