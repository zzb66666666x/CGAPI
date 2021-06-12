#include "glcontext.h"

gl_context* glapi_ctx = nullptr;

void set_global_gl_context(gl_context* ptr){
    glapi_ctx = ptr;
}

gl_context::gl_context(int npixels, bool double_buf){
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
    return ctx;
}   