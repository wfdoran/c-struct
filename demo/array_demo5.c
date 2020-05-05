#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define data_t double
#define prefix double
#include <array.h>
#undef prefix
#undef data_t

    

int main(void) {
    srand(time(NULL));
    
    array_double_t *a = array_double_init();
    
    for (int i = 0; i < 10; i++) {
        array_double_append(a, ((double) (rand() % 0xfff)) / 0x100);
    }
    
    array_double_heapify(a);
    
    for (int i = 0; i < array_double_size(a); i++) {
        printf("%6.4f \n", array_double_get(a, i));        
    }

    array_double_destroy(&a);
    return 0;
}
   
