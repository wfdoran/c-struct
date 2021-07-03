#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define data_t int32_t
#define prefix int32
#include <tree.h>
#undef prefix
#undef data_t

int main(void) {
    srand(time(NULL));
    
    tree_int32_t *t = tree_int32_init();

    int32_t marker = 40000;
    tree_int32_insert(t, marker, NULL);
    for (int i = 0; i < 20; i++) {
        int32_t val = (rand() >> 3) & 0xffff;
        tree_int32_insert(t, val, NULL);
    }
    
    for (size_t i = 0; i < tree_int32_size(t); i++) {
        key_int32_value_t pair = tree_int32_get_rank(t, i);
        printf("%4zu %10d\n", i, pair.key);
    }

    printf("num less than %d = %zu\n", marker, tree_int32_num_less(t, marker));
    printf("num less than or equal to %d = %zu\n", marker, tree_int32_num_less_equal(t, marker));
    printf("num greater than %d = %zu\n", marker, tree_int32_num_greater(t, marker));
    printf("num greater than or equal to %d = %zu\n", marker, tree_int32_num_greater_equal(t, marker));
    
    tree_int32_destroy(&t);        
    return 0;
}
