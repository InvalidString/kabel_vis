#include <alloca.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dyn_arr.h"

void dyn_arr_rev(
    void *arr_ptr, size_t len,
    size_t data_size
){
    void* tmp = alloca(data_size);
    for (size_t i = 0; i < len/2; i++) {
        void* a = (char*)arr_ptr + i*data_size;
        void* b = (char*)arr_ptr + (len-1-i)*data_size;
        memcpy(tmp, a, data_size);
        memcpy(a, b, data_size);
        memcpy(b, tmp, data_size);
    }
}

void dyn_arr_make_space(
    void **arr_ptr, size_t *cap, size_t *len,
    size_t data_size
){

    (*len)++;
    if(*len >= *cap){
        size_t new_capacity = *cap * 2 + 2;
        *arr_ptr = realloc(*arr_ptr, new_capacity*data_size);
        if(!*arr_ptr){
            perror("dynarr allocation fail");
            exit(1);
        }
        *cap = new_capacity;
    }

}
