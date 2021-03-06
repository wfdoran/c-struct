#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int x;
    float y;
} pair_t;

#define data_t char*
#define prefix str
#include <array.h>
#undef prefix
#undef data_t

    

int main(void) {
    srand(time(NULL));
    
    array_str_t *a = array_str_init();

    array_str_append(a, "hello");
    array_str_append(a, "world");
    array_str_append(a, "this");
    array_str_append(a, "is");
    array_str_append(a, "a");
    array_str_append(a, "test");
        
    array_str_sort(a);
    
    for (int i = 0; i < array_str_size(a); i++) {
        printf("%s\n", array_str_get(a, i));
    }
    
    printf("%ld %ld\n", array_str_bisect(a, "hello"), array_str_bisect(a, "goodbye"));

    array_str_destroy(&a);
    return 0;
}
   
