#ifndef _GEOMETRY_H
#define _GEOMETRY_H

#include "configs.h"
#include "../../include/gl/common.h"
#include <stdio.h>
#include <glm/glm.hpp>
#include <array>
#include <map>
#include <queue>
#include <assert.h>
#include <math.h>

class Triangle
{
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

    inline int sanity_check(float c1, float c2, float c3){
        if (abs(1-c1-c2-c3) < 0.1)
            return 1;
        printf("%f, %f, %f\n", c1, c2, c3);
        return 0;
    }
};

typedef struct{
    int sizes[GL_MAX_VERTEX_ATTRIB_NUM];
    int strides[GL_MAX_VERTEX_ATTRIB_NUM];
    int indices[GL_MAX_VERTEX_ATTRIB_NUM];
    int offsets[GL_MAX_VERTEX_ATTRIB_NUM];
    GLenum dtypes[GL_MAX_VERTEX_ATTRIB_NUM];
} crawler_config_t;

// forward definition
class glProgram;
class TriangleCrawler{
    public:
        TriangleCrawler();
        inline void set_config(int layout, int size, int stride, int offset, GLenum dtype){
            if (dtype != GL_FLOAT){
                throw std::runtime_error("not supporting non float dtype when parsing vertex data");
            }
            config.sizes[layout] = size;
            config.strides[layout] = stride;
            config.offsets[layout] = offset;
            config.dtypes[layout] = dtype;
        }
        inline void reset_config(){
            for (int i=0; i<GL_MAX_VERTEX_ATTRIB_NUM; i++){
                config.sizes[i] = 0;
                config.strides[i] = 0;
                config.indices[i] = 0;
                config.offsets[i] = 0;
                config.dtypes[i] = GL_FLOAT;
            }
        }
        inline void reset_data(){
            {
                std::map<int, std::queue<glm::vec2>>::iterator it;
                for (it=data_float_vec2.begin(); it!=data_float_vec2.end(); it++){
                    while (! it->second.empty())
                        it->second.pop();
                }
            }
            {
                std::map<int, std::queue<glm::vec3>>::iterator it;
                for (it=data_float_vec3.begin(); it!=data_float_vec3.end(); it++){
                    while (! it->second.empty())
                        it->second.pop();
                }
            }
            {
                std::map<int, std::queue<glm::vec4>>::iterator it;
                for (it=data_float_vec4.begin(); it!=data_float_vec4.end(); it++){
                    while (! it->second.empty())
                        it->second.pop();
                }
            }
        }
        inline void set_start_point(int * layouts, int layout_cnt, int thread_id, int first_vertex){
            for (int i=0; i<layout_cnt; i++){
                int layout = layouts[i];
                if (layout<0)
                    continue;
                config.indices[layout] = first_vertex * config.strides[layout];
                config.indices[layout] += thread_id * config.strides[layout];
            }
        }
        int crawl(char* source, int buf_size, int first_vertex, glProgram& shader);

        std::map<int, std::queue<glm::vec1>> data_float_vec1;
        std::map<int, std::queue<glm::vec2>> data_float_vec2;
        std::map<int, std::queue<glm::vec3>> data_float_vec3;
        std::map<int, std::queue<glm::vec4>> data_float_vec4;
        crawler_config_t config;
};


#endif