#ifndef COMP_H
#define COMP_H

#include <stdint.h>

#define GENERIC_COMP \
    if (*a > *b) { \
        return 1;\
    }\
    if (*a < *b) {\
        return -1;\
    }\
    return 0;    
    
int comp_int64(int64_t *a, int64_t *b) {
    GENERIC_COMP
}

int comp_int32(int32_t *a, int32_t *b) {
    GENERIC_COMP
}

int comp_float(float *a, float *b) {
    GENERIC_COMP
}


int comp_char(char *a, char *b) {
    GENERIC_COMP
}

#undef GENERIC_COMP

int comp_str(char **a, char **b) {
    return strcmp(*a, *b);
}

#define DEFAULT_COMP(x) _Generic((x), \
    int64_t: &comp_int64, \
    int32_t: &comp_int32, \
    float: &comp_float, \
    char: &comp_char, \
    char*: &comp_str, \
    default: NULL)
    

    
#endif