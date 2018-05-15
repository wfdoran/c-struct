#include <stdint.h>
#include <stdio.h>

#define data_t int64_t
#define prefix int64

#include "array.h"

#undef data_t
#undef prefix

#define data_t float
#define prefix float

#include "array.h"

#undef data_t
#undef prefix

int main(void) {
    array_int64_t a;
    array_int64_init(&a);
    array_int64_append(&a, 5);
    array_int64_append(&a, 3);
    array_int64_append(&a, 1);
    
    array_float_t b;
    array_float_init(&b);
    array_float_append(&b, 2.0);
    array_float_append(&b, 3.5);
    
    printf("%.4f %ld\n", array_float_get(&b,1), array_int64_get(&a,1));
    return 0;
}    