#include <stddef.h>
#include <assert.h>
#include <string.h>

#pragma once

#define DecDynArr(T) \
    typedef struct{ \
        T *ptr; \
        size_t len; \
        size_t cap; \
    }ArrOf ## T;

#define apush(a, v) \
    (dyn_arr_make_space( \
        ((void**)(&(a)->ptr)), \
        &(a)->cap, \
        &(a)->len, \
        sizeof((a)->ptr[0]) \
    ), (a)->ptr[(a)->len - 1] = (v), &(a)->ptr[(a)->len -1])

#define apush_ptr(a, v) \
    (dyn_arr_make_space( \
        ((void**)(&(a)->ptr)), \
        &(a)->cap, \
        &(a)->len, \
        sizeof(*(v)) \
    ), ((a)->ptr)[(a)->len - 1] = *(v), &(a)->ptr[(a)->len -1])

#define aget(a, i) \
    (assert((i) < (a)->len && "out of bounds"), ((a)->ptr)[i])
#define aget_ptr(a, i) \
    (assert((i) < (a)->len && "out of bounds"), &((a)->ptr)[i])

#define alast(a) \
    (assert((a)->len && "out of bounds"), ((a)->ptr)[(a)->len - 1])
#define alast_ptr(a) \
    (assert((a)->len && "out of bounds"), &((a)->ptr)[(a)->len - 1])

#define arev(a) \
    dyn_arr_rev((a)->ptr, (a)->len, sizeof((a)->ptr[0]))

#define aremove(a, i) \
    (memmove( \
        &(a)->ptr[i], \
        &(a)->ptr[(i)+1], \
        sizeof((a)->ptr[0])*((a)->len-1-(i)) \
    ), ((a)->len)--)

#define dynarr_free(a) (free((a)->ptr), memset(a, 0, sizeof(*(a))))

void dyn_arr_make_space(
    void **arr_ptr, size_t *cap, size_t *len,
    size_t data_size
);

void dyn_arr_rev(
    void *arr_ptr, size_t len,
    size_t data_size
);
