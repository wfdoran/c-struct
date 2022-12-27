#include <stdint.h>
#include <stdio.h>

#define data_t int64_t
#define prefix int64

#include <array.h>

#undef data_t
#undef prefix

#define data_t float
#define prefix float

#include <array.h>

#undef data_t
#undef prefix

#define data_t void*
#define prefix void

#include <array.h>

#undef data_t
#undef prefix

int main(void) {
  array_int64_t *a = array_int64_init();
    array_int64_append(a, 5);
    array_int64_append(a, 3);
    array_int64_append(a, 1);
    array_int64_append(a, 2);
    
    array_float_t *b = array_float_init();
    array_float_append(b, 2.0);
    array_float_append(b, 3.5);
    
    array_void_t *c = array_void_init();

    printf("%.4f %ld\n", array_float_get(b,1), array_int64_get(a,1));
    
    array_int64_t *d = array_int64_clone(a);

    array_int64_sort(a);
    
    for (int i = 0; i < array_int64_size(d); i++) {
        printf("%ld ", array_int64_get(d,i));
    }
    printf("\n");
    for (int i = 0; i < array_int64_size(a); i++) {
        printf("%ld ", array_int64_get(a,i));
    }
    printf("\n");

    printf("index of 3 = %zu\n", array_int64_index(a, 3));
    printf("index of 3 = %zu\n", array_int64_bisect(a, 3));

    array_int64_destroy(&a);
    array_float_destroy(&b);
    array_void_destroy(&c);
    array_int64_destroy(&d);
    return 0;
}    
