#include "glcontext.h"

gl_context* glapi_ctx = nullptr;

void set_global_gl_context(gl_context* ptr){
    glapi_ctx = ptr;
}
