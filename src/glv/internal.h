#ifndef _internal_h_
#define _internal_h_

#include "../gl/glcontext.h"
#include <stdlib.h>

// container of io stream's title
#define MAX_NAME_LEN    128
#define KEY_ESC         27


/**
 * macro function
 */
#define MALLOC(T,LEN) ((T*)malloc(sizeof(T) * (LEN)))
#define FREE(p)         \
    do{                 \
        free(p);        \
        p = NULL;       \
    }while(0)           \

/**
 * glv type
 */
typedef struct{
    int width;
    int height;
    char name[MAX_NAME_LEN];
    // GLV_STREAM_FILE, GLV_STREAM_Window
    int type;
}_GLVStream;

typedef struct{
    gl_context *ctx;
    _GLVStream *curStream;
}_GLVContext;

extern _GLVContext *_glvContext;

extern int should_exit_flag;

// /**
//  * file internal function
//  */
// _GLVStream *create_file_stream(int width, int height, const char *name);
// int write_file_stream(_GLVStream *_file);

// /**
//  * window internal function
//  */
// _GLVStream* create_window_stream(int width, int height, const char* name);
// int write_window_stream(_GLVStream *_window);

#endif