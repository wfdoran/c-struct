#include <comp.h>
#include <stdio.h>

typedef int data_t;

int main(void) {
    data_t temp;
    int (*comp) (data_t a, data_t b) = DEFAULT_COMP(temp);
    
    printf("%p\n", comp);
    printf("%p\n", &comp_int);
    printf("%p\n", &comp_float);
    
    return 0;
}