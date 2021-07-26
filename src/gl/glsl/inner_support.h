#ifndef _INNER_SUPPORT_H
#define _INNER_SUPPORT_H

#include <map>

#ifdef GLSL_CODE
enum filter_type{
    NEAREST,
    BILINEAR
};
#else
#include "texture.h"
#include "vec_math.h"
#include "../formats.h"
#endif

typedef struct{
    unsigned char* tex_data;
    int width;
    int height;
    int color_format;
    filter_type filter;
}sampler_data_pack;

// the call back to enable glsl code to fetch data from graphics pipeline
typedef sampler_data_pack (*get_sampler2D_data_fptr)(int texunit_id);

class ShaderInterface{
    public:
    virtual ~ShaderInterface(){}
    virtual void glsl_main() = 0;
    virtual void input_port(std::map<std::string, data_t>& indata) = 0; 
    virtual void output_port(std::map<std::string, data_t>& outdata) = 0; 
    virtual void input_uniform_dispatch(int idx, data_t& data) = 0; 
    virtual data_t output_uniform_dispatch(int idx) = 0; 
    virtual void set_inner_variable(int variable, data_t& data) = 0;
    virtual void get_inner_variable(int variable, data_t& data) = 0;
    virtual void set_sampler2D_callback(get_sampler2D_data_fptr func) = 0;
};

#ifdef GLSL_CODE

class GLSLShader;

// fp to member function
typedef void (GLSLShader::*set_uniform)(data_t& var);
typedef data_t (GLSLShader::*get_uniform)(void);

class sampler2D{
    public:
    sampler2D(){}
    int texunit_id;
    int width;
    int height;
    int color_format;
    unsigned char *data;
    filter_type filter;
    sampler2D& operator=(int val){
        texunit_id = val;
        sampler_data_pack tmp = get_sampler2D(val);
        width = tmp.width;
        height = tmp.height;
        color_format = tmp.color_format;
        data = tmp.tex_data;
        filter = tmp.filter;
        return *this;
    }
};

glm::vec4 texture(sampler2D &texture, glm::vec2 &texcoord)
{
    glm::vec4 res = glm::vec4(1.0f);
    if (texture.height == 0 || texture.width == 0)
        throw std::runtime_error("invalid texture used in shader\n");
    float x = texcoord.x * texture.width;
    float y = texcoord.y * texture.height;
    int channel;
    if (texture.filter == filter_type::NEAREST)
    {
        x += 0.5f;
        y += 0.5f;
        int index = ((int)y % texture.height) * texture.width + ((int)x % texture.width);
        switch(texture.color_format){
            case FORMAT_COLOR_8UC3:
                channel = 3;
                for (int i = 0; i < channel; ++i)
                {
                    res[i] = ((float) texture.data[index * channel + i]) / 255.0f;
                }
                break;
            case FORMAT_COLOR_8UC4:
                throw std::runtime_error("not supporting that color format yet\n");
                break;
            default:
                throw std::runtime_error("invalid color format\n");
                break;
        }
    }
    else if (texture.filter == filter_type::BILINEAR)
    {
        // TODO
    }

    return res;
}

#endif

#endif