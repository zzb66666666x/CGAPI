#ifndef _GLCONTEXT_H
#define _GLCONTEXT_H

#include "configs.h"
#include "globj.h"

class gl_context{
    public:
    // construct 
    gl_context(int npixels, bool double_buf);
    // data
    glShareData         share;
    // multi-threading
    glThreads           threads;
    // width & height
    int width;
    int height;
    float znear;
    float zfar;
    // frame buffer info and depth test
    glStorage<color_t> *framebuf;
    glStorage<float>   *zbuf;
    glStorage<color_t> *windowbuf;
    glStorage<color_t>  framebuf_1;
    glStorage<color_t>  framebuf_2;
    glStorage<float>    zbuf_1;
    glStorage<float>    zbuf_2;
    bool                use_double_buf;
    bool                use_z_test;
    struct {
        bool open;
        unsigned int cull_face_mode;
        unsigned int front_face_mode;
    }cull_face;
    // shader
    glProgram               shader;
    // programmable shader programs
    glProgramManager         glsl_shaders;
    // redering pipeline requirement
    glRenderPayload         payload;
    // pipeline of function pointers and their data 
    glPipeline              pipeline;
    // framebuffer clear color
    color_t                 clear_color;
};

extern gl_context* glapi_ctx;

#define GET_CURRENT_CONTEXT(C) gl_context *C = glapi_ctx

extern void _cg_context_sanity_check(gl_context * ctx);
extern gl_context* _cg_create_context(int width, int height, bool double_buf);
extern void _cg_make_current(gl_context* ctx);
extern void _cg_free_context_data(gl_context* ctx);
extern void _cg_reset_current_context();
extern void _cg_swap_framebuffer(gl_context* ctx);

#endif