#include "render.h"
#include "../../include/gl/common.h"
#include "configs.h"
#include "glcontext.h"
#include "binning.h"
#include <assert.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <pthread.h>
#include <stdio.h>
#include <set>
#include <omp.h>

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

inline static void back_face_culling(Triangle& t)
{
    glm::vec3 v01 = t.screen_pos[1] - t.screen_pos[0];
    glm::vec3 v02 = t.screen_pos[2] - t.screen_pos[0];
    glm::vec3 res = glm::cross(v01, v02);

    // the horizontal plane will not be culled.
    t.culling = res.z < 0.0f;
}

static void view_culling(Triangle& t, std::vector<Triangle *> &view_culling_list){


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
        } else {
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
            buf = vbuf_data + (P->first_vertex) * (config.stride) + (indices[layout] + (int)((long long)config.pointer));
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
    // printf("screen ratio: %f%%\n", cnt / (double)len * 100);
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

////////////////// OPENMP MULTI-THREADS VERSION OF RENDERING //////////////////
void process_geometry_ebo_openmp()
{
    /**
     * input assembly
     */
    GET_PIPELINE(ppl);
    GET_CURRENT_CONTEXT(ctx);

    vertex_attrib_t* vattrib_data = (vertex_attrib_t*)ppl->vao_ptr->getDataPtr();

    std::vector<int> indices;
    int triangle_size = 0;
    if (ppl->use_indices) {
        int vaoId = ctx->payload.renderMap[GL_ARRAY_BUFFER];
        ppl->indexCache.getCacheData(vaoId, indices);
        if (!(ppl->ebo_config.ebo_ptr->usage == GL_STATIC_DRAW && ppl->vbo_ptr->usage == GL_STATIC_DRAW && indices.size() != 0)) {
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
#pragma omp parallel for
                for (int i = first_index; i < ebuf_size; ++i) {
                    indices[i] = ebuf_data[i];
                }
                triangle_size = (ebuf_size - first_index) / 3;
            } break;
            default:
                break;
            }
            ppl->indexCache.addCacheData(vaoId, indices);
        } else {
            triangle_size = indices.size() / 3;
        }

    } else {
        int vaoId = ctx->payload.renderMap[GL_ARRAY_BUFFER];
        ppl->indexCache.getCacheData(vaoId, indices);
        if (!(ppl->vbo_ptr->usage == GL_STATIC_DRAW && indices.size() != 0)) {

            int first_vertex_ind = ppl->first_vertex;
            int vertex_num = MIN(ppl->vertex_num, ppl->vbo_ptr->getSize() / vattrib_data[0].stride);

            // case: ((38 - 33) / 3) * 3 + 33 == 36, first_vertex_ind == 33
            vertex_num = ((vertex_num - first_vertex_ind) / 3) * 3 + first_vertex_ind;
            indices.resize(vertex_num - first_vertex_ind);

            int len = vertex_num - first_vertex_ind;
#pragma omp parallel for
            for (int i = 0; i < len; ++i) {
                indices[i] = first_vertex_ind + i;
                // printf("i=%d. Hello! threadID=%d  thraed number:%d\n", i, omp_get_thread_num(), omp_get_num_threads());
            }
            triangle_size = (vertex_num - first_vertex_ind) / 3;
            ppl->indexCache.addCacheData(vaoId, indices);
        } else {
            triangle_size = indices.size() / 3;
        }
    }

    unsigned char* vbuf_data = (unsigned char*)ppl->vbo_ptr->getDataPtr();

    std::vector<Triangle*>& triangle_list = ppl->triangle_list;

    // check and delete
    if (triangle_list.size() != triangle_size) {
        if (triangle_list.size() != 0) {
            for (int i = 0, len = triangle_list.size(); i < len; ++i) {
                delete triangle_list[i];
            }
        } else {
            triangle_list.resize(triangle_size);
            for (int i = 0; i < triangle_size; ++i) {
                triangle_list[i] = new Triangle();
            }
        }
    }

    angle = angle + 1.0f;

    // begin parallel block
    void* input_ptr;
    unsigned char* buf;
    glProgram shader = ctx->shader;
    int i, j;
#pragma omp parallel for private(input_ptr) private(buf) private(shader) private(i) private(j)
    for (int tri_ind = 0; tri_ind < triangle_size; ++tri_ind) {
        // printf("tri_ind=%d. Hello! threadID=%d  thraed number:%d\n", tri_ind, omp_get_thread_num(), omp_get_num_threads());
        // parse data
        for (i = 0; i < 3; ++i) {
            for (j = 0; j < shader.layout_cnt; ++j) {
                switch (shader.layouts[j]) {
                    case LAYOUT_POSITION:
                        input_ptr = &(shader.input_Pos);
                        break;
                    case LAYOUT_COLOR:
                        input_ptr = &(shader.vert_Color);
                        break;
                    case LAYOUT_TEXCOORD:
                        input_ptr = &(shader.iTexcoord);
                        break;
                    case LAYOUT_NORMAL:
                        input_ptr = &(shader.vert_Normal);
                        break;
                    case LAYOUT_INVALID:
                    default:
                        input_ptr = nullptr;
                        break;
                }
                if (input_ptr == nullptr) {
                    continue;
                }
                vertex_attrib_t& config = vattrib_data[shader.layouts[j]];
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
            shader.set_transform_matrices(ctx->width, ctx->height, ctx->znear, ctx->zfar, angle);
            shader.default_vertex_shader();

            // assemble triangle
            triangle_list[tri_ind]->screen_pos[i] = shader.gl_Position;
            triangle_list[tri_ind]->color[i] = shader.gl_VertexColor;
            triangle_list[tri_ind]->frag_shading_pos[i] = shader.frag_Pos;
            triangle_list[tri_ind]->texcoord[i] = shader.iTexcoord;
            triangle_list[tri_ind]->vert_normal[i] = shader.gl_Normal;
        }

        // TODO view frustum culling
        //
        // std::vector<Triangle*> view_culling_list;
        // view_culling(*triangle_list[tri_ind], view_culling_list);
        // TODO back face culling
        if(true){
            back_face_culling(*triangle_list[tri_ind]);
        }
        // if (true) {
        //     if (!triangle_list[tri_ind]->culling) {
        //         back_face_culling(*triangle_list[tri_ind]);
        //     } else {
        //         for (int ind = 0, tlen = view_culling_list.size(); ind < tlen; ++ind) {
        //             back_face_culling(*view_culling_list[ind]);
        //             if (!view_culling_list[ind]->culling) {
        //                 // TODO add main triangle list
        //             }
        //         }
        //     }
        // }

        if (triangle_list[tri_ind]->culling) {
            continue;
        }

        for (i = 0; i < 3; ++i) {
            triangle_list[tri_ind]->screen_pos[i] /= triangle_list[tri_ind]->screen_pos[i].w;

            // view port transformation
            triangle_list[tri_ind]->screen_pos[i].x = 0.5 * ctx->width * (triangle_list[tri_ind]->screen_pos[i].x + 1.0);
            triangle_list[tri_ind]->screen_pos[i].y = 0.5 * ctx->height * (triangle_list[tri_ind]->screen_pos[i].y + 1.0);

            // [-1,1] to [0,1]
            triangle_list[tri_ind]->screen_pos[i].z = triangle_list[tri_ind]->screen_pos[i].z * 0.5 + 0.5;
        }
    }
}

void rasterize_with_shading_openmp(){
    GET_CURRENT_CONTEXT(ctx);
    std::vector<Triangle*>& triangle_list = ctx->pipeline.triangle_list;
    int width = ctx->width, height = ctx->height;
    std::vector<Pixel>& pixel_tasks = ctx->pipeline.pixel_tasks;

    color_t* frame_buf = (color_t*)ctx->framebuf->getDataPtr();

    Triangle* t = nullptr;
    int len = triangle_list.size();
    float* zbuf = (float*)ctx->zbuf->getDataPtr();
    glProgram shader = ctx->shader;

#pragma omp parallel for private(t) private(shader)
    for (int i = 0; i < len; ++i) {
        t = triangle_list[i];

        if (t->culling) {
            t->culling = false;
            continue;
        }

        glm::vec4* screen_pos = t->screen_pos;
        int minx, maxx, miny, maxy, x, y;
        minx = MIN(screen_pos[0].x, MIN(screen_pos[1].x, screen_pos[2].x));
        miny = MIN(screen_pos[0].y, MIN(screen_pos[1].y, screen_pos[2].y));
        maxx = MAX(screen_pos[0].x, MAX(screen_pos[1].x, screen_pos[2].x));
        maxy = MAX(screen_pos[0].y, MAX(screen_pos[1].y, screen_pos[2].y));

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
                    throw std::runtime_error("please open the z depth test\n");
                } 
                else {
                    // zp: z value after interpolation
                    float zp = alpha * screen_pos[0].z + beta * screen_pos[1].z + gamma * screen_pos[2].z;
                    omp_set_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                    if (zp < zbuf[index]) {
                        zbuf[index] = zp;
                        // omp_unset_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                        // fragment shader input
                        shader.diffuse_Color = interpolate(alpha, beta, gamma, t->color[0], t->color[1], t->color[2], 1);
                        shader.texcoord = interpolate(alpha, beta, gamma, t->texcoord[0], t->texcoord[1], t->texcoord[2], 1);
                        // fragment shading
                        shader.default_fragment_shader();
                        frame_buf[index].R = shader.frag_Color.x * 255.0f;
                        frame_buf[index].G = shader.frag_Color.y * 255.0f;
                        frame_buf[index].B = shader.frag_Color.z * 255.0f;
                    }
                    omp_unset_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                }
            }
        }
    }
}

////////////////// MANUAL MULTI-THREADS VERSION OF RENDERING //////////////////
// thread functions
void* _thr_process_vertex(void* thread_id);
void* _thr_binning(void* thread_id);
void* _thr_rasterize(void* thread_id);

// helpers
static inline void _merge_crawlers();
static inline void _free_triangles();

// thread utils for vertex processing
static pthread_mutex_t vertex_threads_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t vertex_threads_cv = PTHREAD_COND_INITIALIZER;
int process_vertex_sync = 0;

// thread utils for binning
static pthread_mutex_t binning_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t binning_cv = PTHREAD_COND_INITIALIZER;
int binning_sync = 0;

// thread untils for rasterizing
static pthread_mutex_t rasterize_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t rasterize_cv = PTHREAD_COND_INITIALIZER;
int rasterize_sync = 0;

// global variables
volatile int quit_vertex_processing = 0;
volatile int quit_rasterizing = 0;
volatile int quit_binning = 0;
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
    quit_binning = 1;
    quit_rasterizing = 1;
    // terminate threads in use
    // C->threads.reset();
    // free allocated memory
    _free_triangles();
}

static inline void _merge_crawlers()
{
    // only one thread is allowed to enter this function in one time
    Triangle* tri = new Triangle;
    int cnt = 0;
    int crawlerID = 0;
    int flag = 1;
    while (flag) {
        TriangleCrawler& crawler = crawlers[crawlerID];
        crawlerID = (crawlerID+1) % PROCESS_VERTEX_THREAD_COUNT;
        if (cnt%3 == 0 && cnt>0){
            // for compatibility of old functions (they use queue to store triangle pointers)
            glapi_ctx->pipeline.triangle_stream.push(tri);
            // for parallel computation in binning and rasterizing, use std vector to store tri pointers
            // since using a queue requires a mutex lock
            glapi_ctx->pipeline.triangle_list.push_back(tri);
            tri = new Triangle;
        }
        if (flag) {
            // vec2
            std::map<int, std::queue<glm::vec2>>::iterator it;
            glm::vec2* vec_ptr;
            for (it = crawler.data_float_vec2.begin(); it != crawler.data_float_vec2.end(); it++) {
                vec_ptr = nullptr;
                switch (it->first) {
                    case VSHADER_OUT_TEXCOORD:
                        vec_ptr = &(tri->texcoord[cnt % 3]);
                        break;
                    default:
                        break;
                }
                if (it->second.empty() || vec_ptr == nullptr) {
                    flag = 0;
                    delete tri;
                    tri = nullptr;
                    break;
                } else {
                    *vec_ptr = it->second.front();
                    it->second.pop();
                }
            }
        }
        if (flag) {
            // vec3
            std::map<int, std::queue<glm::vec3>>::iterator it;
            glm::vec3* vec_ptr;
            for (it = crawler.data_float_vec3.begin(); it != crawler.data_float_vec3.end(); it++) {
                vec_ptr = nullptr;
                switch (it->first) {
                    case VSHADER_OUT_COLOR:
                        vec_ptr = &(tri->color[cnt % 3]);
                        break;
                    case VSHADER_OUT_NORMAL:
                        vec_ptr = &(tri->vert_normal[cnt % 3]);
                        break;
                    case VSHADER_OUT_FRAGPOS:
                        vec_ptr = &(tri->frag_shading_pos[cnt % 3]);
                        break;
                    default:
                        break;
                }
                if (it->second.empty() || vec_ptr == nullptr) {
                    flag = 0;
                    delete tri;
                    tri = nullptr;
                    break;
                } else {
                    *vec_ptr = it->second.front();
                    it->second.pop();
                }
            }
        }
        if (flag) {
            // vec4
            std::map<int, std::queue<glm::vec4>>::iterator it;
            glm::vec4* vec_ptr;
            for (it = crawler.data_float_vec4.begin(); it != crawler.data_float_vec4.end(); it++) {
                vec_ptr = nullptr;
                switch (it->first) {
                    case VSHADER_OUT_POSITION:
                        vec_ptr = &(tri->screen_pos[cnt % 3]);
                        break;
                    default:
                        break;
                }
                if (it->second.empty() || vec_ptr == nullptr) {
                    flag = 0;
                    delete tri;
                    tri = nullptr;
                    break;
                } else {
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
    // std::cout<<"finish vertex processing"<<std::endl;
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
        vattrib_data = (vertex_attrib_t*)C->pipeline.vao_ptr->getDataPtr();
        vbuf_data = (char*)C->pipeline.vbo_ptr->getDataPtr();
        vbuf_size = C->pipeline.vbo_ptr->getSize();
        // 2. register config information of all layouts to crawler
        for (int i = 0; i < GL_MAX_VERTEX_ATTRIB_NUM; i++) {
            int layout = local_shader.layouts[i];
            if (layout >= 0 && layout < GL_MAX_VERTEX_ATTRIB_NUM) {
                crawlers[id].set_config(layout, vattrib_data[layout].size, vattrib_data[layout].stride,
                    (int)(long long)vattrib_data[layout].pointer, vattrib_data[layout].type);
            }
        }
        crawlers[id].set_start_point(local_shader.layouts, local_shader.layout_cnt, id, C->pipeline.first_vertex);
        // 3. crawl the data until return failure
        while (1) {
            int ret = crawlers[id].crawl(vbuf_data, vbuf_size, C->pipeline.first_vertex, local_shader);
            if (ret == GL_FAILURE)
                break;
        }
        // 4. sync and merge crawlers
        pthread_mutex_lock(&vertex_threads_mtx);
        process_vertex_sync++;
        if (process_vertex_sync == PROCESS_VERTEX_THREAD_COUNT) {
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
        } else {
            while (pthread_cond_wait(&vertex_threads_cv, &vertex_threads_mtx) != 0);
        }
        pthread_mutex_unlock(&vertex_threads_mtx);
        // 5. reset the crawler and ready for next frame
        crawlers[id].reset_config();
        crawlers[id].reset_data();
    }
    return nullptr;
}

void binning_threadmain(){
    static int first_entry = 1;
    static int thread_ids[BINNING_THREAD_COUNT];
    GET_CURRENT_CONTEXT(C);
    GET_PIPELINE(P);
    pthread_t* ths = C->threads.thr_arr;
    // set the multi-threading
    pthread_mutex_lock(&binning_mtx);
    if (first_entry) {
        first_entry = 0;
        // create threads
        assert(C->threads.get(thread_ids, BINNING_THREAD_COUNT) == GL_SUCCESS);
        for (int i = 0; i < BINNING_THREAD_COUNT; i++) {
            pthread_create(&ths[thread_ids[i]], NULL, _thr_binning, (void*)(long long)i);
        }
    }
    quit_binning = 0;
    binning_sync = 0;
    pthread_cond_broadcast(&binning_cv);
    pthread_mutex_unlock(&binning_mtx);
    // make main thread sleep
    pthread_mutex_lock(&pipeline_mtx);
    pipeline_stage = DOING_BINNING;
    while (pipeline_stage == DOING_BINNING) {
        pthread_cond_wait(&pipeline_cv, &pipeline_mtx);
    }
    pthread_mutex_unlock(&pipeline_mtx);
    // std::cout<<"finish binning"<<std::endl;
}

void* _thr_binning(void* thread_id){
    int id = (int)(long long)(thread_id);
    GET_PIPELINE(P);
    Triangle* cur_tri;
    int tri_idx;
    while (!quit_binning){
        // start of each frame
        tri_idx = id;
        while (1){
            // IF YOU ARE USING QUEUE TO STORE TRIANGLE POINTERS
            // pthread_mutex_lock(& P->triangle_stream_mtx);
            // if (P->triangle_stream.empty()){
            //     pthread_mutex_unlock(& P->triangle_stream_mtx);
            //     break;
            // }
            // cur_tri = P->triangle_stream.front();
            // P->triangle_stream.pop();
            // pthread_mutex_unlock(& P-> triangle_stream_mtx);
            // USE VECTOR FOR NO LOCK OPERATION
            if (tri_idx >= P->triangle_list.size()){
                break;
            }
            cur_tri = P->triangle_list[tri_idx];
            tri_idx += BINNING_THREAD_COUNT;
            if (cur_tri->culling) {
                cur_tri->culling = false;
                continue;
            }
            // check triangle's overlap with bins
            std::set<Bin*> overlap_bins = binning_overlap(cur_tri, P->bins);
            std::set<Bin*>::iterator it;
            for (it = overlap_bins.begin(); it != overlap_bins.end(); it++){
                Bin* temp_bin = *it;
                uint64_t mask = compute_cover_mask(cur_tri, temp_bin);
                if (mask != 0){
                    pthread_mutex_lock(&temp_bin->lock);
                    temp_bin->tasks.push((primitive_t){cur_tri, mask});
                    pthread_mutex_unlock(&temp_bin->lock);
                }
            }
        }
        // std::cout<<"partially finish binning"<<std::endl;
        // finish binning, sleep
        pthread_mutex_lock(&binning_mtx);
        binning_sync++;
        if (binning_sync == BINNING_THREAD_COUNT){
            binning_sync = 0;
            // calculate bins with non-zero triangles
            P->bins->prepare_non_empty_bins();
            // move the pipeline forward
            pthread_mutex_lock(&pipeline_mtx);
            pipeline_stage = DOING_RASTERIZATION;
            pthread_cond_signal(&pipeline_cv);
            pthread_mutex_unlock(&pipeline_mtx);
            while (pthread_cond_wait(&binning_cv, &binning_mtx) != 0);
        }else{
            while(pthread_cond_wait(&binning_cv, &binning_mtx) != 0);
        }
        pthread_mutex_unlock(&binning_mtx);
    }
    return nullptr;
}

void rasterize_threadmain()
{
    // std::cout << "begin to rasterize\n";
    // rasterize_with_shading();
    static int first_entry = 1;
    static int thread_ids[RASTERIZE_THREAD_COUNT];
    GET_CURRENT_CONTEXT(C);
    GET_PIPELINE(P);
    pthread_t* ths = C->threads.thr_arr;
    // set the multi-threading
    pthread_mutex_lock(&rasterize_mtx);
    if (first_entry) {
        first_entry = 0;
        // create threads
        assert(C->threads.get(thread_ids, RASTERIZE_THREAD_COUNT) == GL_SUCCESS);
        for (int i = 0; i < RASTERIZE_THREAD_COUNT; i++) {
            pthread_create(&ths[thread_ids[i]], NULL, _thr_rasterize, (void*)(long long)i);
        }
    }
    quit_rasterizing = 0;
    rasterize_sync = 0;
    pthread_cond_broadcast(&rasterize_cv);
    pthread_mutex_unlock(&rasterize_mtx);
    // make main thread sleep
    pthread_mutex_lock(&pipeline_mtx);
    pipeline_stage = DOING_RASTERIZATION;
    while (pipeline_stage == DOING_RASTERIZATION) {
        pthread_cond_wait(&pipeline_cv, &pipeline_mtx);
    }
    pthread_mutex_unlock(&pipeline_mtx);
    // std::cout<<"finish rasterizing"<<std::endl;
}

static inline void _free_triangles(){
    GET_PIPELINE(P);
    std::vector<Triangle*>::iterator it;
    for(it = P->triangle_list.begin(); it!=P->triangle_list.end(); it++){
        delete (*it);
    }
    P->triangle_list.clear();
    while (!(P->triangle_stream.empty())){
        delete P->triangle_stream.front();
        P->triangle_stream.pop();
    }
}

void* _thr_rasterize(void* thread_id)
{
    int id = (int)(long long)(thread_id);
    GET_CURRENT_CONTEXT(C);
    GET_PIPELINE(P);
    int bin_idx;
    Bin* cur_bin;
    int pixel_end_x; // pixel index should be < than that 
    int pixel_end_y;
    uint64_t bit_mask = 1;
    glProgram local_shader = C->shader;
    while (!quit_rasterizing){
        // start of one frame
        float* zbuf = (float*)C->zbuf->getDataPtr();
        color_t* frame_buf = (color_t*)C->framebuf->getDataPtr();
        bin_idx = id;
        // loop over the bin tasks (of the whole screen) belonging to each thread
        while (1){
            // grab one bin to rasterize
            // cur_bin = P->bins->get_bin_by_index(bin_idx);
            cur_bin = P->bins->get_non_empty_bin(bin_idx);
            if (cur_bin == nullptr)
                break;
            pixel_end_x = cur_bin->pixel_bin_x + BIN_SIDE_LENGTH;
            pixel_end_y = cur_bin->pixel_bin_y + BIN_SIDE_LENGTH;
            if (!cur_bin->is_full){
                pixel_end_x = MIN(pixel_end_x, C->width);
                pixel_end_y = MIN(pixel_end_y, C->height);
            }
            // check point
            // std::cout<<"bin: "<<bin_idx<<" with pos: "<<cur_bin->pixel_bin_x<<" "<<cur_bin->pixel_bin_y<<"  ";
            // std::cout<<"has triangles: "<<cur_bin->tasks.size()<<"  ";
            // std::cout<<"is full: "<<cur_bin->is_full<<std::endl;
            // loop over the triangles belong to it
            while (! cur_bin->tasks.empty()){
                primitive_t pri = cur_bin->tasks.front();
                uint64_t cover_mask = pri.cover_mask;
                int cnt = 0;
                while (cover_mask != 0){
                    if (cover_mask & bit_mask){
                        // process pixels in the tile
                        int x, y, tile_pixel_begin_x, tile_pixel_begin_y, tile_pixel_end_x, tile_pixel_end_y;
                        x = cnt % TILE_NUM_PER_AXIS;
                        y = cnt / TILE_NUM_PER_AXIS;
                        Triangle* t = pri.tri;
                        glm::vec4* screen_pos = t->screen_pos;
                        cur_bin->get_tile(x, y, &tile_pixel_begin_x, &tile_pixel_begin_y);
                        tile_pixel_end_x = MIN(pixel_end_x, tile_pixel_begin_x+TILE_SIDE_LENGTH);
                        tile_pixel_end_y = MIN(pixel_end_y, tile_pixel_begin_y+TILE_SIDE_LENGTH);
                        for(int temp_pixel_x = tile_pixel_begin_x; temp_pixel_x < tile_pixel_end_x; temp_pixel_x++){
                            for (int temp_pixel_y = tile_pixel_begin_y; temp_pixel_y < tile_pixel_end_y; temp_pixel_y++){
                                int index = GET_INDEX(temp_pixel_x, temp_pixel_y, C->width, C->height);
                                if (!t->inside(temp_pixel_x+0.5f, temp_pixel_y+0.5f))
                                    continue;
                                glm::vec3 coef = t->computeBarycentric2D(temp_pixel_x + 0.5f, temp_pixel_y + 0.5f);
                                // perspective correction
                                float Z_viewspace = 1.0 / (coef[0] / screen_pos[0].w + coef[1] / screen_pos[1].w + coef[2] / screen_pos[2].w);
                                float alpha = coef[0] * Z_viewspace / screen_pos[0].w;
                                float beta = coef[1] * Z_viewspace / screen_pos[1].w;
                                float gamma = coef[2] * Z_viewspace / screen_pos[2].w;
                                if (!C->use_z_test) {
                                    throw std::runtime_error("please open the z depth test\n");
                                }else{
                                    float zp = alpha * screen_pos[0].z + beta * screen_pos[1].z + gamma * screen_pos[2].z;
                                    if (zp < zbuf[index]) {
                                        zbuf[index] = zp;
                                        // fragment shader input
                                        local_shader.diffuse_Color = interpolate(alpha, beta, gamma, t->color[0], t->color[1], t->color[2], 1);
                                        local_shader.texcoord = interpolate(alpha, beta, gamma, t->texcoord[0], t->texcoord[1], t->texcoord[2], 1);
                                        // fragment shading
                                        local_shader.default_fragment_shader();
                                        frame_buf[index].R = local_shader.frag_Color.x * 255.0f;
                                        frame_buf[index].G = local_shader.frag_Color.y * 255.0f;
                                        frame_buf[index].B = local_shader.frag_Color.z * 255.0f;
                                    }
                                } 
                            }
                        }
                    }
                    cnt ++;
                    cover_mask = cover_mask >> 1;
                }
                cur_bin->tasks.pop();
            }
            bin_idx += RASTERIZE_THREAD_COUNT;
        }
        // std::cout<<"partially finish rasterizing"<<std::endl;
        // finish rasterizing, sync point
        pthread_mutex_lock(&rasterize_mtx);
        rasterize_sync ++;
        if(rasterize_sync == RASTERIZE_THREAD_COUNT){
            rasterize_sync = 0;
            // _free_triangles();
            pthread_mutex_lock(&pipeline_mtx);
            pipeline_stage = PIPELINE_FINISH;
            pthread_cond_signal(&pipeline_cv);
            pthread_mutex_unlock(&pipeline_mtx);
            while (pthread_cond_wait(&rasterize_cv, &rasterize_mtx) != 0);
        }else{
            while(pthread_cond_wait(&rasterize_cv, &rasterize_mtx) != 0);
        }
        pthread_mutex_unlock(&rasterize_mtx);
    }
    return nullptr;
}


