#ifndef _INTERNAL_H
#define _INTERNAL_H

#include "../gl/glcontext.h"
#include <stdlib.h>
#include <vector>

#define FILE_TYPE_BMP       0

// container of files
#define MAX_FILENAME_LEN    128
// container of ctxs
#define MAX_GLVFILE_NUM     3

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

}output_file_t;

typedef gl_context* glctx_ptr;
typedef struct{
    glctx_ptr ctx;
    output_file_t outFile;
}_GLVFile;

extern std::array<bool, MAX_GLVFILE_NUM> glv_file_usage;
extern std::array<_GLVFile*, MAX_GLVFILE_NUM> glv_files;
extern _GLVFile *_glv_file_current;

int _glv_files_free_slot();

#endif