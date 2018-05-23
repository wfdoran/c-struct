#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define data_t int32_t
#define prefix int32
#include <array.h>
#undef prefix
#undef data_t

    

int main(void) {
    srand(time(NULL));
    
    array_int32_t a;
    array_int32_init(&a);
    
    for (int i = 0; i < 10; i++) {
        array_int32_heappush(&a, rand() % 100);
    }
           
    
    while (array_int32_size(&a) > 0) {
        printf("%d\n", array_int32_heappop(&a));
    }
        
    return 0;
}
   