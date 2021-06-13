#ifndef _INTERNAL_H
#define _INTERNAL_H

#include "../gl/glcontext.h"

#define FILE_TYPE_UNDEF 0

typedef struct{
    gl_context ctx;
    
}_GLVContext;


typedef struct{

    int width;
    int height;
    const char *filename;
    int type;

}_GLVFile;

#endif