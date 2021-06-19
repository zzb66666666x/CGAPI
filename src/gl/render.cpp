#include <pthread.h>
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
    int indices[] = {0,0};
    glm::vec3* vec3_ptr;
    int flag = 1;
    Triangle* tri = new Triangle();
    angle = angle + 2.0f;
    while (cnt<vertex_num){
        if (!flag){
            delete tri;
            break;
        }
        for (int i=0; i<C->shader.layout_cnt; i++){
            vec3_ptr = nullptr;
            switch(C->shader.layouts[i]){
                case 0:
                    vec3_ptr = &(C->shader.input_Pos);
                    break;
                case 1:
                    vec3_ptr = &(C->shader.vert_Color);
                    break;
                default:
                    throw std::runtime_error("invalid layout\n");
            }
            vertex_attrib_t& config = vattrib_data[C->shader.layouts[i]];
            buf = vbuf_data + (indices[i] + (int)((long long)config.pointer));
            switch(config.type){
                case GL_FLOAT:
                    switch(config.size){
                        case 3:
                            (*vec3_ptr).x = *(float*)(buf+0);
                            (*vec3_ptr).y = *(float*)(buf+sizeof(float)*1);
                            (*vec3_ptr).z = *(float*)(buf+sizeof(float)*2);
                            // std::cout<<"extracted float: "<<(*vec3_ptr).x<<" "<< (*vec3_ptr).y<<" "<< (*vec3_ptr).z<<std::endl;
                            break;
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
        cnt += 1;
        if (cnt>0 && cnt % 3 == 0){
            P->triangle_stream.push(tri);
            tri = nullptr;
            if (cnt+3<=vertex_num)
                tri = new Triangle();
        }
    }
}

inline static glm::vec3 interpolate(float alpha, float beta, float gamma, glm::vec3 &vert1, glm::vec3 &vert2, glm::vec3 &vert3, float weight)
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
            C->shader.default_fragment_shader();
            frame_buf[i].R = C->shader.frag_Color.x * 255.0f;
            frame_buf[i].G = C->shader.frag_Color.y * 255.0f;
            frame_buf[i].B = C->shader.frag_Color.z * 255.0f;
            pixel_tasks[i].write = false;
        }
    }
}

////////////////// MULTI-THREADS VERSION OF RENDERING //////////////////
// thread sync
static pthread_mutex_t vertex_threads_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t vertex_threads_cv = PTHREAD_COND_INITIALIZER;
int should_process_vertex = 0; 
// global variables
int quit_vertex_processing;
int globl_vert_cnt = 0;
int globl_finish_flag = 1;
int globl_vert_indices[2] = {0,0};

// set quit flags, terminate all threads
void terminate_all_threads(){

}

// create 3 threads to parallel vertex processing
void process_geometry_threadmain(){
    static int first_entry = 1;
    static int thread_ids[] = {0,1,2};
    GET_CURRENT_CONTEXT(C);
    vertex_attrib_t* vattrib_data = (vertex_attrib_t*) C->pipeline.vao_ptr->getDataPtr();
    for (int i=0; i<C->shader.layout_cnt; i++){
        if (!vattrib_data[C->shader.layouts[i]].activated)
            throw std::runtime_error("using inactive layout\n");
    }
    pthread_t * ths = C->threads.thr_arr;
    if (first_entry){
        first_entry = 0;
        // create threads
        for (int i=0; i<3; i++){
            pthread_create(&ths[i], NULL, _thr_process_vertex, (void*)&i);
        }
    }
    pthread_mutex_lock(&vertex_threads_mtx);
    should_process_vertex = 1;
    pthread_cond_broadcast(&vertex_threads_cv);
    pthread_mutex_unlock(&vertex_threads_mtx);
}

void *_thr_process_vertex(void *thread_id)
{
    // thread local variables
    int id = *((int*)thread_id);
    GET_CURRENT_CONTEXT(C);
    GET_PIPELINE(P);
    glProgram local_shader = C->shader;
    vertex_attrib_t* vattrib_data = (vertex_attrib_t*) P->vao_ptr->getDataPtr();
    char* vbuf_data = (char *) P->vbo_ptr->getDataPtr();
    int vbuf_size = P->vbo_ptr->getSize();
    int vertex_num = P->vertex_num;
    vbuf_data += (P->first_vertex)*(vattrib_data[0].stride);
    char* buf;
    glm::vec3* vec3_ptr;

    while(!quit_vertex_processing){
        pthread_mutex_lock(&vertex_threads_mtx);
        while(!should_process_vertex){
            pthread_cond_wait(&vertex_threads_cv, &vertex_threads_mtx);
        }
        pthread_mutex_unlock(&vertex_threads_mtx);
        // do the job
        switch(id){
            case 0:
                // manager
                
                break;
            case 1:
            case 2:
                // salves

                break;
            default:
                break;
        }
    }
}

void *_thr_rasterize(void *thread_id)
{

    return nullptr;
}
