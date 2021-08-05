#ifndef _GEOMETRY_H
#define _GEOMETRY_H

#include "../../include/gl/common.h"
#include "glsl/parse.h"
#include "glsl/shader.hpp"
#include "configs.h"
#include <array>
#include <assert.h>
#include <map>
#include <math.h>
#include <queue>
#include <stdio.h>
#include "glsl/vec_math.h"

class Vertex {
public:
    glm::vec4 screen_pos;
    glm::vec3 color;
    glm::vec3 frag_shading_pos;
    glm::vec3 vert_normal;
    glm::vec2 texcoord;

    inline static Vertex lerp(Vertex& v1, Vertex& v2, float weight)
    {
        Vertex res;
        res.screen_pos = v1.screen_pos + (v2.screen_pos - v1.screen_pos) * weight;
        res.color = v1.color + (v2.color - v1.color) * weight;
        res.frag_shading_pos = v1.frag_shading_pos + (v2.frag_shading_pos - v1.frag_shading_pos) * weight;
        res.vert_normal = v1.vert_normal + (v2.vert_normal - v1.vert_normal) * weight;
        res.texcoord = v1.texcoord + (v2.texcoord - v1.texcoord) * weight;
        return res;
    }
};

class Triangle {
public:
    /**
     * counter clockwise order
     */
    glm::vec4 screen_pos[3];
    glm::vec3 color[3];
    glm::vec3 frag_shading_pos[3];
    glm::vec3 vert_normal[3];
    glm::vec2 texcoord[3];

    bool culling = false;

    bool inside(float x, float y);
    glm::vec3 computeBarycentric2D(float x, float y);

    inline Vertex getVertex(int index)
    {
        Vertex v;
        if (index < 0 || index > 2) {
            return v;
        }
        v.screen_pos = screen_pos[index];
        v.color = color[index];
        v.frag_shading_pos = frag_shading_pos[index];
        v.vert_normal = vert_normal[index];
        v.texcoord = texcoord[index];
        return v;
    }

    inline void setVertex(int index, Vertex& v)
    {
        if (index < 0 || index > 2) {
            return;
        }
        screen_pos[index] = v.screen_pos;
        color[index] = v.color;
        frag_shading_pos[index] = v.frag_shading_pos;
        vert_normal[index] = v.vert_normal;
        texcoord[index] = v.texcoord;
    }

    inline int sanity_check(float c1, float c2, float c3)
    {
        if (abs(1 - c1 - c2 - c3) < 0.1)
            return 1;
        printf("%f, %f, %f\n", c1, c2, c3);
        return 0;
    }
};

typedef struct {
    int sizes[GL_MAX_VERTEX_ATTRIB_NUM];
    int strides[GL_MAX_VERTEX_ATTRIB_NUM];
    int indices[GL_MAX_VERTEX_ATTRIB_NUM];
    int offsets[GL_MAX_VERTEX_ATTRIB_NUM];
    GLenum dtypes[GL_MAX_VERTEX_ATTRIB_NUM];
} crawler_config_t;

// forward definition
class glProgram;
class TriangleCrawler {
public:
    TriangleCrawler();
    inline void set_config(int layout, int size, int stride, int offset, GLenum dtype)
    {
        if (dtype != GL_FLOAT) {
            throw std::runtime_error("not supporting non float dtype when parsing vertex data");
        }
        config.sizes[layout] = size;
        config.strides[layout] = stride;
        config.offsets[layout] = offset;
        config.dtypes[layout] = dtype;
    }
    inline void reset_config()
    {
        for (int i = 0; i < GL_MAX_VERTEX_ATTRIB_NUM; i++) {
            config.sizes[i] = 0;
            config.strides[i] = 0;
            config.indices[i] = 0;
            config.offsets[i] = 0;
            config.dtypes[i] = GL_FLOAT;
        }
    }
    inline void reset_data()
    {
        {
            std::map<int, std::queue<glm::vec2>>::iterator it;
            for (it = data_float_vec2.begin(); it != data_float_vec2.end(); it++) {
                while (!it->second.empty())
                    it->second.pop();
            }
        }
        {
            std::map<int, std::queue<glm::vec3>>::iterator it;
            for (it = data_float_vec3.begin(); it != data_float_vec3.end(); it++) {
                while (!it->second.empty())
                    it->second.pop();
            }
        }
        {
            std::map<int, std::queue<glm::vec4>>::iterator it;
            for (it = data_float_vec4.begin(); it != data_float_vec4.end(); it++) {
                while (!it->second.empty())
                    it->second.pop();
            }
        }
    }
    inline void set_start_point(int* layouts, int layout_cnt, int thread_id, int first_vertex)
    {
        for (int i = 0; i < layout_cnt; i++) {
            int layout = layouts[i];
            if (layout < 0)
                continue;
            config.indices[layout] = first_vertex * config.strides[layout];
            config.indices[layout] += thread_id * config.strides[layout];
        }
    }
    int crawl(char* source, int buf_size, int first_vertex, glProgram& shader);

    std::map<int, std::queue<glm::vec2>> data_float_vec2;
    std::map<int, std::queue<glm::vec3>> data_float_vec3;
    std::map<int, std::queue<glm::vec4>> data_float_vec4;
    crawler_config_t config;
};

class ProgrammableVertex {
public:
    glm::vec4 screen_pos;
    std::map<std::string, data_t> vertex_attrib;
    ProgrammableVertex() {

    }
    ProgrammableVertex(const ProgrammableVertex& v){
        this->screen_pos = v.screen_pos;
        this->vertex_attrib = v.vertex_attrib;
    }
    inline static ProgrammableVertex lerp(Shader* shader_ptr, ProgrammableVertex &v1, ProgrammableVertex &v2, float weight)
    {
        ProgrammableVertex res;
        res.screen_pos = v1.screen_pos + (v2.screen_pos - v1.screen_pos) * weight;
        for (auto it = v1.vertex_attrib.begin(); it != v1.vertex_attrib.end(); ++it) {
            int dtype = shader_ptr->io_profile[it->first].dtype;
            data_t interp_data;
            switch (dtype) {
                case TYPE_VEC2:
                    interp_data.vec2_var = it->second.vec2_var + (v2.vertex_attrib[it->first].vec2_var - it->second.vec2_var) * weight;
                    break;
                case TYPE_VEC3:
                    interp_data.vec3_var = it->second.vec3_var + (v2.vertex_attrib[it->first].vec3_var - it->second.vec3_var) * weight;
                    break;
                case TYPE_VEC4:
                    interp_data.vec4_var = it->second.vec4_var + (v2.vertex_attrib[it->first].vec4_var - it->second.vec4_var) * weight;
                    break;
                default:
                    throw std::runtime_error("vertex don't interp on these types now\n");
            }
            res.vertex_attrib.emplace(it->first, interp_data);
        }
        return res;
    }


};

class ProgrammableTriangle{
public:
    glm::vec4 screen_pos[3];
    std::map<std::string, data_t> vertex_attribs[3];

    bool culling = false;
    float w_inversed[3];

    ProgrammableVertex vertices[3];

    Shader* cur_shader;

    bool inside(float x, float y);
    glm::vec3 computeBarycentric2D(float x, float y);
    void view_frustum_culling(const std::vector<glm::vec4>& planes, std::vector<ProgrammableTriangle*>& res);

    inline void setVertex(int index, ProgrammableVertex& v)
    {
#ifndef GL_SCANLINE
        screen_pos[index] = v.screen_pos;
        vertex_attribs[index] = v.vertex_attrib;
#else
        vertices[index].screen_pos = v.screen_pos;
        vertices[index].vertex_attrib = v.vertex_attrib;
#endif
    }

private:
    bool outside_clip_space();
    bool all_inside_clip_space();
    bool inside_plane(const glm::vec4& plane, glm::vec4& pos);
    ProgrammableVertex intersect(ProgrammableVertex& v1, ProgrammableVertex& v2, const glm::vec4& plane);
};

#endif