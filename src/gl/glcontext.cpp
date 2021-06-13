#include <iostream>
#include "glcontext.h"

gl_context* glapi_ctx = nullptr;

gl_context::gl_context(int npixels, bool double_buf){
    std::cout<<"npixels: "<<npixels<<std::endl;
    share = glShareData();
    framebuf_1 = glStorage<color_t>(npixels, true, GLOBJ_FRAMEBUF, GL_FRAMEBUFFER);
    zbuf_1 = glStorage<float>(npixels, false, GLOBJ_ZBUF, GL_FRAMEBUFFER_ATTACH_ZBUF);
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
    _cg_context_sanity_check(ctx);
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

void _cg_context_sanity_check(gl_context* ctx){
    std::cout<<"the context itself has size: "<<sizeof(gl_context)<<std::endl;
    color_t* data = (color_t *) ctx->framebuf_1.getDataPtr();
    int size = ctx->framebuf_1.getSize();
    color_t temp_color = {1,2,3};
    std::cout<<"begin checking frame buffer"<<std::endl;
    for (int i=0; i<size; i++){
        data[i] = temp_color;
    }
    std::cout<<"pass test of frame buffer"<<std::endl;

    float* data_zbuf = (float *) ctx->zbuf_1.getDataPtr();
    int size_zbuf = ctx->zbuf_1.getSize();
    std::cout<<"begin checking z buffer"<<std::endl;
    for (int i=0; i<size_zbuf; i++){
        data_zbuf[i] = 100;
        // printf("%d\n",i);
    }
    std::cout<<"pass test of z buffer"<<std::endl;
}