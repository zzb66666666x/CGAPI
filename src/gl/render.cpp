#include <pthread.h>
#include "render.h"
#include "glcontext.h"
#include "../../include/gl/common.h"

#define GET_PIPELINE(P) glPipeline* P = &(glapi_ctx->pipeline)

// wrapper of vertex shader
void process_vertex(){
    
}

// work with the triStream
void assemble_triangle(){

}

// rasterize one triangle per call 
void rasterize(){

}

// Wrapper of fragment shader program
void calculate_color(){

}

void* _thr_process_vertex(void* thread_id){

    return nullptr;
}

void* _thr_rasterize(void* thread_id){

    return nullptr;
}
