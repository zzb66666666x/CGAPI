#ifndef _SYMBOLS_H
#define _SYMBOLS_H

#ifdef __cplusplus
extern "C"{
#endif

#include "parse.h"

typedef struct{
    int io;
    int dtype;
    int layout;
}io_attrib;

extern void emplace_profile(const char* name, int io, int dtype, int layout);
extern buffer_t code_for_input();
extern buffer_t code_for_output();
extern buffer_t code_for_uniform();
extern void* get_profile();
extern void clear_profile();
extern void print_profile();

#ifdef __cplusplus
}
#endif

#endif