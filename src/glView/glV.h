#ifndef _GLV_H
#define _GLV_H

#include "../gl/glcontext.h"
#include "visualize.h"

typedef struct{
    gl_context      ctx;
    visual_config   conf;
}glVisual;

typedef glVisual* glV;

// interface function
glV glVCreateContext();
int glVMakeCurrent();


#endif