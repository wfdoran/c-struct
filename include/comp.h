#ifndef COMP_H
#define COMP_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define GENERIC_COMP \
    if (*a > *b) { \
        return 1;\
    }\
    if (*a < *b) {\
        return -1;\
    }\
    return 0;    
    
static int comp_int64(int64_t *a, int64_t *b) {
    GENERIC_COMP
}

static int comp_int32(int32_t *a, int32_t *b) {
    GENERIC_COMP
}

static int comp_int16(int16_t *a, int16_t *b) {
    GENERIC_COMP
}

static int comp_float(float *a, float *b) {
    GENERIC_COMP
}

static int comp_double(double *a, double *b) {
    GENERIC_COMP
}


static int comp_char(char *a, char *b) {
    GENERIC_COMP
}

#undef GENERIC_COMP

static int comp_str(char **a, char **b) {
    return strcmp(*a, *b);
}

#define DEFAULT_COMP(x) _Generic((x), \
    int64_t: &comp_int64, \
    int32_t: &comp_int32, \
    int16_t: &comp_int16, \
    float: &comp_float, \
    double: &comp_double, \
    char: &comp_char, \
    char*: &comp_str, \
    default: NULL)


static int comp_str_data(char *a, char *b) {
    return strcmp(a, b);
}


#define DEFAULT_COMP_TYPE(x) _Generic((x),	\
    char*: &comp_str_data, \
    default: NULL)
    
#endif
