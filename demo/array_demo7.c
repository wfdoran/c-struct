#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define data_t int16_t
#define prefix int16
#include <array.h>
#undef prefix
#undef data_t

int16_t add(int16_t a, int16_t b) {
    return a + b;
}

int16_t sqr(int16_t a) {
    return a * a;
}

int main(void) {
    array_int16_t a;
    array_int16_init(&a);
 
    for (int16_t i = 1; i <= 10; i++) {
        array_int16_append(&a, i);
    }
    array_int16_map(&a, sqr);
    printf("%d\n", array_int16_fold(&a, add));
}
