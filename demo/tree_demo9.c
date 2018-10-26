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
    
    for (int i = 0; i < 10; i++) {
        int32_t val = (rand() >> 3) & 0xffff;
        tree_int32_insert(t, val, NULL);
    }
    
    for (size_t i = 0; i < tree_int32_size(t); i++) {
        key_int32_value_t pair = tree_int32_get_rank(t, i);
        printf("%4d %4d\n", i, pair.key);
    }
    
    tree_int32_destroy(&t);        
    return 0;
}