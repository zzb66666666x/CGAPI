#ifndef _GLCONTEXT_H
#define _GLCONTEXT_H

#include "configs.h"
#include "globj.h"
#include <atomic>

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
    int start_pos_x;
    int start_pos_y;
    float znear;
    float zfar;
    // frame buffer info and depth test
    glStorage<color_t> *framebuf;
    glStorage<float>   *zbuf;
    std::vector<omp_lock_t>* cur_sync_unit;
    std::vector<omp_lock_t> sync_unit;
    std::vector<std::atomic_bool*>* cur_framebuf_lock;
    std::vector<std::atomic_bool*> framebuf_lock;
    glStorage<color_t>  framebuf_1;
    glStorage<color_t>  framebuf_2;
    glStorage<float>    zbuf_1;
    glStorage<float>    zbuf_2;
    color_t*            override_color_buf;
    float*              override_depth_buf;
    int                 override_buf_npixels;
    bool                override_default_framebuf;
    bool                use_double_buf;
    bool                use_z_test;
    bool                draw_color_buf;
    bool                flip_image;
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

    // for debug
    bool debug_flag;
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