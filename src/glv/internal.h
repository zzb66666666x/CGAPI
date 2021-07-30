#ifndef _internal_h_
#define _internal_h_

#include "../../include/glv/glv.h"
#include "../gl/glcontext.h"
#include <stdlib.h>
#include <string>

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
    bool should_exit_flag;
    int keyinput;
}_GLVStream;

typedef struct{
    gl_context *ctx;
    _GLVStream *curStream;
}_GLVContext;

extern _GLVContext *_glvContext;

#endif