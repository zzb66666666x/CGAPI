#include "configs.h"
#include <iostream>
#include "glcontext.h"
#include "render.h"

gl_context* glapi_ctx = nullptr;

gl_context::gl_context(int npixels, bool double_buf){
    // std::cout<<"npixels: "<<npixels<<std::endl;
    // share = glShareData();
    framebuf_1 = glStorage<color_t>(npixels, GL_FRAMEBUFFER);
    zbuf_1 = glStorage<float>(npixels, GL_DEPTH_ATTACHMENT);
    use_double_buf = double_buf;
    zbuf = &zbuf_1;
    sync_unit.resize(npixels);
    std::vector<omp_lock_t>::iterator it;
    for (int i=0; i < sync_unit.size(); i++) {
        omp_init_lock(&(sync_unit[i]));
    }
    cur_sync_unit = &sync_unit;
    override_color_buf = nullptr;
    override_depth_buf = nullptr;
    override_buf_npixels = 0;
    override_default_framebuf = false;
    use_z_test = false;
    draw_color_buf = true;
    flip_image  = true;
    cull_face.open = false;
    framebuf = &framebuf_1;
    if (double_buf){
        framebuf_2 = glStorage<color_t>(npixels, GL_FRAMEBUFFER);
        zbuf_2 = glStorage<float>(npixels, GL_DEPTH_ATTACHMENT);
    }
    // shader = glProgram();
    shader.initialize_layouts();
    // payload = glRenderPayload();
    // pipeline = glPipeline();
    pipeline.pixel_tasks = std::vector<Pixel>(npixels);
    pipeline.init_pixel_locks();
    clear_color.R = 0;
    clear_color.G = 0;
    clear_color.B = 0;
    debug_flag = true;
}

gl_context* _cg_create_context(int width, int height, bool double_buf){
    int npixels = width*height;
    gl_context * ctx = new gl_context(npixels, double_buf);
    ctx->width = width;
    ctx->height = height;
    ctx->start_pos_x = 0;
    ctx->start_pos_y = 0;
    ctx->pipeline.bins = new ScreenBins(width, height);
    ctx->znear = 0.1;
    ctx->zfar = 500;
    // _cg_context_sanity_check(ctx);
    // std::cout<<"context ptr: "<<ctx<<std::endl;
    return ctx;
}   

void _cg_make_current(gl_context* ctx){
    glapi_ctx = ctx;
}

extern void _cg_free_context_data(gl_context* ctx){
    terminate_all_threads();
    delete ctx;
}

void _cg_reset_current_context(){
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

void _cg_swap_framebuffer(gl_context* ctx){
    if(ctx == nullptr){
        return;
    }
    if(ctx->framebuf == &ctx->framebuf_1){
        ctx->framebuf = &ctx->framebuf_2;
        ctx->zbuf = &ctx->zbuf_2;
    }else{
        ctx->framebuf = &ctx->framebuf_1;
        ctx->zbuf = &ctx->zbuf_1;
    }
}