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

#define DEFAULT_COMP(x) _Generic((x), \
    int: &comp_int, \
    float: &comp_float, \
    default: NULL)
    
