typedef struct {
    int x;
    float y;
} pair_t;

#define data_t pair_t
#define prefix pair
#include <array.h>
#undef prefix
#undef data_t

int main(void) {
    array_pair_t a;
    array_pair_init(&a);

    pair_t p1 = {.x = 2, .y = 2.0};
    pair_t p2 = {.x = 3, .y = 4.0};
    
    array_pair_append(&a, p1);
    array_pair_append(&a, p2);
    
    return 0;
}
   