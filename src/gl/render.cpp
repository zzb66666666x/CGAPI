#include "render.h"
#include "../../include/gl/common.h"
#include "configs.h"
#include "glcontext.h"
#include <assert.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <pthread.h>
#include <stdio.h>

#include <atomic>
#include <thread>

#define GET_PIPELINE(P) glPipeline* P = &(glapi_ctx->pipeline)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define GET_INDEX(x, y, width, height) ((height - 1 - y) * width + x)

// for test
float angle = 0.0f;

////////////////// GENERAL STATIC HELPERS //////////////////
inline static glm::vec3 interpolate(float alpha, float beta, float gamma, glm::vec3& vert1, glm::vec3& vert2, glm::vec3& vert3, float weight)
{
    return (alpha * vert1 + beta * vert2 + gamma * vert3) / weight;
}

inline static glm::vec2 interpolate(float alpha, float beta, float gamma, glm::vec2& vert1, glm::vec2& vert2, glm::vec2& vert3, float weight)
{
    return (alpha * vert1 + beta * vert2 + gamma * vert3) / weight;
}

////////////////// SINGLE-THREAD VERSION OF RENDERING //////////////////
// single thread version of processing parsing vertex data 
void process_geometry()
{
    // 1. parse vbo and do vertex shading
    GET_PIPELINE(P);
    GET_CURRENT_CONTEXT(C);
    vertex_attrib_t* vattrib_data = (vertex_attrib_t*)P->vao_ptr->getDataPtr();
    char* vbuf_data = (char*)P->vbo_ptr->getDataPtr();
    int vbuf_size = P->vbo_ptr->getSize();
    int vertex_num = P->vertex_num;
    // 2. check if the config is activated
    int indices[GL_MAX_VERTEX_ATTRIB_NUM];
    for (int i = 0; i < C->shader.layout_cnt; i++) {
        if (C->shader.layouts[i] == LAYOUT_INVALID) {
            continue;
        } else if (C->shader.layouts[i] >= P->vao_ptr->getSize()) {
            C->shader.layouts[i] = LAYOUT_INVALID;
            continue;
        } else if (!vattrib_data[C->shader.layouts[i]].activated) {
            C->shader.layouts[i] = LAYOUT_INVALID;
        }
        else{
            // valid layout
            int temp = C->shader.layouts[i];
            indices[temp] = (P->first_vertex) * (vattrib_data[temp].stride);
        }
    }
    // 3. parse vertex data
    int cnt = P->first_vertex;
    char* buf;
    void* input_ptr;
    int flag = 1;
    Triangle* tri = new Triangle();
    angle = angle + 2.0f;
    C->shader.set_transform_matrices(C->width, C->height, C->znear, C->zfar, angle);
    while (cnt < vertex_num) {
        if (!flag) {
            delete tri;
            break;
        }
        for (int i = 0; i < C->shader.layout_cnt; i++) {
            int layout = C->shader.layouts[i];
            if (layout > 3) {
                throw std::runtime_error("invalid layout\n");
            }
            switch (layout) {
            case LAYOUT_POSITION:
                input_ptr = &(C->shader.input_Pos);
                break;
            case LAYOUT_COLOR:
                input_ptr = &(C->shader.vert_Color);
                break;
            case LAYOUT_TEXCOORD:
                input_ptr = &(C->shader.iTexcoord);
                break;
            case LAYOUT_NORMAL:
                input_ptr = &(C->shader.vert_Normal);
                break;
            case LAYOUT_INVALID:
            default:
                input_ptr = nullptr;
                break;
            }
            if (input_ptr == nullptr)
                continue;
            vertex_attrib_t& config = vattrib_data[layout];
            buf = vbuf_data + (P->first_vertex) * (config.stride) +
                  (indices[layout] + (int)((long long)config.pointer));
            switch (config.type) {
            case GL_FLOAT:
                switch (config.size) {
                case 2: {
                    glm::vec2* vec2 = (glm::vec2*)input_ptr;
                    vec2->x = *(float*)(buf + 0);
                    vec2->y = *(float*)(buf + sizeof(float) * 1);
                    break;
                }
                case 3: {
                    glm::vec3* vec3 = (glm::vec3*)input_ptr;
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
            indices[layout] += config.stride;
            if (indices[layout] >= vbuf_size) {
                flag = 0;
            }
        } 
        // 4. vertex shading
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
        tri->screen_pos[cnt % 3] = C->shader.gl_Position;
        tri->color[cnt % 3] = C->shader.gl_VertexColor;
        tri->frag_shading_pos[cnt % 3] = C->shader.frag_Pos;
        tri->texcoord[cnt % 3] = C->shader.iTexcoord;
        ++cnt;
        if (cnt % 3 == 0) {
            P->triangle_stream.push(tri);
            if (cnt + 3 > vertex_num) {
                break;
            }
            tri = new Triangle();
        }
    }
}

// process vertex with support for EBO
void process_geometry_ebo()
{
    /**
     * input processing
     */
    GET_PIPELINE(ppl);
    GET_CURRENT_CONTEXT(ctx);
    vertex_attrib_t* vattrib_data = (vertex_attrib_t*)ppl->vao_ptr->getDataPtr();

    // check if the config is activated
    for (int i = 0; i < ctx->shader.layout_cnt; ++i) {
        if (ctx->shader.layouts[i] != LAYOUT_INVALID
            && (ctx->shader.layouts[i] >= ppl->vao_ptr->getSize()
                || !vattrib_data[ctx->shader.layouts[i]].activated)) {
            ctx->shader.layouts[i] = LAYOUT_INVALID;
        }
    }

    std::vector<int>& indices = ppl->indices;
    int triangle_size = 0;
    if (ppl->use_indices) {
        // first ebo data index
        const void* first_indices = (const void*)ppl->ebo_config.first_indices;
        switch (ppl->ebo_config.indices_type) {
            case GL_UNSIGNED_INT: {
                // ebo data array
                unsigned int* ebuf_data = (unsigned int*)ppl->ebo_config.ebo_ptr->getDataPtr();
                int first_index = (size_t)first_indices / sizeof(unsigned int);
                int ebuf_size = MIN(ppl->vertex_num, ppl->ebo_config.ebo_ptr->getSize());
                // case: ((6 - 1) / 3) * 3 + 1 == 4 , first_index == 1
                ebuf_size = ((ebuf_size - first_index) / 3) * 3 + first_index;
                // vertex_num = ((vertex_num - first_vertex_ind) % 3) * 3 + first_vertex_ind;
                indices.resize(ebuf_size - first_index);
                for (int i = first_index; i < ebuf_size; ++i) {
                    indices[i] = ebuf_data[i];
                }
                triangle_size = (ebuf_size - first_index) / 3;
            } break;
            default:
                break;
        }

    } else {
        int first_vertex_ind = ppl->first_vertex;
        int vertex_num = MIN(ppl->vertex_num, ppl->vbo_ptr->getSize() / vattrib_data[0].stride);
        // int vertex_offsets[3] = { 0, 0, 0 };

        // parse vertex data
        // use the first stride to determine start of vertex buffer
        // vbuf_data += first_vertex_ind * vattrib_data[0].stride;

        // case: ((38 - 33) / 3) * 3 + 33 == 36, first_vertex_ind == 33
        vertex_num = ((vertex_num - first_vertex_ind) / 3) * 3 + first_vertex_ind;
        indices.resize(vertex_num - first_vertex_ind);
        for (int i = 0, len = vertex_num - first_vertex_ind; i < len; ++i) {
            indices[i] = first_vertex_ind + i;
        }
        triangle_size = (vertex_num - first_vertex_ind) / 3;
    }

    /**
     * 
     */
    unsigned char* vbuf_data = (unsigned char*)ppl->vbo_ptr->getDataPtr();

    std::vector<Triangle>& triangle_list = ppl->triangle_list;
    triangle_list.resize(triangle_size);
    int tri_ind = 0;
    void* input_ptr;
    unsigned char* buf;
    // angle = angle + 2.0f;
    while (tri_ind < triangle_size) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < ctx->shader.layout_cnt; ++j) {
                if (ctx->shader.layouts[j] > 3) {
                    throw std::runtime_error("invalid layout\n");
                }
                switch (ctx->shader.layouts[j]) {
                    case LAYOUT_POSITION:
                        input_ptr = &(ctx->shader.input_Pos);
                        break;
                    case LAYOUT_COLOR:
                        input_ptr = &(ctx->shader.vert_Color);
                        break;
                    case LAYOUT_TEXCOORD:
                        input_ptr = &(ctx->shader.iTexcoord);
                        break;
                    case LAYOUT_NORMAL:
                        input_ptr = &(ctx->shader.vert_Normal);
                        break;
                    case LAYOUT_INVALID:
                    default:
                        input_ptr = nullptr;
                        break;
                }
                if (input_ptr == nullptr) {
                    continue;
                }
                vertex_attrib_t& config = vattrib_data[ctx->shader.layouts[j]];
                // buf = vbuf_data + (indices[i] + (int)((long long)config.pointer));
                buf = vbuf_data + (size_t)(indices[tri_ind * 3 + i] * config.stride) + (size_t)config.pointer;
                switch (config.type) {
                    case GL_FLOAT:
                        switch (config.size) {
                            case 2: {
                                glm::vec2* vec2 = (glm::vec2*)input_ptr;
                                vec2->x = *(float*)(buf + 0);
                                vec2->y = *(float*)(buf + sizeof(float) * 1);
                                break;
                            }
                            case 3: {
                                glm::vec3* vec3 = (glm::vec3*)input_ptr;
                                vec3->x = *(float*)(buf + 0);
                                vec3->y = *(float*)(buf + sizeof(float) * 1);
                                vec3->z = *(float*)(buf + sizeof(float) * 2);
                                break;
                            }
                            default:
                                throw std::runtime_error("not supported size\n");
                            }
                            break;
                    default:
                        throw std::runtime_error("not supported type\n");
                }
            }
            // 4. vertex shading
            ctx->shader.set_transform_matrices(ctx->width, ctx->height, ctx->znear, ctx->zfar, angle);
            ctx->shader.default_vertex_shader();

            ctx->shader.gl_Position.x /= ctx->shader.gl_Position.w;
            ctx->shader.gl_Position.y /= ctx->shader.gl_Position.w;
            ctx->shader.gl_Position.z /= ctx->shader.gl_Position.w;
            // 5. view port transformation
            ctx->shader.gl_Position.x = 0.5 * ctx->width * (ctx->shader.gl_Position.x + 1.0);
            ctx->shader.gl_Position.y = 0.5 * ctx->height * (ctx->shader.gl_Position.y + 1.0);
            // [-1,1] to [0,1]
            ctx->shader.gl_Position.z = ctx->shader.gl_Position.z * 0.5 + 0.5;
            // 6. assemble triangle
            triangle_list[tri_ind].screen_pos[i] = ctx->shader.gl_Position;
            triangle_list[tri_ind].color[i] = ctx->shader.gl_VertexColor;
            triangle_list[tri_ind].frag_shading_pos[i] = ctx->shader.frag_Pos;
            triangle_list[tri_ind].texcoord[i] = ctx->shader.iTexcoord;
        }
        ++tri_ind;
    }
}

// single thread version of rasterization
// generating pixel tasks for fragment shading
void rasterize()
{
    GET_CURRENT_CONTEXT(C);
    std::queue<Triangle*>& triangle_stream = C->pipeline.triangle_stream;
    int width = C->width, height = C->height;
    std::vector<Pixel>& pixel_tasks = C->pipeline.pixel_tasks;

    while (!triangle_stream.empty()) {
        Triangle* t = triangle_stream.front();
        triangle_stream.pop();
        glm::vec4* screen_pos = t->screen_pos;
        int minx, maxx, miny, maxy, x, y;
        minx = MIN(screen_pos[0].x, MIN(screen_pos[1].x, screen_pos[2].x));
        miny = MIN(screen_pos[0].y, MIN(screen_pos[1].y, screen_pos[2].y));
        maxx = MAX(screen_pos[0].x, MAX(screen_pos[1].x, screen_pos[2].x));
        maxy = MAX(screen_pos[0].y, MAX(screen_pos[1].y, screen_pos[2].y));

        float* zbuf = (float*)C->zbuf->getDataPtr();
        // AABB algorithm
        for (y = miny; y <= maxy; ++y) {
            for (x = minx; x <= maxx; ++x) {
                int index = GET_INDEX(x, y, width, height);
                if (!t->inside(x + 0.5f, y + 0.5f))
                    continue;

                // alpha beta gamma
                glm::vec3 coef = t->computeBarycentric2D(x + 0.5f, y + 0.5f);
                // perspective correction
                float Z_viewspace = 1.0 / (coef[0] / screen_pos[0].w + coef[1] / screen_pos[1].w + coef[2] / screen_pos[2].w);
                float alpha = coef[0] * Z_viewspace / screen_pos[0].w;
                float beta = coef[1] * Z_viewspace / screen_pos[1].w;
                float gamma = coef[2] * Z_viewspace / screen_pos[2].w;

                if (!C->use_z_test) {
                    pixel_tasks[index].write = true;
                    pixel_tasks[index].vertexColor = interpolate(alpha, beta, gamma, t->color[0], t->color[1], t->color[2], 1);
                } else {
                    // zp: z value after interpolation
                    float zp = alpha * screen_pos[0].z + beta * screen_pos[1].z + gamma * screen_pos[2].z;
                    if (zp < zbuf[index]) {
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

// raterization with the support for EBO drawing
void rasterize_ebo()
{
    GET_CURRENT_CONTEXT(ctx);
    std::vector<Triangle>& triangle_list = ctx->pipeline.triangle_list;
    int width = ctx->width, height = ctx->height;
    std::vector<Pixel>& pixel_tasks = ctx->pipeline.pixel_tasks;

    Triangle* t = nullptr;
    for (int i = 0, len = triangle_list.size(); i < len; ++i) {
        t = &triangle_list[i];
        glm::vec4* screen_pos = t->screen_pos;
        int minx, maxx, miny, maxy, x, y;
        minx = MIN(screen_pos[0].x, MIN(screen_pos[1].x, screen_pos[2].x));
        miny = MIN(screen_pos[0].y, MIN(screen_pos[1].y, screen_pos[2].y));
        maxx = MAX(screen_pos[0].x, MAX(screen_pos[1].x, screen_pos[2].x));
        maxy = MAX(screen_pos[0].y, MAX(screen_pos[1].y, screen_pos[2].y));

        float* zbuf = (float*)ctx->zbuf->getDataPtr();
        // AABB algorithm
        for (y = miny; y <= maxy; ++y) {
            for (x = minx; x <= maxx; ++x) {
                int index = GET_INDEX(x, y, width, height);
                if (!t->inside(x + 0.5f, y + 0.5f))
                    continue;

                // alpha beta gamma
                glm::vec3 coef = t->computeBarycentric2D(x + 0.5f, y + 0.5f);
                // perspective correction
                float Z_viewspace = 1.0 / (coef[0] / screen_pos[0].w + coef[1] / screen_pos[1].w + coef[2] / screen_pos[2].w);
                float alpha = coef[0] * Z_viewspace / screen_pos[0].w;
                float beta = coef[1] * Z_viewspace / screen_pos[1].w;
                float gamma = coef[2] * Z_viewspace / screen_pos[2].w;

                if (!ctx->use_z_test) {
                    pixel_tasks[index].write = true;
                    pixel_tasks[index].vertexColor = interpolate(alpha, beta, gamma, t->color[0], t->color[1], t->color[2], 1);
                } else {
                    // zp: z value after interpolation
                    float zp = alpha * screen_pos[0].z + beta * screen_pos[1].z + gamma * screen_pos[2].z;
                    if (zp < zbuf[index]) {
                        zbuf[index] = zp;
                        pixel_tasks[index].write = true;
                        pixel_tasks[index].vertexColor = interpolate(alpha, beta, gamma, t->color[0], t->color[1], t->color[2], 1);
                        pixel_tasks[index].texcoord = interpolate(alpha, beta, gamma, t->texcoord[0], t->texcoord[1], t->texcoord[2], 1);
                    }
                }
            }
        }
    }
}

// single thread version of fragment shading
void process_pixel()
{
    GET_CURRENT_CONTEXT(ctx);
    GET_PIPELINE(ppl);
    std::vector<Pixel>& pixel_tasks = ppl->pixel_tasks;
    color_t* frame_buf = (color_t*)ctx->framebuf->getDataPtr();
    for (int i = 0, len = pixel_tasks.size(); i < len; ++i) {
        if (pixel_tasks[i].write) {
            ctx->shader.diffuse_Color = pixel_tasks[i].vertexColor;
            ctx->shader.texcoord = pixel_tasks[i].texcoord;
            ctx->shader.default_fragment_shader();
            frame_buf[i].R = ctx->shader.frag_Color.x * 255;
            frame_buf[i].G = ctx->shader.frag_Color.y * 255;
            frame_buf[i].B = ctx->shader.frag_Color.z * 255;
            pixel_tasks[i].write = false;
        }
    }
}

// do the rasterization and fragment shading all at once
// save the cost of r/w of pixel_task buffer
// with the cost of less flexibility
void rasterize_with_shading(){
    GET_CURRENT_CONTEXT(C);
    std::queue<Triangle*>& triangle_stream = C->pipeline.triangle_stream;
    int width = C->width, height = C->height;
    std::vector<Pixel>& pixel_tasks = C->pipeline.pixel_tasks;

    while (!triangle_stream.empty()) {
        Triangle* t = triangle_stream.front();
        triangle_stream.pop();
        glm::vec4* screen_pos = t->screen_pos;
        int minx, maxx, miny, maxy, x, y;
        minx = MIN(screen_pos[0].x, MIN(screen_pos[1].x, screen_pos[2].x));
        miny = MIN(screen_pos[0].y, MIN(screen_pos[1].y, screen_pos[2].y));
        maxx = MAX(screen_pos[0].x, MAX(screen_pos[1].x, screen_pos[2].x));
        maxy = MAX(screen_pos[0].y, MAX(screen_pos[1].y, screen_pos[2].y));

        float* zbuf = (float*)C->zbuf->getDataPtr();
        color_t* frame_buf = (color_t*)C->framebuf->getDataPtr();
        // AABB algorithm
        for (y = miny; y <= maxy; ++y) {
            for (x = minx; x <= maxx; ++x) {
                int index = GET_INDEX(x, y, width, height);
                if (!t->inside(x + 0.5f, y + 0.5f))
                    continue;

                // alpha beta gamma
                glm::vec3 coef = t->computeBarycentric2D(x + 0.5f, y + 0.5f);
                // perspective correction
                float Z_viewspace = 1.0 / (coef[0] / screen_pos[0].w + coef[1] / screen_pos[1].w + coef[2] / screen_pos[2].w);
                float alpha = coef[0] * Z_viewspace / screen_pos[0].w;
                float beta = coef[1] * Z_viewspace / screen_pos[1].w;
                float gamma = coef[2] * Z_viewspace / screen_pos[2].w;
                
                if (!C->use_z_test) {
                    throw std::runtime_error("please open the z depth test\n");
                } 
                else {
                    // zp: z value after interpolation
                    float zp = alpha * screen_pos[0].z + beta * screen_pos[1].z + gamma * screen_pos[2].z;
                    if (zp < zbuf[index]) {
                        zbuf[index] = zp;
                        // fragment shader input
                        C->shader.diffuse_Color = interpolate(alpha, beta, gamma, t->color[0], t->color[1], t->color[2], 1);
                        C->shader.texcoord = interpolate(alpha, beta, gamma, t->texcoord[0], t->texcoord[1], t->texcoord[2], 1);
                        // fragment shading
                        C->shader.default_fragment_shader();
                        frame_buf[index].R = C->shader.frag_Color.x * 255.0f;
                        frame_buf[index].G = C->shader.frag_Color.y * 255.0f;
                        frame_buf[index].B = C->shader.frag_Color.z * 255.0f;
                    }
                }
            }
        }
        delete t;
    }
}

////////////////// MULTI-THREADS VERSION OF RENDERING //////////////////
// thread functions
void* _thr_process_vertex(void* thread_id);
void* _thr_rasterize(void* thread_id);

// helpers
static inline void _merge_crawlers();

// thread sync, the barrier to sync threads after processing 3 vertices
static pthread_mutex_t vertex_threads_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t vertex_threads_cv = PTHREAD_COND_INITIALIZER;
int process_vertex_sync = 0;

// global variables
volatile int quit_vertex_processing = 0; // for terminate the threads, make thread functions quit while loop
TriangleCrawler crawlers[PROCESS_VERTEX_THREAD_COUNT];

// pipeline status
static pthread_mutex_t pipeline_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pipeline_cv = PTHREAD_COND_INITIALIZER;
volatile int pipeline_stage = DOING_VERTEX_PROCESSING;

// set quit flags, terminate all threads
void terminate_all_threads()
{
    GET_CURRENT_CONTEXT(C);
    quit_vertex_processing = 1;
    for (int i = 0; i < PROCESS_VERTEX_THREAD_COUNT; i++) {
        pthread_cancel(C->threads.thr_arr[i]);
        pthread_join(C->threads.thr_arr[i], NULL);
    }
    C->threads.reset();
}

static inline void _merge_crawlers(){
    // only one thread is allowed to enter this function in one time
    Triangle * tri = new Triangle;
    int cnt = 0;
    int crawlerID = 0;
    int flag = 1;
    while (flag){
        TriangleCrawler& crawler = crawlers[crawlerID];
        crawlerID = (crawlerID+1) % PROCESS_VERTEX_THREAD_COUNT;
        if (cnt%3 == 0 && cnt>0){
            glapi_ctx->pipeline.triangle_stream.push(tri);
            tri = new Triangle;
        }
        if(flag){
            // vec2
            std::map<int, std::queue<glm::vec2>>::iterator it;
            glm::vec2 * vec_ptr;
            for (it = crawler.data_float_vec2.begin(); it != crawler.data_float_vec2.end(); it++){
                vec_ptr = nullptr;
                switch(it->first){
                    case VSHADER_OUT_TEXCOORD:
                        vec_ptr = &(tri->texcoord[cnt%3]);
                        break;
                    default:
                        break;
                }
                if (it->second.empty() || vec_ptr == nullptr){
                    flag = 0;
                    delete tri;
                    tri = nullptr;
                    break;
                }
                else{
                    *vec_ptr = it->second.front();
                    it->second.pop();
                }
            }
        }
        if (flag){
            // vec3 
            std::map<int,std::queue<glm::vec3>>::iterator it;
            glm::vec3* vec_ptr;
            for (it = crawler.data_float_vec3.begin(); it != crawler.data_float_vec3.end(); it++){
                vec_ptr = nullptr;
                switch(it->first){
                    case VSHADER_OUT_COLOR:
                        vec_ptr = &(tri->color[cnt%3]);
                        break;
                    case VSHADER_OUT_NORMAL:
                        vec_ptr = &(tri->vert_normal[cnt%3]);
                        break;
                    case VSHADER_OUT_FRAGPOS:
                        vec_ptr = &(tri->frag_shading_pos[cnt%3]);
                        break;
                    default:
                        break;
                }
                if (it->second.empty() || vec_ptr==nullptr){
                    flag = 0;
                    delete tri;
                    tri = nullptr;
                    break;
                }
                else{
                    *vec_ptr = it->second.front();
                    it->second.pop();
                }
            }
        }
        if (flag){
            // vec4
            std::map<int, std::queue<glm::vec4>>::iterator it;
            glm::vec4* vec_ptr;
            for (it = crawler.data_float_vec4.begin(); it != crawler.data_float_vec4.end(); it++){
                vec_ptr = nullptr;
                switch(it->first){
                    case VSHADER_OUT_POSITION:
                        vec_ptr = &(tri->screen_pos[cnt%3]);
                        break;
                    default:
                        break;
                }
                if (it->second.empty() || vec_ptr==nullptr){
                    flag = 0;
                    delete tri;
                    tri = nullptr;
                    break;
                }
                else{
                    *vec_ptr = it->second.front();
                    it->second.pop();
                }
            }
        }
        // std::cout<<tri->color[0].x<<" "<<tri->color[0].y<<" "<<tri->color[0].z<<std::endl;
        // std::cout<<tri->color[1].x<<" "<<tri->color[1].y<<" "<<tri->color[1].z<<std::endl;
        // std::cout<<tri->color[2].x<<" "<<tri->color[2].y<<" "<<tri->color[2].z<<std::endl;
        // std::cout<<std::endl;
        cnt++;
    }
}

void process_geometry_threadmain()
{
    static int first_entry = 1;
    static int thread_ids[PROCESS_VERTEX_THREAD_COUNT];
    GET_CURRENT_CONTEXT(C);
    GET_PIPELINE(P);
    // update the angle
    angle += 2.0;
    // sanity check before drawing
    vertex_attrib_t* vattrib_data = (vertex_attrib_t*)C->pipeline.vao_ptr->getDataPtr();
    for (int i = 0; i < C->shader.layout_cnt; i++) {
        if (C->shader.layouts[i] == LAYOUT_INVALID) {
            continue;
        } else if (C->shader.layouts[i] >= P->vao_ptr->getSize()) {
            C->shader.layouts[i] = LAYOUT_INVALID;
            continue;
        } else if (!vattrib_data[C->shader.layouts[i]].activated) {
            C->shader.layouts[i] = LAYOUT_INVALID;
        }
    }
    pthread_t* ths = C->threads.thr_arr;
    // critical section
    pthread_mutex_lock(&vertex_threads_mtx);
    C->shader.set_transform_matrices(C->width, C->height, C->znear, C->zfar, angle);
    if (first_entry) {
        first_entry = 0;
        // create threads
        assert(C->threads.get(thread_ids, PROCESS_VERTEX_THREAD_COUNT) == GL_SUCCESS);
        for (int i = 0; i < PROCESS_VERTEX_THREAD_COUNT; i++) {
            crawlers[i].reset_config();
            pthread_create(&ths[thread_ids[i]], NULL, _thr_process_vertex, (void*)(long long)i);
        }
    }
    quit_vertex_processing = 0;
    process_vertex_sync = 0;
    pthread_cond_broadcast(&vertex_threads_cv);
    pthread_mutex_unlock(&vertex_threads_mtx);
    // make main thread sleep
    pthread_mutex_lock(&pipeline_mtx);
    pipeline_stage = DOING_VERTEX_PROCESSING;
    while (pipeline_stage == DOING_VERTEX_PROCESSING) {
        pthread_cond_wait(&pipeline_cv, &pipeline_mtx);
    }
    pthread_mutex_unlock(&pipeline_mtx);
}

void* _thr_process_vertex(void* thread_id)
{
    int id = (int)(long long)(thread_id);
    GET_CURRENT_CONTEXT(C);
    glProgram local_shader = C->shader;
    vertex_attrib_t* vattrib_data = nullptr;
    char* vbuf_data = nullptr;
    int vbuf_size = 0;
    int vertex_num = 0;
    // while the whole program is not terminated (eg. press the close button of window)
    while (!quit_vertex_processing) {
        // do one frame's job, and sleep
        // 1. fetch data from context
        vattrib_data = (vertex_attrib_t*) C->pipeline.vao_ptr->getDataPtr();
        vbuf_data = (char*)C->pipeline.vbo_ptr->getDataPtr();
        vbuf_size = C->pipeline.vbo_ptr->getSize();
        // 2. register config information of all layouts to crawler
        for (int i=0; i<GL_MAX_VERTEX_ATTRIB_NUM; i++){
            int layout = local_shader.layouts[i];
            if (layout>=0 && layout<GL_MAX_VERTEX_ATTRIB_NUM){
                crawlers[id].set_config(layout, vattrib_data[layout].size, vattrib_data[layout].stride,
                (int)(long long)vattrib_data[layout].pointer, vattrib_data[layout].type);
            }
        }
        crawlers[id].set_start_point(local_shader.layouts, local_shader.layout_cnt, id, C->pipeline.first_vertex);
        // 3. crawl the data until return failure
        while (1){
            int ret = crawlers[id].crawl(vbuf_data, vbuf_size, C->pipeline.first_vertex, local_shader);
            if (ret==GL_FAILURE)
                break; 
        }
        // 4. sync and merge crawlers
        pthread_mutex_lock(&vertex_threads_mtx);
        process_vertex_sync ++;
        if (process_vertex_sync == PROCESS_VERTEX_THREAD_COUNT){
            process_vertex_sync = 0;
            // merge crawlers
            _merge_crawlers();
            // move the pipeline forward
            pthread_mutex_lock(&pipeline_mtx);
            pipeline_stage = DOING_RASTERIZATION;
            pthread_cond_signal(&pipeline_cv);
            pthread_mutex_unlock(&pipeline_mtx);
            // sleep 
            while (pthread_cond_wait(&vertex_threads_cv, &vertex_threads_mtx) != 0);
        }
        else{
            while (pthread_cond_wait(&vertex_threads_cv, &vertex_threads_mtx) != 0);
        }
        pthread_mutex_unlock(&vertex_threads_mtx);
        // 5. reset the crawler and ready for next frame
        crawlers[id].reset_config();
        crawlers[id].reset_data();
    }
    return nullptr;
}

void rasterize_threadmain()
{
    // std::cout << "begin to rasterize\n";
    rasterize_with_shading();
}

void* _thr_rasterize(void* thread_id)
{

    return nullptr;
}


