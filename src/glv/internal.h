#ifndef _INTERNAL_H
#define _INTERNAL_H

#include "../gl/glcontext.h"
#include <stdlib.h>

#define FILE_TYPE_BMP 0

// container of files
#define MAX_FILENAME_LEN 128


/**
 * macro function
 */
#define MALLOC(T,LEN) ((T*)malloc(sizeof(T) * (LEN)))
#define FREE(p) do{                 \
                    free(p);        \
                    p = NULL;       \
                }while(0)           \


/**
 * glv type
 */ 

typedef struct{

    int width;
    int height;
    char filename[MAX_FILENAME_LEN];
    int type;

}_GLVFile;

typedef struct{
    gl_context ctx;
    _GLVFile *curFile;
}_GLVContext;

_GLVContext *_glvContext = nullptr;


#endif