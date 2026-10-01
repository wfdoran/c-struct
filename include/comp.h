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
    
static inline int comp_int64(int64_t *a, int64_t *b) {
    GENERIC_COMP
}

static inline int comp_int32(int32_t *a, int32_t *b) {
    GENERIC_COMP
}

static inline int comp_int16(int16_t *a, int16_t *b) {
    GENERIC_COMP
}

static inline int comp_int8(int8_t *a, int8_t *b) {
    GENERIC_COMP
}

static inline int comp_uint64(uint64_t *a, uint64_t *b) {
    GENERIC_COMP
}

static inline int comp_uint32(uint32_t *a, uint32_t *b) {
    GENERIC_COMP
}

static inline int comp_uint16(uint16_t *a, uint16_t *b) {
    GENERIC_COMP
}

static inline int comp_uint8(uint8_t *a, uint8_t *b) {
    GENERIC_COMP
}

static inline int comp_float(float *a, float *b) {
    GENERIC_COMP
}

static inline int comp_double(double *a, double *b) {
    GENERIC_COMP
}


static inline int comp_char(char *a, char *b) {
    GENERIC_COMP
}

#undef GENERIC_COMP

static inline int comp_str(char **a, char **b) {
    return strcmp(*a, *b);
}

static inline int comp_cstr(const char **a, const char **b) {
    return strcmp(*a, *b);
}

#define DEFAULT_COMP(x) _Generic((x), \
    int64_t: &comp_int64, \
    int32_t: &comp_int32, \
    int16_t: &comp_int16, \
    int8_t: &comp_int8, \
    uint64_t: &comp_uint64, \
    uint32_t: &comp_uint32, \
    uint16_t: &comp_uint16, \
    uint8_t: &comp_uint8, \
    float: &comp_float, \
    double: &comp_double, \
    char: &comp_char, \
    char*: &comp_str, \
    const char*: &comp_cstr, \
    default: NULL)


static inline int comp_str_data(char *a, char *b) {
    return strcmp(a, b);
}

static inline int comp_cstr_data(const char *a, const char *b) {
    return strcmp(a, b);
}


#define DEFAULT_COMP_TYPE(x) _Generic((x),	\
    char*: &comp_str_data, \
    const char*: &comp_cstr_data, \
    default: NULL)
    
#endif
