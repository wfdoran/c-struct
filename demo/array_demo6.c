#include <stdint.h>
#include <stdio.h>

typedef struct {
	int x[5];
} intarr_t;

#define data_t intarr_t
#define prefix intarr

#include <array.h>

#undef data_t
#undef prefix


int main(void) {
    array_intarr_t a;
    array_intarr_init(&a);
    
    intarr_t b = {.x = {1,2,3,4,5}};
    array_intarr_append(&a, (intarr_t){.x = {1,2,3,4,5}});
    return 0;
}    