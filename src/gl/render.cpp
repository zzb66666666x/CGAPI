#include "render.h"
#include "../../include/gl/common.h"
#include "binning.h"
#include "configs.h"
#include "glcontext.h"
#include <assert.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <omp.h>
#include <pthread.h>
#include <set>
#include <unordered_map>

#define GET_PIPELINE(P) glPipeline* P = &(glapi_ctx->pipeline)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define GET_INDEX(x, y, width, height) (((height) - 1 - (y)) * (width) + (x))

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

#define GENERAL_INTERP(alpha, beta, gamma, vert1, vert2, vert3, weight) ((alpha * vert1 + beta * vert2 + gamma * vert3) / weight)

/////////////////////////////////////// face culling function //////////////////////////////////
inline static void backface_culling(Triangle& t)
{
    glm::vec3 v01 = t.screen_pos[1] - t.screen_pos[0];
    glm::vec3 v02 = t.screen_pos[2] - t.screen_pos[0];
    glm::vec3 res = glm::normalize(glm::cross(v01, v02));

    // the parallel plane will not be culled.
    t.culling = res.z < -0.01f;
}

/////////////////////////////////////// view frustum culling function //////////////////////////////////
static const std::vector<glm::vec4> planes = {
    //Near
    glm::vec4(0, 0, 1, -1),
    //far
    glm::vec4(0, 0, -1, -1),
    //left
    glm::vec4(-1, 0, 0, -1),
    //top
    glm::vec4(0, 1, 0, -1),
    //right
    glm::vec4(1, 0, 0, -1),
    //bottom
    glm::vec4(0, -1, 0, -1)
};

static bool outside_clip_space(Triangle& t)
{
    for (int i = 0; i < 3; ++i) {
        if ((t.screen_pos[0][i] > t.screen_pos[0].w && t.screen_pos[1][i] > t.screen_pos[1].w && t.screen_pos[2][i] > t.screen_pos[2].w)
            || (t.screen_pos[0][i] < -t.screen_pos[0].w && t.screen_pos[1][i] < -t.screen_pos[1].w && t.screen_pos[2][i] < -t.screen_pos[2].w)) {
            return true;
        }
    }
    return false;
}

static bool all_inside_clip_space(Triangle& t)
{
    for (int i = 0; i < 3; ++i) {
        if (!(t.screen_pos[i].x < t.screen_pos[i].w && t.screen_pos[i].x > -t.screen_pos[i].w
                && t.screen_pos[i].y < t.screen_pos[i].w && t.screen_pos[i].y > -t.screen_pos[i].w
                && t.screen_pos[i].z < t.screen_pos[i].w && t.screen_pos[i].z > -t.screen_pos[i].w)) {
            return false;
        }
    }
    return true;
}

static bool inside_plane(const glm::vec4& plane, glm::vec4& pos)
{
    return glm::dot(pos, plane) <= 0;
}

static Vertex intersect(Vertex& v1, Vertex& v2, const glm::vec4& plane)
{
    float d1 = glm::dot(v1.screen_pos, plane);
    float d2 = glm::dot(v2.screen_pos, plane);
    float weight = d1 / (d1 - d2);
    return Vertex::lerp(v1, v2, weight);
}

static void view_frustum_culling(Triangle& t, std::vector<Triangle*>& result_list)
{
    // if all vertices are outside or inside, it will return directly.
    if ((t.culling = outside_clip_space(t)) || all_inside_clip_space(t)) {
        return;
    }
    // for loop
    int i, j, len, jlen;
    // current triangle will be culling.
    t.culling = true;

    std::vector<Vertex> vertex_list(3);
    for (int i = 0; i < 3; ++i) {
        // vertex_list[i] = t.getVertex(i);
        vertex_list[i].screen_pos = t.screen_pos[i];
        vertex_list[i].color = t.color[i];
        vertex_list[i].frag_shading_pos = t.frag_shading_pos[i];
        vertex_list[i].vert_normal = t.vert_normal[i];
        vertex_list[i].texcoord = t.texcoord[i];
    }

    for (i = 0, len = planes.size(); i < len; ++i) {
        std::vector<Vertex> input(vertex_list);
        vertex_list.clear();
        for (j = 0, jlen = input.size(); j < jlen; ++j) {
            Vertex& current = input[j];
            Vertex& last = input[(j + jlen - 1) % jlen];
            if (inside_plane(planes[i], current.screen_pos)) {
                if (!inside_plane(planes[i], last.screen_pos)) {
                    Vertex intersecting = intersect(last, current, planes[i]);
                    vertex_list.push_back(intersecting);
                }
                vertex_list.push_back(current);
            } else if (inside_plane(planes[i], last.screen_pos)) {
                Vertex intersecting = intersect(last, current, planes[i]);
                vertex_list.push_back(intersecting);
            }
        }
    }
    // printf("vertex_list size: %d\n", vertex_list.size());
    if (vertex_list.size() < 3) {
        return;
    }

    Triangle* tri = nullptr;
    result_list.resize(vertex_list.size() - 3 + 1);
    int next = 1;
    for (i = 0, len = result_list.size(); i < len; ++i) {
        tri = new Triangle();
        tri->setVertex(0, vertex_list[0]);
        tri->setVertex(1, vertex_list[next]);
        tri->setVertex(2, vertex_list[++next]);
        result_list[i] = tri;
    }
}

///////////////////////////// SINGLE-THREAD VERSION OF RENDERING ////////////////////////////
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
void rasterize_with_shading()
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
                } else {
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

    std::vector<int>* indices;
    int triangle_size = 0;
    if (ppl->use_indices) {
        int vaoId = ctx->payload.renderMap[GL_ARRAY_BUFFER];
        int ret = ppl->indexCache.getCacheData(vaoId, &indices);
        if (ppl->ebo_config.ebo_ptr->usage == GL_STATIC_DRAW && ppl->vbo_ptr->usage == GL_STATIC_DRAW && ret == GL_FALSE) {
            std::vector<int> innerIndices;
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
                innerIndices.resize(ebuf_size - first_index);
#ifdef GL_PARALLEL_OPEN
#pragma omp parallel for
#endif
                for (int i = first_index; i < ebuf_size; ++i) {
                    innerIndices[i] = ebuf_data[i];
                }
                triangle_size = (ebuf_size - first_index) / 3;
            } break;
            default:
                break;
            }
            ppl->indexCache.addCacheData(vaoId, innerIndices);
            ppl->indexCache.getCacheData(vaoId, &indices);
        } else {
            triangle_size = indices->size() / 3;
        }

    } else {
        int vaoId = ctx->payload.renderMap[GL_ARRAY_BUFFER];
        int ret = ppl->indexCache.getCacheData(vaoId, &indices);
        if (ppl->vbo_ptr->usage == GL_STATIC_DRAW && ret == GL_FALSE) {
            std::vector<int> innerIndices;
            int first_vertex_ind = ppl->first_vertex;
            int vertex_num = MIN(ppl->vertex_num, ppl->vbo_ptr->getSize() / vattrib_data[0].stride);

            // case: ((38 - 33) / 3) * 3 + 33 == 36, first_vertex_ind == 33
            vertex_num = ((vertex_num - first_vertex_ind) / 3) * 3 + first_vertex_ind;
            innerIndices.resize(vertex_num - first_vertex_ind);

            int len = vertex_num - first_vertex_ind;
#ifdef GL_PARALLEL_OPEN
#pragma omp parallel for
#endif
            for (int i = 0; i < len; ++i) {
                innerIndices[i] = first_vertex_ind + i;
                // printf("i=%d. Hello! threadID=%d  thraed number:%d\n", i, omp_get_thread_num(), omp_get_num_threads());
            }
            triangle_size = (vertex_num - first_vertex_ind) / 3;
            ppl->indexCache.addCacheData(vaoId, innerIndices);
            ppl->indexCache.getCacheData(vaoId, &indices);
        } else {
            triangle_size = indices->size() / 3;
        }
    }

    unsigned char* vbuf_data = (unsigned char*)ppl->vbo_ptr->getDataPtr();

    std::vector<Triangle*>& triangle_list = ppl->triangle_list;

    // check and delete
    if (triangle_list.size() < triangle_size) {
        int tsize = triangle_list.size();
        triangle_list.resize(triangle_size);
        for (int i = tsize, len = triangle_size; i < len; ++i) {
            triangle_list[i] = new Triangle();
        }
    } else if (triangle_list.size() > triangle_size) {
        for (int i = triangle_size, len = triangle_list.size(); i < len; ++i) {
            delete triangle_list[i];
        }
        triangle_list.resize(triangle_size);
    }

    angle = angle + 1.0f;

    std::vector<Triangle*>& tri_culling_list = ppl->tri_culling_list;

    // begin parallel block
    void* input_ptr;
    unsigned char* buf;
    glProgram shader = ctx->shader;
    int i, j;
#ifdef GL_PARALLEL_OPEN
#pragma omp parallel for private(input_ptr) private(buf) private(shader) private(i) private(j)
#endif
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
                buf = vbuf_data + (size_t)((*indices)[tri_ind * 3 + i] * config.stride) + (size_t)config.pointer;
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
            // printf("i: %d\n", i);
            // assemble triangle
            triangle_list[tri_ind]->screen_pos[i] = shader.gl_Position;
            triangle_list[tri_ind]->color[i] = shader.gl_VertexColor;
            triangle_list[tri_ind]->frag_shading_pos[i] = shader.frag_Pos;
            triangle_list[tri_ind]->texcoord[i] = shader.iTexcoord;
            triangle_list[tri_ind]->vert_normal[i] = shader.gl_Normal;
        }

        // view frustum culling list
        std::vector<Triangle*> vfc_list;
        view_frustum_culling(*triangle_list[tri_ind], vfc_list);
        if (vfc_list.size() != 0) {
            if (ctx->cull_face.open) {
                omp_set_lock(&ppl->tri_culling_lock);
                for (int tind = 0, tlen = vfc_list.size(); tind < tlen; ++tind) {
                    backface_culling(*vfc_list[tind]);
                    if (vfc_list[tind]->culling) {
                        delete vfc_list[tind];
                    } else {
                        tri_culling_list.push_back(vfc_list[tind]);
                    }
                }
                omp_unset_lock(&ppl->tri_culling_lock);
            } else {
                omp_set_lock(&ppl->tri_culling_lock);
                tri_culling_list.insert(tri_culling_list.end(), vfc_list.begin(), vfc_list.end());
                omp_unset_lock(&ppl->tri_culling_lock);
            }
        } else if (ctx->cull_face.open && !triangle_list[tri_ind]->culling) {
            backface_culling(*triangle_list[tri_ind]);
        }

        if (triangle_list[tri_ind]->culling) {
            continue;
        }
        for (i = 0; i < 3; ++i) {
            triangle_list[tri_ind]->screen_pos[i].x /= triangle_list[tri_ind]->screen_pos[i].w;
            triangle_list[tri_ind]->screen_pos[i].y /= triangle_list[tri_ind]->screen_pos[i].w;
            triangle_list[tri_ind]->screen_pos[i].z /= triangle_list[tri_ind]->screen_pos[i].w;

            // view port transformation
            triangle_list[tri_ind]->screen_pos[i].x = 0.5 * ctx->width * (triangle_list[tri_ind]->screen_pos[i].x + 1.0);
            triangle_list[tri_ind]->screen_pos[i].y = 0.5 * ctx->height * (triangle_list[tri_ind]->screen_pos[i].y + 1.0);

            // [-1,1] to [0,1]
            triangle_list[tri_ind]->screen_pos[i].z = triangle_list[tri_ind]->screen_pos[i].z * 0.5 + 0.5;
        }
    }

    // merge triangle_list and tri_culling_list
    int ind = triangle_list.size();
    triangle_list.resize(triangle_list.size() + tri_culling_list.size());
    for (int tri_ind = 0, len = tri_culling_list.size(); tri_ind < len; ++tri_ind) {
        for (i = 0; i < 3; ++i) {
            tri_culling_list[tri_ind]->screen_pos[i].x /= tri_culling_list[tri_ind]->screen_pos[i].w;
            tri_culling_list[tri_ind]->screen_pos[i].y /= tri_culling_list[tri_ind]->screen_pos[i].w;
            tri_culling_list[tri_ind]->screen_pos[i].z /= tri_culling_list[tri_ind]->screen_pos[i].w;

            // view port transformation
            tri_culling_list[tri_ind]->screen_pos[i].x = 0.5 * ctx->width * (tri_culling_list[tri_ind]->screen_pos[i].x + 1.0);
            tri_culling_list[tri_ind]->screen_pos[i].y = 0.5 * ctx->height * (tri_culling_list[tri_ind]->screen_pos[i].y + 1.0);

            // [-1,1] to [0,1]
            tri_culling_list[tri_ind]->screen_pos[i].z = tri_culling_list[tri_ind]->screen_pos[i].z * 0.5 + 0.5;
        }
        triangle_list[ind + tri_ind] = tri_culling_list[tri_ind];
    }
    ppl->tri_culling_list.clear();
}

void rasterize_with_shading_openmp()
{
    GET_CURRENT_CONTEXT(ctx);
    std::vector<Triangle*>& triangle_list = ctx->pipeline.triangle_list;
    int width = ctx->width, height = ctx->height;
    std::vector<Pixel>& pixel_tasks = ctx->pipeline.pixel_tasks;

    color_t* frame_buf = (color_t*)ctx->framebuf->getDataPtr();

    Triangle* t = nullptr;
    int len = triangle_list.size();
    float* zbuf = (float*)ctx->zbuf->getDataPtr();
    glProgram shader = ctx->shader;
#ifdef GL_PARALLEL_OPEN
#pragma omp parallel for private(t) private(shader)
#endif
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

#if 1
        // view shrinking in rasterization
        minx = minx < 0 ? 0 : minx;
        miny = miny < 0 ? 0 : miny;
        maxx = maxx >= width ? width - 1 : maxx;
        maxy = maxy >= height ? height - 1 : maxy;
#endif

        // AABB algorithm
        for (y = miny; y <= maxy; ++y) {
            for (x = minx; x <= maxx; ++x) {
                int index = GET_INDEX(x, y, width, height);
                if (!t->inside(x + 0.5f, y + 0.5f))
                    continue;

                // alpha beta gamma
                glm::vec3 coef = t->computeBarycentric2D(x + 0.5f, y + 0.5f);
                // perspective correction
                // float Z_viewspace = 1.0 / (coef[0] / screen_pos[0].w + coef[1] / screen_pos[1].w + coef[2] / screen_pos[2].w);
                // float alpha = coef[0] * Z_viewspace / screen_pos[0].w;
                // float beta = coef[1] * Z_viewspace / screen_pos[1].w;
                // float gamma = coef[2] * Z_viewspace / screen_pos[2].w;

                float zp = coef[0] * screen_pos[0].z + coef[1] * screen_pos[1].z + coef[2] * screen_pos[2].z;
                float Z = 1.0 / (coef[0] + coef[1] + coef[2]);

                zp *= Z;

                float alpha = coef[0];
                float beta = coef[1];
                float gamma = coef[2];

                if (!ctx->use_z_test) {
                    throw std::runtime_error("please open the z depth test\n");
                } else {
                    // zp: z value after interpolation
                    // float zp = alpha * screen_pos[0].z + beta * screen_pos[1].z + gamma * screen_pos[2].z;
                    omp_set_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                    if (zp < zbuf[index]) {
                        zbuf[index] = zp;
                        // pixel_tasks[index].vertexColor = interpolate(alpha, beta, gamma, t->color[0], t->color[1], t->color[2], 1);
                        // pixel_tasks[index].texcoord = interpolate(alpha, beta, gamma, t->texcoord[0], t->texcoord[1], t->texcoord[2], 1);
                        // pixel_tasks[index].normal = interpolate(alpha, beta, gamma, t->vert_normal[0], t->vert_normal[1], t->vert_normal[2], 1);
                        // omp_unset_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                        // // all threads always make it 'true';
                        // pixel_tasks[index].write = true;

                        // fragment shader input
                        shader.diffuse_Color = interpolate(alpha, beta, gamma, t->color[0], t->color[1], t->color[2], 1);
                        shader.texcoord = interpolate(alpha, beta, gamma, t->texcoord[0], t->texcoord[1], t->texcoord[2], 1);
                        shader.normal = interpolate(alpha, beta, gamma, t->vert_normal[0], t->vert_normal[1], t->vert_normal[2], 1);
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

void process_pixel_openmp()
{
    GET_CURRENT_CONTEXT(ctx);
    GET_PIPELINE(ppl);
    std::vector<Pixel>& pixel_tasks = ppl->pixel_tasks;
    color_t* frame_buf = (color_t*)ctx->framebuf->getDataPtr();
    int len = pixel_tasks.size();
    glProgram shader = ctx->shader;
#pragma omp parallel for private(shader)
    for (int i = 0; i < len; ++i) {
        if (pixel_tasks[i].write) {
            shader.diffuse_Color = pixel_tasks[i].vertexColor;
            shader.texcoord = pixel_tasks[i].texcoord;
            shader.normal = pixel_tasks[i].normal;
            shader.default_fragment_shader();
            frame_buf[i].R = shader.frag_Color.x * 255;
            frame_buf[i].G = shader.frag_Color.y * 255;
            frame_buf[i].B = shader.frag_Color.z * 255;
            pixel_tasks[i].write = false;
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
    C->threads.reset();
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
        crawlerID = (crawlerID + 1) % PROCESS_VERTEX_THREAD_COUNT;
        if (cnt % 3 == 0 && cnt > 0) {
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
            while (pthread_cond_wait(&vertex_threads_cv, &vertex_threads_mtx) != 0)
                ;
        } else {
            while (pthread_cond_wait(&vertex_threads_cv, &vertex_threads_mtx) != 0)
                ;
        }
        pthread_mutex_unlock(&vertex_threads_mtx);
        // 5. reset the crawler and ready for next frame
        crawlers[id].reset_config();
        crawlers[id].reset_data();
    }
    return nullptr;
}

void binning_threadmain()
{
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

void* _thr_binning(void* thread_id)
{
    int id = (int)(long long)(thread_id);
    GET_PIPELINE(P);
    Triangle* cur_tri;
    int tri_idx;
    while (!quit_binning) {
        // start of each frame
        tri_idx = id;
        while (1) {
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
            if (tri_idx >= P->triangle_list.size()) {
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
            for (it = overlap_bins.begin(); it != overlap_bins.end(); it++) {
                Bin* temp_bin = *it;
                uint64_t mask = compute_cover_mask(cur_tri, temp_bin);
                if (mask != 0) {
                    pthread_mutex_lock(&temp_bin->lock);
                    temp_bin->tasks.push((primitive_t) { cur_tri, mask });
                    pthread_mutex_unlock(&temp_bin->lock);
                }
            }
        }
        // std::cout<<"partially finish binning"<<std::endl;
        // finish binning, sleep
        pthread_mutex_lock(&binning_mtx);
        binning_sync++;
        if (binning_sync == BINNING_THREAD_COUNT) {
            binning_sync = 0;
            // calculate bins with non-zero triangles
            P->bins->prepare_non_empty_bins();
            // move the pipeline forward
            pthread_mutex_lock(&pipeline_mtx);
            pipeline_stage = DOING_RASTERIZATION;
            pthread_cond_signal(&pipeline_cv);
            pthread_mutex_unlock(&pipeline_mtx);
            while (pthread_cond_wait(&binning_cv, &binning_mtx) != 0)
                ;
        } else {
            while (pthread_cond_wait(&binning_cv, &binning_mtx) != 0)
                ;
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

static inline void _free_triangles()
{
    GET_PIPELINE(P);
    std::vector<Triangle*>::iterator it;
    for (it = P->triangle_list.begin(); it != P->triangle_list.end(); it++) {
        delete (*it);
    }
    P->triangle_list.clear();
    while (!(P->triangle_stream.empty())) {
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
    while (!quit_rasterizing) {
        // start of one frame
        float* zbuf = (float*)C->zbuf->getDataPtr();
        color_t* frame_buf = (color_t*)C->framebuf->getDataPtr();
        bin_idx = id;
        // loop over the bin tasks (of the whole screen) belonging to each thread
        while (1) {
            // grab one bin to rasterize
            // cur_bin = P->bins->get_bin_by_index(bin_idx);
            cur_bin = P->bins->get_non_empty_bin(bin_idx);
            if (cur_bin == nullptr)
                break;
            pixel_end_x = cur_bin->pixel_bin_x + BIN_SIDE_LENGTH;
            pixel_end_y = cur_bin->pixel_bin_y + BIN_SIDE_LENGTH;
            if (!cur_bin->is_full) {
                pixel_end_x = MIN(pixel_end_x, C->width);
                pixel_end_y = MIN(pixel_end_y, C->height);
            }
            // check point
            // std::cout<<"bin: "<<bin_idx<<" with pos: "<<cur_bin->pixel_bin_x<<" "<<cur_bin->pixel_bin_y<<"  ";
            // std::cout<<"has triangles: "<<cur_bin->tasks.size()<<"  ";
            // std::cout<<"is full: "<<cur_bin->is_full<<std::endl;
            // loop over the triangles belong to it
            while (!cur_bin->tasks.empty()) {
                primitive_t pri = cur_bin->tasks.front();
                uint64_t cover_mask = pri.cover_mask;
                int cnt = 0;
                while (cover_mask != 0) {
                    if (cover_mask & bit_mask) {
                        // process pixels in the tile
                        int x, y, tile_pixel_begin_x, tile_pixel_begin_y, tile_pixel_end_x, tile_pixel_end_y;
                        x = cnt % TILE_NUM_PER_AXIS;
                        y = cnt / TILE_NUM_PER_AXIS;
                        Triangle* t = pri.tri;
                        glm::vec4* screen_pos = t->screen_pos;
                        cur_bin->get_tile(x, y, &tile_pixel_begin_x, &tile_pixel_begin_y);
                        tile_pixel_end_x = MIN(pixel_end_x, tile_pixel_begin_x + TILE_SIDE_LENGTH);
                        tile_pixel_end_y = MIN(pixel_end_y, tile_pixel_begin_y + TILE_SIDE_LENGTH);
                        for (int temp_pixel_x = tile_pixel_begin_x; temp_pixel_x < tile_pixel_end_x; temp_pixel_x++) {
                            for (int temp_pixel_y = tile_pixel_begin_y; temp_pixel_y < tile_pixel_end_y; temp_pixel_y++) {
                                int index = GET_INDEX(temp_pixel_x, temp_pixel_y, C->width, C->height);
                                if (!t->inside(temp_pixel_x + 0.5f, temp_pixel_y + 0.5f))
                                    continue;
                                glm::vec3 coef = t->computeBarycentric2D(temp_pixel_x + 0.5f, temp_pixel_y + 0.5f);
                                // perspective correction
                                float Z_viewspace = 1.0 / (coef[0] / screen_pos[0].w + coef[1] / screen_pos[1].w + coef[2] / screen_pos[2].w);
                                float alpha = coef[0] * Z_viewspace / screen_pos[0].w;
                                float beta = coef[1] * Z_viewspace / screen_pos[1].w;
                                float gamma = coef[2] * Z_viewspace / screen_pos[2].w;
                                if (!C->use_z_test) {
                                    throw std::runtime_error("please open the z depth test\n");
                                } else {
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
                    cnt++;
                    cover_mask = cover_mask >> 1;
                }
                cur_bin->tasks.pop();
            }
            bin_idx += RASTERIZE_THREAD_COUNT;
        }
        // std::cout<<"partially finish rasterizing"<<std::endl;
        // finish rasterizing, sync point
        pthread_mutex_lock(&rasterize_mtx);
        rasterize_sync++;
        if (rasterize_sync == RASTERIZE_THREAD_COUNT) {
            rasterize_sync = 0;
            // _free_triangles();
            pthread_mutex_lock(&pipeline_mtx);
            pipeline_stage = PIPELINE_FINISH;
            pthread_cond_signal(&pipeline_cv);
            pthread_mutex_unlock(&pipeline_mtx);
            while (pthread_cond_wait(&rasterize_cv, &rasterize_mtx) != 0)
                ;
        } else {
            while (pthread_cond_wait(&rasterize_cv, &rasterize_mtx) != 0)
                ;
        }
        pthread_mutex_unlock(&rasterize_mtx);
    }
    return nullptr;
}

///////////////////////////// PROGRAMMABLE VERSION WITH OPENMP ////////////////////////////
static void programmable_interpolate(Shader* shader_ptr, ProgrammableTriangle* t, float alpha, float beta, float gamma, std::map<std::string, data_t>& target)
{
    for (auto it = (t->vertex_attribs)[0].begin(); it != (t->vertex_attribs)[0].end(); ++it) {
        int dtype = shader_ptr->io_profile[it->first].dtype;
        data_t interp_data;
        switch (dtype) {
            case TYPE_VEC2:
                interp_data.vec2_var = GENERAL_INTERP(alpha, beta, gamma, (t->vertex_attribs)[0][it->first].vec2_var, (t->vertex_attribs)[1][it->first].vec2_var, t->vertex_attribs[2][it->first].vec2_var, 1.0f);
                break;
            case TYPE_VEC3:
                interp_data.vec3_var = GENERAL_INTERP(alpha, beta, gamma, (t->vertex_attribs)[0][it->first].vec3_var, (t->vertex_attribs)[1][it->first].vec3_var, t->vertex_attribs[2][it->first].vec3_var, 1.0f);
                break;
            case TYPE_VEC4:
                interp_data.vec4_var = GENERAL_INTERP(alpha, beta, gamma, (t->vertex_attribs)[0][it->first].vec4_var, (t->vertex_attribs)[1][it->first].vec4_var, t->vertex_attribs[2][it->first].vec4_var, 1.0f);
                break;
            default:
                throw std::runtime_error("don't interp on these types now\n");
        }
        target.emplace(it->first, interp_data);
    }
}

// static void programmable_interpolate(Shader* shader_ptr, ProgrammableTriangle* t, float alpha, float beta, float gamma, std::unordered_map<std::string, data_t>& target)
// {
//     for (auto it = (t->vertex_attribs)[0].begin(); it != (t->vertex_attribs)[0].end(); ++it) {
//         int dtype = shader_ptr->io_profile[it->first].dtype;
//         data_t interp_data;
//         switch (dtype) {
//             case TYPE_VEC2:
//                 interp_data.vec2_var = GENERAL_INTERP(alpha, beta, gamma, (t->vertex_attribs)[0][it->first].vec2_var, (t->vertex_attribs)[1][it->first].vec2_var, t->vertex_attribs[2][it->first].vec2_var, 1.0f);
//                 break;
//             case TYPE_VEC3:
//                 interp_data.vec3_var = GENERAL_INTERP(alpha, beta, gamma, (t->vertex_attribs)[0][it->first].vec3_var, (t->vertex_attribs)[1][it->first].vec3_var, t->vertex_attribs[2][it->first].vec3_var, 1.0f);
//                 break;
//             case TYPE_VEC4:
//                 interp_data.vec4_var = GENERAL_INTERP(alpha, beta, gamma, (t->vertex_attribs)[0][it->first].vec4_var, (t->vertex_attribs)[1][it->first].vec4_var, t->vertex_attribs[2][it->first].vec4_var, 1.0f);
//                 break;
//             default:
//                 throw std::runtime_error("don't interp on these types now\n");
//         }
//         target.emplace(it->first, interp_data);
//     }
// }

static void programmable_view_port(ProgrammableTriangle* t)
{
    GET_CURRENT_CONTEXT(ctx);
    for (int i = 0; i < 3;++i){
        t->w_inversed[i] = 1.0f / t->screen_pos[i].w;
        t->screen_pos[i].x *= t->w_inversed[i];
        t->screen_pos[i].y *= t->w_inversed[i];
        t->screen_pos[i].z *= t->w_inversed[i];
        // printf("1/w: %f, x: %f, y: %f, z: %f\n", t->screen_pos[i].w, t->screen_pos[i].x, t->screen_pos[i].y, t->screen_pos[i].z);

        // view port transformation
        t->screen_pos[i].x = 0.5f * ctx->width * (t->screen_pos[i].x + 1.0f);
        t->screen_pos[i].y = 0.5f * ctx->height * (t->screen_pos[i].y + 1.0f);

        // [-1,1] to [0,1]
        t->screen_pos[i].z = t->screen_pos[i].z * 0.5f + 0.5f;
    }
}

static std::vector<int>* programmable_parse_indices(int &triangle_size)
{
    GET_CURRENT_CONTEXT(ctx);

    vertex_attrib_t* vattrib_data = (vertex_attrib_t*)((vertex_array_object_t*)ctx->pipeline.vao_ptr->getDataPtr())->attribs;

    std::vector<int>* indices = nullptr;

    if (ctx->pipeline.use_indices) {
        int vaoId = ctx->payload.renderMap[GL_ARRAY_BUFFER];
        int ret = ctx->pipeline.indexCache.getCacheData(vaoId, &indices);
        if (ctx->pipeline.ebo_config.ebo_ptr->usage == GL_STATIC_DRAW && ctx->pipeline.vbo_ptr->usage == GL_STATIC_DRAW && ret == GL_FALSE) {
            std::vector<int> innerIndices;
            // first ebo data index
            const void* first_indices = (const void*)ctx->pipeline.ebo_config.first_indices;
            switch (ctx->pipeline.ebo_config.indices_type) {
                case GL_UNSIGNED_INT: {
                    // ebo data array
                    unsigned int* ebuf_data = (unsigned int*)ctx->pipeline.ebo_config.ebo_ptr->getDataPtr();
                    int first_index = (size_t)first_indices / sizeof(unsigned int);
                    int ebuf_size = MIN(ctx->pipeline.vertex_num, ctx->pipeline.ebo_config.ebo_ptr->getSize());
                    // case: ((6 - 1) / 3) * 3 + 1 == 4 , first_index == 1
                    ebuf_size = ((ebuf_size - first_index) / 3) * 3 + first_index;
                    // vertex_num = ((vertex_num - first_vertex_ind) % 3) * 3 + first_vertex_ind;
                    innerIndices.resize(ebuf_size - first_index);
#ifdef GL_PARALLEL_OPEN
#pragma omp parallel for
#endif
                    for (int i = first_index; i < ebuf_size; ++i) {
                        innerIndices[i] = ebuf_data[i];
                    }
                    triangle_size = (ebuf_size - first_index) / 3;
                } break;
                default:
                    break;
            }
            ctx->pipeline.indexCache.addCacheData(vaoId, innerIndices);
            ctx->pipeline.indexCache.getCacheData(vaoId, &indices);
        } else {
            triangle_size = indices->size() / 3;
        }

    } else {
        int vaoId = ctx->payload.renderMap[GL_ARRAY_BUFFER];
        int ret = ctx->pipeline.indexCache.getCacheData(vaoId, &indices);
        if (ctx->pipeline.vbo_ptr->usage == GL_STATIC_DRAW && ret == GL_FALSE) {
            std::vector<int> innerIndices;
            int first_vertex_ind = ctx->pipeline.first_vertex;
            int vertex_num = MIN(ctx->pipeline.vertex_num, ctx->pipeline.vbo_ptr->getSize() / vattrib_data[0].stride);

            // case: ((38 - 33) / 3) * 3 + 33 == 36, first_vertex_ind == 33
            vertex_num = ((vertex_num - first_vertex_ind) / 3) * 3 + first_vertex_ind;
            innerIndices.resize(vertex_num - first_vertex_ind);

            int len = vertex_num - first_vertex_ind;
#ifdef GL_PARALLEL_OPEN
#pragma omp parallel for
#endif
            for (int i = 0; i < len; ++i) {
                innerIndices[i] = first_vertex_ind + i;
                // printf("i=%d. Hello! threadID=%d  thraed number:%d\n", i, omp_get_thread_num(), omp_get_num_threads());
            }
            triangle_size = (vertex_num - first_vertex_ind) / 3;
            ctx->pipeline.indexCache.addCacheData(vaoId, innerIndices);
            ctx->pipeline.indexCache.getCacheData(vaoId, &indices);
        } else {
            triangle_size = indices->size() / 3;
        }
    }

    return indices;
}

void programmable_process_geometry_openmp()
{
    /**
     * input assembly
     */
    GET_PIPELINE(ppl);
    GET_CURRENT_CONTEXT(ctx);

    vertex_attrib_t* vattrib_data = (vertex_attrib_t*)((vertex_array_object_t *)ppl->vao_ptr->getDataPtr())->attribs;

    int triangle_size = 0;
    std::vector<int> *indices = programmable_parse_indices(triangle_size);
    unsigned char* vbuf_data = (unsigned char*)ppl->vbo_ptr->getDataPtr();

    std::vector<ProgrammableTriangle*>& triangle_list = ppl->prog_triangle_list;

    // triangle list management
    if (triangle_list.size() < triangle_size) {
        int tsize = triangle_list.size();
        triangle_list.resize(triangle_size);
        for (int i = tsize, len = triangle_size; i < len; ++i) {
            triangle_list[i] = new ProgrammableTriangle();
        }
    } 
    // else if (triangle_list.size() > triangle_size) {
    //     for (int i = triangle_size, len = triangle_list.size(); i < len; ++i) {
    //         delete triangle_list[i];
    //     }
    //     triangle_list.resize(triangle_size);
    // }

    Shader* vert_shader = ctx->payload.cur_shader_program_ptr->get_shader(GL_VERTEX_SHADER);
    
    // std::vector<ProgrammableTriangle*>& tri_culling_list = ppl->prog_tri_culling_list;

    std::vector<std::vector<ProgrammableTriangle*>> tri_cullings;
    std::vector<ShaderInterface*> shader_interfaces;
    shader_interfaces.resize(ppl->cpu_num);
    tri_cullings.resize(ppl->cpu_num);
    for (int i = 0; i < ppl->cpu_num; i++) {
        shader_interfaces[i] = vert_shader->get_shader_utils(i);
    }

    // begin parallel block
    // std::unordered_map<std::string, data_t> vs_input, vs_output;
    std::map<std::string, data_t> vs_input, vs_output;
    void* input_ptr;
    unsigned char* buf;
    int thread_id;
#ifdef GL_PARALLEL_OPEN
#pragma omp parallel for private(input_ptr) private(buf) private(vs_input) private(vs_output) private(thread_id)
#endif
    for (int tri_ind = 0; tri_ind < triangle_size; ++tri_ind) {
        // printf("tri_id=%d. Hello! threadID=%d  thraed number:%d\n", tri_ind, omp_get_thread_num(), omp_get_num_threads());
        // parse data
        thread_id = omp_get_thread_num();
        triangle_list[tri_ind]->cur_shader = vert_shader;
        for (int i = 0; i < 3; ++i) {
            // parse VAO
            for (auto it = vert_shader->layouts.begin(); it != vert_shader->layouts.end(); ++it) {
                vertex_attrib_t& config = vattrib_data[it->second->layout];
                buf = vbuf_data + (size_t)((*indices)[tri_ind * 3 + i] * config.stride) + (size_t)config.pointer;
                if (config.type == GL_FLOAT) {
                    switch (it->second->dtype) {
                        case TYPE_VEC2:
                            vs_input[it->first] = (data_t) { .vec2_var = glm::vec2(*(float*)(buf + 0), *(float*)(buf + sizeof(float) * 1)) };
                            break;
                        case TYPE_VEC3:
                            vs_input[it->first] = (data_t) { .vec3_var = glm::vec3(*(float*)(buf + 0), *(float*)(buf + sizeof(float) * 1), *(float*)(buf + sizeof(float) * 2)) };
                            break;
                        default:
                            break;
                    }
                }
            }

            shader_interfaces[thread_id]->input_port(vs_input);

            // execute vertex shading
            shader_interfaces[thread_id]->glsl_main();
            shader_interfaces[thread_id]->output_port(vs_output);

            // assemble triangle
            data_t gl_pos_inner;
            shader_interfaces[thread_id]->get_inner_variable(INNER_GL_POSITION, gl_pos_inner);
            triangle_list[tri_ind]->screen_pos[i] = gl_pos_inner.vec4_var;
            triangle_list[tri_ind]->vertex_attribs[i] = vs_output;

            vs_input.clear();
            vs_output.clear();
        }

        // view frustum culling list
        std::vector<ProgrammableTriangle*> vfc_list;
        triangle_list[tri_ind]->view_frustum_culling(planes, vfc_list);
        if (vfc_list.size() != 0) {
            // if (ctx->cull_face.open) {
            //     omp_set_lock(&ppl->tri_culling_lock);
            //     for (int tind = 0, tlen = vfc_list.size(); tind < tlen; ++tind) {
            //         // backface_culling(*vfc_list[tind]);
            //         if (vfc_list[tind]->culling) {
            //             delete vfc_list[tind];
            //         } else {
            //             tri_culling_list.push_back(vfc_list[tind]);
            //         }
            //     }
            //     omp_unset_lock(&ppl->tri_culling_lock);
            // } else {
            //     omp_set_lock(&ppl->tri_culling_lock);
            //     tri_culling_list.insert(tri_culling_list.end(), vfc_list.begin(), vfc_list.end());
            //     omp_unset_lock(&ppl->tri_culling_lock);
            // }

            if(ctx->cull_face.open){
                
            }else{
                tri_cullings[thread_id].insert(tri_cullings[thread_id].end(), vfc_list.begin(), vfc_list.end());
            }
        } else if (ctx->cull_face.open && !triangle_list[tri_ind]->culling) {
            // backface_culling(*triangle_list[tri_ind]);
        }

        // vfc_list.clear();

        if (triangle_list[tri_ind]->culling) {
            continue;
        }

        programmable_view_port(triangle_list[tri_ind]);
    }

    // merge triangle_list and tri_culling_list
    // int ind = triangle_list.size();
    // triangle_list.resize(triangle_list.size() + tri_culling_list.size());
    // for (int tri_ind = 0, len = tri_culling_list.size(); tri_ind < len; ++tri_ind) {
    //     for (int i = 0; i < 3; ++i) {
    //         tri_culling_list[tri_ind]->w_inversed[i] = 1.0f / tri_culling_list[tri_ind]->screen_pos[i].w;
    //         tri_culling_list[tri_ind]->screen_pos[i].x *= tri_culling_list[tri_ind]->w_inversed[i];
    //         tri_culling_list[tri_ind]->screen_pos[i].y *= tri_culling_list[tri_ind]->w_inversed[i];
    //         tri_culling_list[tri_ind]->screen_pos[i].z *= tri_culling_list[tri_ind]->w_inversed[i];
    //         // printf("1/w: %f, x: %f, y: %f, z: %f\n", tri_culling_list[tri_ind]->screen_pos[i].w, tri_culling_list[tri_ind]->screen_pos[i].x, tri_culling_list[tri_ind]->screen_pos[i].y, tri_culling_list[tri_ind]->screen_pos[i].z);

    //         // view port transformation
    //         tri_culling_list[tri_ind]->screen_pos[i].x = 0.5f * ctx->width * (tri_culling_list[tri_ind]->screen_pos[i].x + 1.0f);
    //         tri_culling_list[tri_ind]->screen_pos[i].y = 0.5f * ctx->height * (tri_culling_list[tri_ind]->screen_pos[i].y + 1.0f);

    //         // [-1,1] to [0,1]
    //         tri_culling_list[tri_ind]->screen_pos[i].z = tri_culling_list[tri_ind]->screen_pos[i].z * 0.5f + 0.5f;
    //     }
    //     triangle_list[ind + tri_ind] = tri_culling_list[tri_ind];
    // }
    // tri_culling_list.clear();

    int culling_size = 0, ind = triangle_list.size();
    for (int i = 0; i < ppl->cpu_num; ++i) {
        // printf("thread_id: %d, size: %d\n", i, tri_cullings[i].size());
        culling_size += tri_cullings[i].size();
    }

    triangle_list.resize(triangle_list.size() + culling_size);

    for (int j = 0; j < ppl->cpu_num; ++j) {
        int tri_ind = 0, len = tri_cullings[j].size();
        for (; tri_ind < len; ++tri_ind) {
            // for (int i = 0; i < 3; ++i) {
            //     tri_cullings[j][tri_ind]->w_inversed[i] = 1.0f / tri_cullings[j][tri_ind]->screen_pos[i].w;
            //     tri_cullings[j][tri_ind]->screen_pos[i].x *= tri_cullings[j][tri_ind]->w_inversed[i];
            //     tri_cullings[j][tri_ind]->screen_pos[i].y *= tri_cullings[j][tri_ind]->w_inversed[i];
            //     tri_cullings[j][tri_ind]->screen_pos[i].z *= tri_cullings[j][tri_ind]->w_inversed[i];
            //     // view port transformation
            //     tri_cullings[j][tri_ind]->screen_pos[i].x = 0.5f * ctx->width * (tri_cullings[j][tri_ind]->screen_pos[i].x + 1.0f);
            //     tri_cullings[j][tri_ind]->screen_pos[i].y = 0.5f * ctx->height * (tri_cullings[j][tri_ind]->screen_pos[i].y + 1.0f);

            //     // [-1,1] to [0,1]
            //     tri_cullings[j][tri_ind]->screen_pos[i].z = tri_cullings[j][tri_ind]->screen_pos[i].z * 0.5f + 0.5f;
            // }
            programmable_view_port(tri_cullings[j][tri_ind]);
            triangle_list[ind + tri_ind] = tri_cullings[j][tri_ind];
        }
        ind += len;
    }
}

void programmable_rasterize_with_shading_openmp()
{
    GET_CURRENT_CONTEXT(ctx);
    std::vector<ProgrammableTriangle*>& prog_triangle_list = ctx->pipeline.prog_triangle_list;
    int width = ctx->width, height = ctx->height;

    color_t* frame_buf = (color_t*)ctx->framebuf->getDataPtr();

    int len = prog_triangle_list.size();
    float* zbuf = (float*)ctx->zbuf->getDataPtr();
    Shader* fragment_shader = ctx->payload.cur_shader_program_ptr->get_shader(GL_FRAGMENT_SHADER);
    std::vector<ShaderInterface*> shader_interfaces;
    shader_interfaces.resize(ctx->pipeline.cpu_num);
    for (int i = 0; i < ctx->pipeline.cpu_num; i++) {
        shader_interfaces[i] = fragment_shader->get_shader_utils(i);
    }
    
    // parallel variable value
    ProgrammableTriangle* t = nullptr;
    glm::vec4* screen_pos = nullptr;
    float* w_inv = nullptr;
    // std::unordered_map<std::string, data_t> frag_shader_in, frag_shader_out;

#ifdef GL_PARALLEL_OPEN
#pragma omp parallel for private(t) private(screen_pos) private(w_inv)
    // private(frag_shader_in) private(frag_shader_out)
#endif
    for (int i = 0; i < len; ++i) {
        t = prog_triangle_list[i];
        int thread_id = omp_get_thread_num();
        ShaderInterface* functions = shader_interfaces[thread_id];

        if (t->culling) {
            t->culling = false;
            continue;
        }

        screen_pos = t->screen_pos;
        w_inv = t->w_inversed;
        int minx, maxx, miny, maxy, x, y;
        minx = MIN(screen_pos[0].x, MIN(screen_pos[1].x, screen_pos[2].x));
        miny = MIN(screen_pos[0].y, MIN(screen_pos[1].y, screen_pos[2].y));
        maxx = MAX(screen_pos[0].x, MAX(screen_pos[1].x, screen_pos[2].x));
        maxy = MAX(screen_pos[0].y, MAX(screen_pos[1].y, screen_pos[2].y));

#if 1
        // view shrinking in rasterization
        minx = minx < 0 ? 0 : minx;
        miny = miny < 0 ? 0 : miny;
        maxx = maxx >= width ? width - 1 : maxx;
        maxy = maxy >= height ? height - 1 : maxy;
#endif

        // AABB algorithm
        for (y = miny; y <= maxy; ++y) {
            for (x = minx; x <= maxx; ++x) {
                int index = GET_INDEX(x, y, width, height);
                if (!t->inside(x + 0.5f, y + 0.5f))
                    continue;

                // alpha beta gamma
                glm::vec3 coef = t->computeBarycentric2D(x + 0.5f, y + 0.5f);
                // perspective correction
                float Z_viewspace = 1.0f / (coef[0] * w_inv[0] + coef[1] * w_inv[1] + coef[2] * w_inv[2]);
                float alpha = coef[0] * Z_viewspace * w_inv[0];
                float beta = coef[1] * Z_viewspace * w_inv[1];
                float gamma = coef[2] * Z_viewspace * w_inv[2];

                if (!ctx->use_z_test) {
                    throw std::runtime_error("please open the z depth test\n");
                } else {
                    // zp: z value after interpolation
                    float zp = alpha * screen_pos[0].w + beta * screen_pos[1].w + gamma * screen_pos[2].w;
                    omp_set_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                    if (zp < zbuf[index]) {
                        zbuf[index] = zp;
                        omp_unset_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                        std::map<std::string, data_t> frag_shader_in, frag_shader_out;
                        programmable_interpolate(fragment_shader, t, alpha, beta, gamma, frag_shader_in);
                        functions->input_port(frag_shader_in);
                        functions->glsl_main();
                        functions->output_port(frag_shader_out);
                        data_t frag_color_union;
                        functions->get_inner_variable(INNER_GL_FRAGCOLOR, frag_color_union);
                        omp_set_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                        frame_buf[index].R = frag_color_union.vec4_var.x * 255.0f;
                        frame_buf[index].G = frag_color_union.vec4_var.y * 255.0f;
                        frame_buf[index].B = frag_color_union.vec4_var.z * 255.0f;
                    }
                    omp_unset_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                    // frag_shader_in.clear();
                    // frag_shader_out.clear();
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// scanline //////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

static void fill_top_flat_triangle(const std::vector<glm::vec4>& new_t, ProgrammableTriangle* t, ShaderInterface* functions, Shader* fragment_shader)
{
    // printf("top! threadID=%d  thraed number:%d\n", omp_get_thread_num(), omp_get_num_threads());
    GET_CURRENT_CONTEXT(ctx);
    float* zbuf = (float*)ctx->zbuf->getDataPtr();
    glm::vec4* screen_pos = t->screen_pos;
    float* w_inv = t->w_inversed;
    color_t* frame_buf = (color_t*)ctx->framebuf->getDataPtr();

    int v[2] = { 0, 1 };
    if(new_t[v[0]].x > new_t[v[1]].x){
        std::swap(v[0], v[1]);
    }
    float l_step = (new_t[2].x - new_t[v[0]].x) / (new_t[2].y - new_t[v[0]].y);
    float r_step = (new_t[2].x - new_t[v[1]].x) / (new_t[2].y - new_t[v[1]].y);
    float start = new_t[v[0]].x;
    float end = new_t[v[1]].x;

    int minx, maxx, miny, maxy, x, y, xlen, ylen;
    minx = MIN(new_t[0].x, MIN(new_t[1].x, new_t[2].x));
    miny = new_t[0].y;
    maxx = MAX(new_t[0].x, MAX(new_t[1].x, new_t[2].x));
    maxy = new_t[2].y;

#if 1
    // view shrinking in rasterization
    minx = minx < 0 ? 0 : minx;
    miny = miny < 0 ? 0 : miny;
    maxx = maxx >= ctx->width ? ctx->width - 1 : maxx;
    maxy = maxy >= ctx->height ? ctx->height - 1 : maxy;
#endif

    if(new_t[0].y < miny){
        int t = miny - new_t[0].y;
        start += l_step * t;
        end += r_step * t;
    }
    start += (new_t[0].y - miny) * l_step;
    end += (new_t[0].y - miny) * r_step;
    for (y = miny, ylen = maxy; y <= ylen; ++y){
        if (start > end || start > maxx) {
            break;
        }
        for (x = (start < minx ? minx : start), xlen = (end > maxx ? maxx : end); x <= xlen; ++x) {
            int index = GET_INDEX(x, y, ctx->width, ctx->height);
            // alpha beta gamma
            glm::vec3 coef = t->computeBarycentric2D(x + 0.5f, y + 0.5f);

            // perspective correction
            float Z_viewspace = 1.0f / (coef[0] * w_inv[0] + coef[1] * w_inv[1] + coef[2] * w_inv[2]);
            float alpha = coef[0] * Z_viewspace * w_inv[0];
            float beta = coef[1] * Z_viewspace * w_inv[1];
            float gamma = coef[2] * Z_viewspace * w_inv[2];

            if (!ctx->use_z_test) {
                throw std::runtime_error("please open the z depth test\n");
            } else {
                // zp: z value after interpolation
                float zp = alpha * screen_pos[0].w + beta * screen_pos[1].w + gamma * screen_pos[2].w;
                omp_set_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                if (zp < zbuf[index]) {
                    zbuf[index] = zp;
                    omp_unset_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                    std::map<std::string, data_t> frag_shader_in, frag_shader_out;
                    programmable_interpolate(fragment_shader, t, alpha, beta, gamma, frag_shader_in);
                    functions->input_port(frag_shader_in);
                    functions->glsl_main();
                    functions->output_port(frag_shader_out);
                    data_t frag_color_union;
                    functions->get_inner_variable(INNER_GL_FRAGCOLOR, frag_color_union);
                    omp_set_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                    frame_buf[index].R = frag_color_union.vec4_var.x * 255.0f;
                    frame_buf[index].G = frag_color_union.vec4_var.y * 255.0f;
                    frame_buf[index].B = frag_color_union.vec4_var.z * 255.0f;
                }
                omp_unset_lock(&(ctx->pipeline.pixel_tasks[index].lock));
            }
        }
        start += l_step;
        end += r_step;
    }
}

static void fill_bottom_flat_triangle(const std::vector<glm::vec4>& new_t, ProgrammableTriangle* t, ShaderInterface* functions, Shader* fragment_shader)
{
    // printf("bottom! threadID=%d  thraed number:%d\n", omp_get_thread_num(), omp_get_num_threads());
    GET_CURRENT_CONTEXT(ctx);
    float* zbuf = (float*)ctx->zbuf->getDataPtr();
    glm::vec4* screen_pos = t->screen_pos;
    float* w_inv = t->w_inversed;
    color_t* frame_buf = (color_t*)ctx->framebuf->getDataPtr();

    int v[2] = { 0, 1 };
    if (new_t[v[0]].x > new_t[v[1]].x) {
        std::swap(v[0], v[1]);
    }
    float l_step = (new_t[2].x - new_t[v[0]].x) / (new_t[2].y - new_t[v[0]].y);
    float r_step = (new_t[2].x - new_t[v[1]].x) / (new_t[2].y - new_t[v[1]].y);
    
    float start = new_t[v[0]].x;
    float end = new_t[v[1]].x;

    int minx, maxx, miny, maxy, x, y, xlen, ylen;
    minx = MIN(new_t[0].x, MIN(new_t[1].x, new_t[2].x));
    miny = new_t[2].y;
    maxx = MAX(new_t[0].x, MAX(new_t[1].x, new_t[2].x));
    maxy = new_t[0].y;

#if 1
    // view shrinking in rasterization
    minx = minx < 0 ? 0 : minx;
    miny = miny < 0 ? 0 : miny;
    maxx = maxx >= ctx->width ? ctx->width - 1 : maxx;
    maxy = maxy >= ctx->height ? ctx->height - 1 : maxy;
#endif
    if (new_t[0].y > maxy) {
        int t = new_t[0].y - maxy;
        start -= l_step * t;
        end -= r_step * t;
    }
    start -= (new_t[0].y - maxy) * l_step;
    end -= (new_t[0].y - maxy) * r_step;
    for (y = maxy, ylen = miny; y >= ylen; --y) {
        if (start > end || end < minx) {
            break;
        }
        for (x = (start < minx ? minx : start), xlen = (end > maxx ? maxx : end); x <= xlen; ++x) {
            int index = GET_INDEX(x, y, ctx->width, ctx->height);
            // alpha beta gamma
            glm::vec3 coef = t->computeBarycentric2D(x + 0.5f, y + 0.5f);

            // perspective correction
            float Z_viewspace = 1.0f / (coef[0] * w_inv[0] + coef[1] * w_inv[1] + coef[2] * w_inv[2]);
            float alpha = coef[0] * Z_viewspace * w_inv[0];
            float beta = coef[1] * Z_viewspace * w_inv[1];
            float gamma = coef[2] * Z_viewspace * w_inv[2];

            if (!ctx->use_z_test) {
                throw std::runtime_error("please open the z depth test\n");
            } else {
                // zp: z value after interpolation
                float zp = alpha * screen_pos[0].w + beta * screen_pos[1].w + gamma * screen_pos[2].w;
                omp_set_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                if (zp < zbuf[index]) {
                    zbuf[index] = zp;
                    omp_unset_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                    std::map<std::string, data_t> frag_shader_in, frag_shader_out;
                    programmable_interpolate(fragment_shader, t, alpha, beta, gamma, frag_shader_in);
                    functions->input_port(frag_shader_in);
                    functions->glsl_main();
                    functions->output_port(frag_shader_out);
                    data_t frag_color_union;
                    functions->get_inner_variable(INNER_GL_FRAGCOLOR, frag_color_union);
                    omp_set_lock(&(ctx->pipeline.pixel_tasks[index].lock));
                    frame_buf[index].R = frag_color_union.vec4_var.x * 255.0f;
                    frame_buf[index].G = frag_color_union.vec4_var.y * 255.0f;
                    frame_buf[index].B = frag_color_union.vec4_var.z * 255.0f;
                }
                omp_unset_lock(&(ctx->pipeline.pixel_tasks[index].lock));
            }
        }
        start -= l_step;
        end -= r_step;
    }
}

// void upTriangle(const std::vector<glm::vec4>& new_t, ProgrammableTriangle* t, ShaderInterface* functions, Shader* fragment_shader)
// {
//     GET_CURRENT_CONTEXT(ctx);
//     float* zbuf = (float*)ctx->zbuf->getDataPtr();
//     glm::vec4* screen_pos = t->screen_pos;
//     float* w_inv = t->w_inversed;
//     color_t* frame_buf = (color_t*)ctx->framebuf->getDataPtr();
// }

void programmable_rasterize_with_scanline(){
    GET_CURRENT_CONTEXT(ctx);
    // printf("scanline begin\n");
    std::vector<ProgrammableTriangle*>& prog_triangle_list = ctx->pipeline.prog_triangle_list;
    int width = ctx->width, height = ctx->height;

    color_t* frame_buf = (color_t*)ctx->framebuf->getDataPtr();

    int len = prog_triangle_list.size();
    Shader* fragment_shader = ctx->payload.cur_shader_program_ptr->get_shader(GL_FRAGMENT_SHADER);
    std::vector<ShaderInterface*> shader_interfaces;
    shader_interfaces.resize(ctx->pipeline.cpu_num);
    for (int i = 0; i < ctx->pipeline.cpu_num; i++) {
        shader_interfaces[i] = fragment_shader->get_shader_utils(i);
    }

    // parallel variable value
    glm::vec4* screen_pos = nullptr;
    ShaderInterface* functions = nullptr;
#ifdef GL_PARALLEL_OPEN
#pragma omp parallel for private(screen_pos) private(functions)
#endif
    for (int i = 0; i < len; ++i) {
        if (prog_triangle_list[i]->culling) {
            prog_triangle_list[i]->culling = false;
            continue;
        }

        functions = shader_interfaces[omp_get_thread_num()];
        screen_pos = prog_triangle_list[i]->screen_pos;

        int v[3] = { 0, 1, 2 };
        
        if(screen_pos[v[0]].y > screen_pos[v[1]].y){
            // std::swap(v[0], v[1]);
            v[0] ^= v[1] ^= v[0] ^= v[1];
        }
        if(screen_pos[v[1]].y > screen_pos[v[2]].y){
            // std::swap(v[1], v[2]);
            v[1] ^= v[2] ^= v[1] ^= v[2];
        }
        if (screen_pos[v[0]].y > screen_pos[v[1]].y) {
            // std::swap(v[0], v[1]);
            v[0] ^= v[1] ^= v[0] ^= v[1];
        }

        if(screen_pos[v[0]].y == screen_pos[v[1]].y){
            fill_top_flat_triangle({ screen_pos[v[0]], screen_pos[v[1]], screen_pos[v[2]] }, prog_triangle_list[i], functions, fragment_shader);
        } else if (screen_pos[v[1]].y == screen_pos[v[2]].y){
            fill_bottom_flat_triangle({ screen_pos[v[1]], screen_pos[v[2]], screen_pos[v[0]] }, prog_triangle_list[i], functions, fragment_shader);
        }else{
            float weight = (screen_pos[v[2]].y - screen_pos[v[1]].y) / (screen_pos[v[2]].y - screen_pos[v[0]].y);
            float pos_x = screen_pos[v[2]].x - (screen_pos[v[2]].x - screen_pos[v[0]].x) * weight;

            glm::vec4 new_p = { pos_x, screen_pos[v[1]].y, 0.0f, 0.0f};
            fill_top_flat_triangle({ screen_pos[v[1]], new_p, screen_pos[v[2]] }, prog_triangle_list[i], functions, fragment_shader);
            fill_bottom_flat_triangle({ screen_pos[v[1]], new_p, screen_pos[v[0]] }, prog_triangle_list[i], functions, fragment_shader);
        }
        // printf("i: %d\n", i);
    }

    // printf("scanline end\n");
}