#include <iostream>
#include "glcontext.h"

gl_context* glapi_ctx = nullptr;

gl_context::gl_context(int npixels, bool double_buf){
    std::cout<<"npixels: "<<npixels<<std::endl;
    share = glShareData();
    framebuf_1 = glStorage<color_t>(npixels, true, GLOBJ_FRAMEBUF, GL_FRAMEBUFFER);
    use_double_buf = double_buf;
    zbuf = nullptr;
    framebuf = &framebuf_1;
    if (double_buf){
        framebuf_2 = glStorage<color_t>(npixels, true, GLOBJ_FRAMEBUF, GL_FRAMEBUFFER);
        zbuf_2 = glStorage<float>(npixels, true, GLOBJ_ZBUF, GL_FRAMEBUFFER_ATTACH_ZBUF);
    }
    shader = glProgram();
    payload = glRenderPayload();
    pipeline = glPipeline();
    windowbuf = nullptr;
}

gl_context* _cg_create_context(int width, int height, bool double_buf){
    int npixels = width*height;
    gl_context * ctx = new gl_context(npixels, double_buf);
    std::cout<<"context ptr: "<<ctx<<std::endl;
    return ctx;
}   

void _cg_make_current(gl_context* ctx){
    glapi_ctx = ctx;
}

extern void _cg_free_context_data(){
    delete glapi_ctx;
    glapi_ctx = nullptr;
}