#include <benchmark/benchmark.h>
#include "../gl/glcontext.h"
#include "../../include/gl/gl.h"
#include "../gl/render.h"
#include "../../include/glv/glv.h"
#include "../gl/glsl/shader.hpp"
#include "header_assimp/model.h"
#include "header_assimp/shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

const int WIDTH = 800, HEIGHT = 600;
const float ZNEAR = 0.1f, ZFAR = 1000.0f;

glm::vec3 lightPos(0.0f, 0.0f, -80.0f);

/////////////////////////////// pipeline helper ///////////////////////////////////////////
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

static void programmable_view_port(ProgrammableTriangle* t)
{
    GET_CURRENT_CONTEXT(ctx);
    for (int i = 0; i < 3; ++i) {
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

static std::vector<int>* programmable_parse_indices(int& triangle_size)
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

///////////////////////////////////////////// pipeline ///////////////////////////////////////////////


static void prepareElement(GLenum mode, int count, unsigned int type, const void* indices)
{
    GET_CURRENT_CONTEXT(ctx);
    if (ctx == nullptr) {
        return;
    }
    glManager& bufs = ctx->share.buffers;
    // glManager& vaos = ctx->share.vertex_attribs;
    glManager& vaos = ctx->share.vertex_array_objects;
    glManager& texs = ctx->share.textures;
    glObject* vao_ptr;
    glObject* vbo_ptr;
    glObject* ebo_ptr;
    glObject* tex_ptr;
    int ret, vao_id, vbo_id, ebo_id;
    // sanity checks for VAO
    vao_id = ctx->payload.renderMap[GL_BIND_VAO];
    vbo_id = ctx->payload.renderMap[GL_ARRAY_BUFFER];
    ebo_id = ctx->payload.renderMap[GL_ELEMENT_ARRAY_BUFFER];
    if (vao_id < 0 || vbo_id < 0 || ebo_id < 0) {
        return;
    }
    ret = vaos.searchStorage(&vao_ptr, vao_id);
    if (ret == GL_FAILURE
        || vao_ptr->bind != GL_BIND_VAO
        || vao_ptr->getDataPtr() == nullptr
        || vao_ptr->getSize() <= 0) {
        return;
    }
    ret = bufs.searchStorage(&vbo_ptr, vbo_id);
    if (ret == GL_FAILURE
        || vbo_ptr->bind != GL_ARRAY_BUFFER
        || vbo_ptr->getDataPtr() == nullptr
        || vbo_ptr->getSize() <= 0) {
        return;
    }
    ret = bufs.searchStorage(&ebo_ptr, ebo_id);
    if (ret == GL_FAILURE
        || ebo_ptr->bind != GL_ELEMENT_ARRAY_BUFFER
        || ebo_ptr->getDataPtr() == nullptr
        || ebo_ptr->getSize() <= 0) {
        return;
    }

    // prepare pipeline environment
    ctx->pipeline.vao_ptr = vao_ptr;
    ctx->pipeline.vbo_ptr = vbo_ptr;
    ctx->pipeline.vertex_num = count;
    ctx->pipeline.ebo_config.ebo_ptr = ebo_ptr;
    ctx->pipeline.ebo_config.first_indices = indices;
    ctx->pipeline.ebo_config.indices_type = type;
    ctx->pipeline.use_indices = true;

    // draw
    // std::list<render_fp>& exec_list = ctx->pipeline.exec;
    // auto iter = exec_list.begin();
    // switch (mode) {
    // case GL_TRIANGLES:
    //     while (iter != exec_list.end()) {
    //         (*iter)();
    //         ++iter;
    //     }
    //     break;
    // default:
    //     break;
    // }
}

/////////////////////////////////////////// test //////////////////////////////////////////////
GLVStream* window = nullptr;
Model* model = nullptr;
ShaderWrapper* shader = nullptr;

void initialize()
{
    if (!glvInit()) {
        std::cout << "glv Init failed\n";
        return;
    }

    window = glvCreateStream(WIDTH, HEIGHT, "sponza atrium", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);

    glm::mat4 modelMatrix(1.0f);
    glm::mat4 viewMatrix(1.0f);
    glm::mat4 projectionMatrix(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -30.0f, -80.0f));
    // modelMatrix = glm::translate(modelMatrix, glm::vec3(20.0f, -30.0f, -80.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 0.1f, 0.1f));
    glm::vec3 eyepos(0.0f, 0.0f, 5.0f);
    glm::vec3 front(0.0f, 0.0f, -1.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    viewMatrix = glm::lookAt(eyepos, eyepos + front, up);
    projectionMatrix = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, ZNEAR, ZFAR);

    model = new Model("../resources/sponza/sponza.obj");

    shader = new ShaderWrapper("../shader/sponza_vert.glsl", "../shader/sponza_frag.glsl");
    shader->use();
    shader->setMat4("model", modelMatrix);
    shader->setMat4("view", viewMatrix);
    shader->setMat4("projection", projectionMatrix);
    shader->setMat4("inv_model", glm::transpose(glm::inverse(modelMatrix)));
    shader->setVec3("lightPos", lightPos);
    shader->setVec3("viewPos", eyepos);
    shader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
}

void memoryCollection(){
    delete model;
    delete shader;
    glvTerminate();
}

static void testProgPipeline(benchmark::State& state){

    initialize();
    // Perform setup here
    for (auto _ : state) {
        state.PauseTiming();
        if (glvWindowShouldClose(window)) {
            break;
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader->use();

        // model.Draw(shader);
        for (unsigned int j = 0; j < model->meshes.size(); ++j) {
            // bind appropriate textures
            unsigned int diffuseNr = 1;
            unsigned int specularNr = 1;
            unsigned int normalNr = 1;
            unsigned int heightNr = 1;
            for (unsigned int i = 0; i < model->meshes[j].textures.size(); i++) {
                glActiveTexture((GLenum)(GL_TEXTURE0 + i)); // active proper texture unit before binding
                // retrieve texture number (the N in diffuse_textureN)
                string number;
                string name = model->meshes[j].textures[i].type;
                if (name == "texture_diffuse")
                    number = std::to_string(diffuseNr++);
                else if (name == "texture_specular")
                    number = std::to_string(specularNr++); // transfer unsigned int to stream
                else if (name == "texture_normal")
                    number = std::to_string(normalNr++); // transfer unsigned int to stream
                else if (name == "texture_height")
                    number = std::to_string(heightNr++); // transfer unsigned int to stream

                // now set the sampler to the correct texture unit
                glUniform1i(glGetUniformLocation(shader->ID, (name + number).c_str()), i);
                // and finally bind the texture
                glBindTexture(GL_TEXTURE_2D, model->meshes[j].textures[i].id);
            }

            // draw mesh
            glBindVertexArray(model->meshes[j].VAO);

            // glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            prepareElement(GL_TRIANGLES, model->meshes[j].indices.size(), GL_UNSIGNED_INT, 0);

            state.ResumeTiming();
            programmable_process_geometry_openmp();
            state.PauseTiming();
            programmable_rasterize_with_shading_openmp();
            glBindVertexArray(0);

            // always good practice to set everything back to defaults once configured.
            glActiveTexture(GL_TEXTURE0);
        }

        glvWriteStream(window);
        state.ResumeTiming();
    }

    memoryCollection();
}

void pipeline(benchmark::State& state)
{

//////////////////////////////////////////////////// geometry processing //////////////////////////////////////////////////
    /**
     * input assembly
     */
    GET_PIPELINE(ppl);
    GET_CURRENT_CONTEXT(ctx);

    vertex_attrib_t* vattrib_data = (vertex_attrib_t*)((vertex_array_object_t*)ppl->vao_ptr->getDataPtr())->attribs;
    int triangle_size = 0;

////////////////////////////////////////////// start timing ///////////////////////////////////////////////
    state.ResumeTiming();
    // parse indices
    std::vector<int>* indices = programmable_parse_indices(triangle_size);
////////////////////////////////////////////// end timing ///////////////////////////////////////////////
    state.PauseTiming();

    unsigned char* vbuf_data = (unsigned char*)ppl->vbo_ptr->getDataPtr();

    std::vector<ProgrammableTriangle*>& triangle_list = ppl->prog_triangle_list;

    // triangle list management
    if (triangle_list.size() < triangle_size) {
        int tsize = triangle_list.size();
        triangle_list.resize(triangle_size);
        for (int i = tsize, len = triangle_size; i < len; ++i) {
            triangle_list[i] = new ProgrammableTriangle();
        }
    } else if (triangle_list.size() > triangle_size) {
        for (int i = triangle_size, len = triangle_list.size(); i < len; ++i) {
            delete triangle_list[i];
        }
        triangle_list.resize(triangle_size);
    }

    Shader* vert_shader = ctx->payload.cur_shader_program_ptr->get_shader(GL_VERTEX_SHADER);

    std::vector<std::vector<ProgrammableTriangle*>> tri_cullings;
    std::vector<ShaderInterface*> shader_interfaces;
    shader_interfaces.resize(ppl->cpu_num);
    tri_cullings.resize(ppl->cpu_num);
    for (int i = 0; i < ppl->cpu_num; i++) {
        shader_interfaces[i] = vert_shader->get_shader_utils(i);
    }

    // begin parallel block
    std::map<std::string, data_t> vs_input, vs_output;
    void* input_ptr;
    unsigned char* buf;
    int thread_id;
    std::vector<ProgrammableTriangle*> vfc_list;
#ifdef GL_PARALLEL_OPEN
#pragma omp parallel for private(input_ptr) private(buf) private(vs_input) private(vs_output) private(thread_id) private(vfc_list)
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
        triangle_list[tri_ind]->view_frustum_culling(planes, vfc_list);
        if (vfc_list.size() != 0) {
            if (ctx->cull_face.open) {
                // for (int tind = 0, tlen = vfc_list.size(); tind < tlen; ++tind) {
                //     // backface_culling(*vfc_list[tind]);
                //     if (vfc_list[tind]->culling) {
                //         delete vfc_list[tind];
                //     } else {
                //         tri_cullings[thread_id].push_back(vfc_list[tind]);
                //     }
                // }
            } else {
                tri_cullings[thread_id].insert(tri_cullings[thread_id].end(), vfc_list.begin(), vfc_list.end());
            }
        } else if (ctx->cull_face.open && !triangle_list[tri_ind]->culling) {
            // backface_culling(*triangle_list[tri_ind]);
        }
        vfc_list.clear();

        if (triangle_list[tri_ind]->culling) {
            continue;
        }
        programmable_view_port(triangle_list[tri_ind]);
    }

    int culling_size = 0, ind = triangle_list.size();
    for (int i = 0; i < ppl->cpu_num; ++i) {
        // printf("thread_id: %d, size: %d\n", i, tri_cullings[i].size());
        culling_size += tri_cullings[i].size();
    }

    triangle_list.resize(triangle_list.size() + culling_size);

    for (int j = 0; j < ppl->cpu_num; ++j) {
        int tri_ind = 0, len = tri_cullings[j].size();
        for (; tri_ind < len; ++tri_ind) {
            programmable_view_port(tri_cullings[j][tri_ind]);
            triangle_list[ind + tri_ind] = tri_cullings[j][tri_ind];
        }
        ind += len;
    }

/////////////////////////////////////////////  rasterization ///////////////////////////////////////////////////

    std::vector<ProgrammableTriangle*>& prog_triangle_list = ctx->pipeline.prog_triangle_list;
    int width = ctx->width, height = ctx->height;

    color_t* frame_buf = (color_t*)ctx->framebuf->getDataPtr();

    int len = prog_triangle_list.size();
    float* zbuf = (float*)ctx->zbuf->getDataPtr();
    Shader* fragment_shader = ctx->payload.cur_shader_program_ptr->get_shader(GL_FRAGMENT_SHADER);
    // std::vector<ShaderInterface*> shader_interfaces;
    shader_interfaces.resize(ctx->pipeline.cpu_num);
    for (int i = 0; i < ctx->pipeline.cpu_num; i++) {
        shader_interfaces[i] = fragment_shader->get_shader_utils(i);
    }

    // parallel variable value
    ProgrammableTriangle* t = nullptr;
    glm::vec4* screen_pos = nullptr;
    float* w_inv = nullptr;
    std::map<std::string, data_t> frag_shader_in, frag_shader_out;
#ifdef GL_PARALLEL_OPEN
#pragma omp parallel for private(t) private(screen_pos) private(w_inv) private(frag_shader_in) private(frag_shader_out)
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
                    frag_shader_in.clear();
                    frag_shader_out.clear();
                }
            }
        }
    }
}

static void testPerStatement(benchmark::State& state)
{
    initialize();
    // Perform setup here
    for (auto _ : state) {
        state.PauseTiming();
        if (glvWindowShouldClose(window)) {
            break;
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader->use();

        // model.Draw(shader);
        for (unsigned int j = 0; j < model->meshes.size(); ++j) {
            // bind appropriate textures
            unsigned int diffuseNr = 1;
            unsigned int specularNr = 1;
            unsigned int normalNr = 1;
            unsigned int heightNr = 1;
            for (unsigned int i = 0; i < model->meshes[j].textures.size(); i++) {
                glActiveTexture((GLenum)(GL_TEXTURE0 + i)); // active proper texture unit before binding
                // retrieve texture number (the N in diffuse_textureN)
                string number;
                string name = model->meshes[j].textures[i].type;
                if (name == "texture_diffuse")
                    number = std::to_string(diffuseNr++);
                else if (name == "texture_specular")
                    number = std::to_string(specularNr++); // transfer unsigned int to stream
                else if (name == "texture_normal")
                    number = std::to_string(normalNr++); // transfer unsigned int to stream
                else if (name == "texture_height")
                    number = std::to_string(heightNr++); // transfer unsigned int to stream

                // now set the sampler to the correct texture unit
                glUniform1i(glGetUniformLocation(shader->ID, (name + number).c_str()), i);
                // and finally bind the texture
                glBindTexture(GL_TEXTURE_2D, model->meshes[j].textures[i].id);
            }

            // draw mesh
            glBindVertexArray(model->meshes[j].VAO);

            // glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            prepareElement(GL_TRIANGLES, model->meshes[j].indices.size(), GL_UNSIGNED_INT, 0);
            pipeline(state);
            glBindVertexArray(0);

            // always good practice to set everything back to defaults once configured.
            glActiveTexture(GL_TEXTURE0);
        }

        glvWriteStream(window);
        state.ResumeTiming();
    }

    memoryCollection();
}

// Register the function as a benchmark
// Benchmark               Time                 CPU            Iterations
// BENCHMARK(testProgPipeline)->Iterations(10);
BENCHMARK(testPerStatement)->Iterations(100);
// Run the benchmark
BENCHMARK_MAIN();
