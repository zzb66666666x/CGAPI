#ifndef _internal_h_
#define _internal_h_

#include "../../include/glv/glv.h"
#include "../gl/glcontext.h"
#include <stdlib.h>
#include <string>

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
    std::string name;
    // GLV_STREAM_FILE, GLV_STREAM_Window
    int type;
    // only for rendering a window
    GLVcursorposfun mouse_move;
    GLVscrollfun mouse_scroll;
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