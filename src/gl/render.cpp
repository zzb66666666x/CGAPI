#include "configs.h"
#include <pthread.h>
#include <assert.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "render.h"
#include "glcontext.h"
#include "../../include/gl/common.h"

#define GET_PIPELINE(P) glPipeline *P = &(glapi_ctx->pipeline)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define GET_INDEX(x, y, width, height) ((height - 1 - y) * width + x)

// static helpers
static glm::vec3 interpolate(float alpha, float beta, float gamma, glm::vec3 &vert1, glm::vec3 &vert2, glm::vec3 &vert3, float weight);
static glm::vec2 interpolate(float alpha, float beta, float gamma, glm::vec2 &vert1, glm::vec2 &vert2, glm::vec2 &vert3, float weight);

// for test
float angle = 0.0f;

// geometry processing
void process_geometry()
{
    // 1. parse vbo and do vertex shading
    GET_PIPELINE(P);
    GET_CURRENT_CONTEXT(C);
    vertex_attrib_t* vattrib_data = (vertex_attrib_t*) P->vao_ptr->getDataPtr();
    char* vbuf_data = (char *) P->vbo_ptr->getDataPtr();
    int vbuf_size = P->vbo_ptr->getSize();
    int vertex_num = P->vertex_num;
    // 2. check if the config is activated
    for (int i=0; i<C->shader.layout_cnt; i++){
        if (!vattrib_data[C->shader.layouts[i]].activated)
            throw std::runtime_error("using inactive layout\n");
    }
    // 3. parse vertex data
    vbuf_data += (P->first_vertex)*(vattrib_data[0].stride); // use the first stride to determine start of vertex buffer
    // float * temp = (float*) vbuf_data;
    // for (int i=0; i<18; i++){
    //     std::cout<<temp[i]<<std::endl;
    // }
    
    int cnt = 0;
    char* buf;
    int indices[] = {0,0,0};
    void* input_ptr;
    int flag = 1;
    Triangle* tri = new Triangle();
    angle = angle + 2.0f;
    while (cnt < vertex_num){
        if (!flag){
            delete tri;
            break;
        }
        for (int i = 0; i < C->shader.layout_cnt; i++){
            if(C->shader.layouts[i] > 2){
                throw std::runtime_error("invalid layout\n");
            }
            switch(C->shader.layouts[i]){
                case 0:
                    input_ptr = &(C->shader.input_Pos);break;
                case 1:
                    input_ptr = &(C->shader.vert_Color);break;
                case 2:
                    input_ptr = &(C->shader.iTexcoord);break;
            }
            vertex_attrib_t& config = vattrib_data[C->shader.layouts[i]];
            buf = vbuf_data + (indices[i] + (int)((long long)config.pointer));
            switch(config.type){
                case GL_FLOAT:
                    switch(config.size){
                        case 2:{
                            glm::vec2 *vec2 = (glm::vec2*)input_ptr;
                            vec2->x = *(float*)(buf + 0);
                            vec2->y = *(float*)(buf + sizeof(float) * 1);
                            break;
                        }
                        case 3:{
                            glm::vec3 *vec3 = (glm::vec3*)input_ptr;
                            vec3->x = *(float*)(buf + 0);
                            vec3->y = *(float*)(buf + sizeof(float) * 1);
                            vec3->z = *(float*)(buf + sizeof(float) * 2);
                            // std::cout<<"extracted float: "<<(*vec3).x<<" "<< (*vec3).y<<" "<< (*vec3).z<<std::endl;
                            break;
                        }
                        default: 
                            throw std::runtime_error("not supported size\n");
                    }
                    break;
                default: 
                    throw std::runtime_error("not supported type\n");
            }
            indices[i] += config.stride;
            if (indices[i] >= vbuf_size){
                flag = 0;
            }
        }
        // 4. vertex shading
        C->shader.set_transform_matrices(C->width, C->height, C->znear, C->zfar, angle);
        C->shader.default_vertex_shader();

        C->shader.gl_Position.x /= C->shader.gl_Position.w;
        C->shader.gl_Position.y /= C->shader.gl_Position.w;
        C->shader.gl_Position.z /= C->shader.gl_Position.w;
        // 5. view port transformation
        C->shader.gl_Position.x = 0.5 * C->width * (C->shader.gl_Position.x + 1.0);
        C->shader.gl_Position.y = 0.5 * C->height * (C->shader.gl_Position.y + 1.0);  
        // [-1,1] to [0,1]
        C->shader.gl_Position.z = C->shader.gl_Position.z * 0.5 + 0.5;
        // 6. assemble triangle
        tri->screen_pos[cnt%3] = C->shader.gl_Position;
        tri->color[cnt%3] = C->shader.gl_VertexColor;
        tri->frag_shading_pos[cnt%3] = C->shader.frag_Pos;
        tri->texcoord[cnt%3] = C->shader.iTexcoord;
        ++cnt;
        if (cnt % 3 == 0){
            P->triangle_stream.push(tri);
            if (cnt + 3 > vertex_num){
                break;
            }
            tri = new Triangle();
        }
    }
}

inline static glm::vec3 interpolate(float alpha, float beta, float gamma, glm::vec3 &vert1, glm::vec3 &vert2, glm::vec3 &vert3, float weight)
{
    return (alpha * vert1 + beta * vert2 + gamma * vert3) / weight;
}

inline static glm::vec2 interpolate(float alpha, float beta, float gamma, glm::vec2 &vert1, glm::vec2 &vert2, glm::vec2 &vert3, float weight)
{
    return (alpha * vert1 + beta * vert2 + gamma * vert3) / weight;
}

void rasterize()
{
    GET_CURRENT_CONTEXT(C);
    std::queue<Triangle *> &triangle_stream = C->pipeline.triangle_stream;
    int width = C->width, height = C->height;
    std::vector<Pixel> &pixel_tasks = C->pipeline.pixel_tasks;
    
    while (!triangle_stream.empty())
    {
        Triangle *t = triangle_stream.front();
        triangle_stream.pop();
        glm::vec4 *screen_pos = t->screen_pos;
        int minx, maxx, miny, maxy, x, y;
        minx = MIN(screen_pos[0].x, MIN(screen_pos[1].x, screen_pos[2].x));
        miny = MIN(screen_pos[0].y, MIN(screen_pos[1].y, screen_pos[2].y));
        maxx = MAX(screen_pos[0].x, MAX(screen_pos[1].x, screen_pos[2].x));
        maxy = MAX(screen_pos[0].y, MAX(screen_pos[1].y, screen_pos[2].y));

        float *zbuf = (float *)C->zbuf->getDataPtr();
        // AABB algorithm
        for (y = miny; y <= maxy; ++y)
        {
            for (x = minx; x <= maxx; ++x)
            {
                int index = GET_INDEX(x, y, width, height);
                if (!t->inside(x + 0.5f, y + 0.5f))
                    continue;

                // alpha beta gamma
                glm::vec3 coef = t->computeBarycentric2D(x + 0.5f, y + 0.5f);
                // perspective correction
                float Z_viewspace = 1.0/(coef[0]/screen_pos[0].w + coef[1]/screen_pos[1].w + coef[2]/screen_pos[2].w);
                float alpha = coef[0]*Z_viewspace/screen_pos[0].w;
                float beta = coef[1]*Z_viewspace/screen_pos[1].w;
                float gamma = coef[2]*Z_viewspace/screen_pos[2].w;
               
                if (!C->use_z_test)
                {
                    pixel_tasks[index].write = true;
                    pixel_tasks[index].vertexColor = interpolate(alpha, beta, gamma, t->color[0], t->color[1], t->color[2], 1);
                }
                else{
                    // zp: z value after interpolation
                    float zp = alpha*screen_pos[0].z + beta*screen_pos[1].z + gamma*screen_pos[2].z;
                    if (zp < zbuf[index]){
                        zbuf[index] = zp;
                        pixel_tasks[index].write = true;
                        pixel_tasks[index].vertexColor = interpolate(alpha, beta, gamma, t->color[0], t->color[1], t->color[2], 1);
                        pixel_tasks[index].texcoord = interpolate(alpha, beta, gamma, t->texcoord[0], t->texcoord[1], t->texcoord[2], 1);
                    }
                }
            }
        }

        delete t;
    }
}

void process_pixel()
{
    GET_CURRENT_CONTEXT(C);
    std::vector<Pixel> &pixel_tasks = C->pipeline.pixel_tasks;
    color_t* frame_buf = (color_t*)C->framebuf->getDataPtr();
    for(int i = 0,len = pixel_tasks.size();i < len;++i)
    {
        if(pixel_tasks[i].write){
            C->shader.diffuse_Color = pixel_tasks[i].vertexColor;
            C->shader.texcoord = pixel_tasks[i].texcoord;
            C->shader.default_fragment_shader();
            frame_buf[i].R = C->shader.frag_Color.x;
            frame_buf[i].G = C->shader.frag_Color.y;
            frame_buf[i].B = C->shader.frag_Color.z;
            pixel_tasks[i].write = false;
        }
    }
}

////////////////// MULTI-THREADS VERSION OF RENDERING //////////////////
// macros
#define PROCESS_VERTEX_THREAD_COUNT 3
#define DOING_VERTEX_PROCESSING     1
#define DOING_RASTERIZATION         2

// thread functions
void* _thr_process_vertex(void* thread_id);
void* _thr_rasterize(void* thread_id);
void *_thr_process_vertex_ver1(void *thread_id);

// thread sync, the barrier to sync threads after processing 3 vertices
static pthread_mutex_t vertex_threads_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t vertex_threads_cv = PTHREAD_COND_INITIALIZER;
static pthread_cond_t inc_vertex_indices_cv = PTHREAD_COND_INITIALIZER;
int process_vertex_sync = 0; 

// global variables
volatile int quit_vertex_processing = 0;            // for terminate the threads, make thread functions quit while loop
int globl_vert_cnt = 0;                             // count the vertices already processed
volatile int globl_proceed_flag = 1;                // flag for whether the vertex processing is finished
int globl_vert_indices[2] = {0,0};                  // progress in processing the vertices
Triangle* globl_new_triangle = nullptr;             // assembly the triangle

// pipeline status
static pthread_mutex_t pipeline_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pipeline_cv = PTHREAD_COND_INITIALIZER;
volatile int pipeline_stage = DOING_VERTEX_PROCESSING;

// to update thread local variables 
typedef struct{
    vertex_attrib_t* vattrib_data;
    char* vbuf_data;
    int vbuf_size;
    int vertex_num;
} vertex_threads_locals_t;
vertex_threads_locals_t vertex_threads_locals;
int new_frame[PROCESS_VERTEX_THREAD_COUNT] = {0,0,0};

static void clear_vertex_indices(){
    for (int i=0; i<glapi_ctx->shader.layout_cnt; i++){
        globl_vert_indices[i] = 0;
    }
}

// set quit flags, terminate all threads
void terminate_all_threads(){
    GET_CURRENT_CONTEXT(C);
    quit_vertex_processing = 1;
    for (int i=0; i<PROCESS_VERTEX_THREAD_COUNT; i++){
        pthread_cancel(C->threads.thr_arr[i]);
        pthread_join(C->threads.thr_arr[i], NULL);
    }
}

// very first version of vertex processing
void process_geometry_threadmain(){
    static int first_entry = 1;
    static int thread_ids[PROCESS_VERTEX_THREAD_COUNT] = {0,1,2};
    GET_CURRENT_CONTEXT(C);
    GET_PIPELINE(P);
    // begin to draw new frame
    for(int i=0; i<PROCESS_VERTEX_THREAD_COUNT; i++){
        new_frame[i] = 1;
    }
    angle += 2.0;
    // sanity check before drawing
    vertex_attrib_t* vattrib_data = (vertex_attrib_t*) C->pipeline.vao_ptr->getDataPtr();
    for (int i=0; i<C->shader.layout_cnt; i++){
        if (!vattrib_data[C->shader.layouts[i]].activated)
            throw std::runtime_error("using inactive layout\n");
    }
    pthread_t * ths = C->threads.thr_arr;
    // update local variables in thread function
    vertex_threads_locals.vattrib_data = (vertex_attrib_t*) P->vao_ptr->getDataPtr();
    vertex_threads_locals.vbuf_data = (char *) P->vbo_ptr->getDataPtr() + (P->first_vertex)*(vattrib_data[0].stride);
    vertex_threads_locals.vbuf_size = P->vbo_ptr->getSize();
    vertex_threads_locals.vertex_num = P->vertex_num;
    // critical section
    pthread_mutex_lock(&vertex_threads_mtx);
    C->shader.set_transform_matrices(C->width, C->height, C->znear, C->zfar, angle);
    if (first_entry){
        first_entry = 0;
        // create threads
        for (int i=0; i<PROCESS_VERTEX_THREAD_COUNT; i++){
            pthread_create(&ths[thread_ids[i]], NULL, _thr_process_vertex, (void *)(long long)i);
        }
    }
    quit_vertex_processing = 0;
    globl_vert_cnt = 0;
    clear_vertex_indices();
    globl_proceed_flag = 1;
    process_vertex_sync = 0;
    pthread_cond_broadcast(&vertex_threads_cv);
    pthread_mutex_unlock(&vertex_threads_mtx);
    pthread_mutex_lock(&pipeline_mtx);
    pipeline_stage = DOING_VERTEX_PROCESSING;
    while (pipeline_stage == DOING_VERTEX_PROCESSING){
        pthread_cond_wait(&pipeline_cv, &pipeline_mtx);
    }
    pthread_mutex_unlock(&pipeline_mtx);
}

// the very first version of vertex processing thread function
void *_thr_process_vertex(void *thread_id)
{
    // thread local variables, variables that do not change often
    int id = (int)(long long)(thread_id);
    // pthread_mutex_lock(&vertex_threads_mtx);
    // std::cout<<id<<std::endl;
    // pthread_mutex_unlock(&vertex_threads_mtx);
    GET_CURRENT_CONTEXT(C);
    glProgram local_shader = C->shader;
    vertex_attrib_t* vattrib_data = nullptr;
    char* vbuf_data = nullptr;
    int vbuf_size = 0;
    int vertex_num = 0;
    // constantly changing variables
    int local_vert_idx;
    char* buf = nullptr;
    glm::vec3* vec3_ptr = nullptr;
    // while the whole program is not terminated (eg. press the close button of window)
    while(!quit_vertex_processing){
        if (new_frame[id]){
            vattrib_data = vertex_threads_locals.vattrib_data;
            vbuf_data = vertex_threads_locals.vbuf_data;
            vbuf_size = vertex_threads_locals.vbuf_size;
            vertex_num = vertex_threads_locals.vertex_num;
            new_frame[id] = 0;
        }
        // first loop over the layouts
        for (int i=0; i<local_shader.layout_cnt; i++){
            vec3_ptr = nullptr;
            switch(C->shader.layouts[i]){
                case 0:
                    vec3_ptr = &(local_shader.input_Pos);
                    break;
                case 1:
                    vec3_ptr = &(local_shader.vert_Color);
                    break;
                default:
                    throw std::runtime_error("invalid layout\n");
            }
            vertex_attrib_t& config = vattrib_data[local_shader.layouts[i]];
            // one sync point here since we want to move the globl_vert_indices
            pthread_mutex_lock(&vertex_threads_mtx);
            process_vertex_sync++;
            local_vert_idx = globl_vert_indices[i];
            if (process_vertex_sync == PROCESS_VERTEX_THREAD_COUNT){
                process_vertex_sync = 0;
                globl_vert_indices[i] += 3*config.stride;
                pthread_cond_broadcast(&inc_vertex_indices_cv);
            }
            else{
                while (pthread_cond_wait(&inc_vertex_indices_cv, &vertex_threads_mtx)!=0);
            }
            if (globl_vert_indices[i] > vbuf_size){
                pthread_mutex_unlock(&vertex_threads_mtx);
                break;
            }
            pthread_mutex_unlock(&vertex_threads_mtx);
            // then there are more than 3 vertices to process
            buf = vbuf_data + (local_vert_idx + (int)((long long)config.pointer)) + id*config.stride;
            switch(config.type){
                case GL_FLOAT:
                    switch(config.size){
                        case 3:
                            (*vec3_ptr).x = *(float*)(buf+0);
                            (*vec3_ptr).y = *(float*)(buf+sizeof(float)*1);
                            (*vec3_ptr).z = *(float*)(buf+sizeof(float)*2);
                            // pthread_mutex_lock(&vertex_threads_mtx);
                            // std::cout<<"extracted float at #"<<id<<": "<<(*vec3_ptr).x<<" "<< (*vec3_ptr).y<<" "<< (*vec3_ptr).z<<std::endl;
                            // pthread_mutex_unlock(&vertex_threads_mtx);
                            break;
                        default: 
                            throw std::runtime_error("not supported size\n");
                    }
                    break;
                default: 
                    throw std::runtime_error("not supported type\n");
            }
        }
        pthread_mutex_lock(&vertex_threads_mtx);
        if (globl_proceed_flag){
            pthread_mutex_unlock(&vertex_threads_mtx);
            // second do vertex shading, and put data inside one triangle
            local_shader.default_vertex_shader();
            local_shader.gl_Position.x /= local_shader.gl_Position.w;
            local_shader.gl_Position.y /= local_shader.gl_Position.w;
            local_shader.gl_Position.z /= local_shader.gl_Position.w;
            // third view port transformation
            local_shader.gl_Position.x = 0.5 * C->width * (local_shader.gl_Position.x + 1.0);
            local_shader.gl_Position.y = 0.5 * C->height * (local_shader.gl_Position.y + 1.0);  
            local_shader.gl_Position.z = local_shader.gl_Position.z * 0.5 + 0.5;
        }
        else{
            pthread_mutex_unlock(&vertex_threads_mtx);
        }
        // finally sync here and assemble triangle
        pthread_mutex_lock(&vertex_threads_mtx);
        // locked
        if (process_vertex_sync == 0 && globl_proceed_flag){
            globl_new_triangle = new Triangle;
        }
        process_vertex_sync ++;
        if (globl_new_triangle != nullptr){
            globl_vert_cnt ++;
            globl_new_triangle->screen_pos[id] = local_shader.gl_Position;
            globl_new_triangle->color[id] = local_shader.gl_VertexColor;
            globl_new_triangle->frag_shading_pos[id] = local_shader.frag_Pos; 
        }
        if (process_vertex_sync == PROCESS_VERTEX_THREAD_COUNT){
            process_vertex_sync = 0;
            if (globl_vert_cnt <= vertex_num && globl_new_triangle != nullptr){
                assert(globl_vert_cnt % 3 == 0);
                C->pipeline.triangle_stream.push(globl_new_triangle);
                globl_new_triangle = nullptr;
                pthread_cond_broadcast(&vertex_threads_cv);
            }
            else{
                pthread_mutex_lock(&pipeline_mtx);
                pipeline_stage = DOING_RASTERIZATION;
                pthread_cond_signal(&pipeline_cv);
                pthread_mutex_unlock(&pipeline_mtx);
                // sleep, wait for next draw call from user space, so that it can be awaken
                globl_proceed_flag = 0;
                while (globl_proceed_flag != 1){ // wait for the threadmain to wait it up again
                    // std::cout<<"receive broadcast signal at: "<<id<<std::endl;
                    pthread_cond_wait(&vertex_threads_cv, &vertex_threads_mtx);
                }
                // std::cout<<"waking up at: "<<id<<std::endl;
            }
        }
        else{
            // wait for the next round of processing triangles
            do{
                pthread_cond_wait(&vertex_threads_cv, &vertex_threads_mtx);
            }while (globl_proceed_flag != 1);
        }
        // unlock here
        pthread_mutex_unlock(&vertex_threads_mtx);
    }
    return nullptr;
}

// imporved version of vertex processing: version 1
void process_geometry_threadmain_ver1(){
    
}

void *_thr_process_vertex_ver1(void *thread_id){


}

void rasterize_threadmain(){
    rasterize();
}

void *_thr_rasterize(void *thread_id)
{

    return nullptr;
}
