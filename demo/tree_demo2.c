#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define data_t int64_t
#define prefix int64
#include <tree.h>
#undef prefix
#undef data_t

int main(void) {
    srand(time(NULL));
    
    tree_int64_t t;
    tree_int64_init(&t);
    
    for (int i = 0; i < 100000; i++) {
        int64_t val = 0;
        for (int j = 0; j < 5; j++) {
            val = (val << 13) + rand();
        }
        tree_int64_insert(&t, val);
    }
    printf("Tree Size: %d\n", tree_int64_size(&t));
    printf("Tree Height: %d\n", tree_int64_height(&t));
    printf("\n");
    
    return 0;
}