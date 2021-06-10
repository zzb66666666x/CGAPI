#ifndef _INTERNAL_H
#define _INTERNAL_H

#include "../gl/glcontext.h"

typedef struct{
    gl_context ctx;
    
}_GLVContext;


typedef struct{

    int width;
    int height;
    char *filename;
    int type;

}_GLVFile;

#endif