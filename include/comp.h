#ifndef COMP_H
#define COMP_H

#include <stdint.h>

int comp_int64(int64_t *a, int64_t *b) {
    if (*a > *b) { 
        return 1;
    }
    if (*a < *b) {
        return -1;
    }
    return 0;    
}    

int comp_int(int *a, int *b) {
    if (*a > *b) { 
        return 1;
    }
    if (*a < *b) {
        return -1;
    }
    return 0;
}

int comp_float(float *a, float *b) {
    if (*a > *b) {
        return 1;
    }
    if (*a < *b) {
        return -1;
    }
    return 0;
}

int comp_str(char **a, char **b) {
    return strcmp(*a, *b);
}

int comp_char(char *a, char *b) {
    if (*a > *b) {
        return 1;
    }
    if (*a < *b) {
        return -1;
    }
    return 0;
}

#define DEFAULT_COMP(x) _Generic((x), \
    int: &comp_int, \
    int64_t: &comp_int64, \
    float: &comp_float, \
    char: &comp_char, \
    char*: &comp_str, \
    default: NULL)
    
#endif