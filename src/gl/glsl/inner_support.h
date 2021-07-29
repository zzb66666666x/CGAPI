#ifndef _INNER_SUPPORT_H
#define _INNER_SUPPORT_H

#include <map>
#include <stdio.h>

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

get_sampler2D_data_fptr get_sampler2D;

class sampler2D{
    public:
    sampler2D():loaded_texture(false){}
    bool loaded_texture;
    int texunit_id;
    int width;
    int height;
    int color_format;
    unsigned char *data;
    filter_type filter;
    sampler2D& operator=(int val){
        texunit_id = val;
        loaded_texture = false;
        // printf("define value for sampler2D\n");
        return *this;
    }
    inline void load_texture(){
        if (loaded_texture)
            return;
        sampler_data_pack tmp = get_sampler2D(texunit_id);
        width = tmp.width;
        height = tmp.height;
        color_format = tmp.color_format;
        data = tmp.tex_data;
        filter = tmp.filter;
        loaded_texture = true;
    }
};

glm::vec4 texture(sampler2D &samp, glm::vec2 &texcoord)
{
    samp.load_texture();
    glm::vec4 res = glm::vec4(1.0f);
    if (samp.height == 0 || samp.width == 0)
        throw std::runtime_error("invalid texture used in shader\n");
    // if (texcoord.x < 0 || texcoord.y < 0)
    //     return res;
    float tx = texcoord.x, ty = texcoord.y;
    float inv_255 = 1.0f / 255.0f;
    // REPEAT S
    if(tx < 0.0f || tx >= 1.0f){
        tx = tx - (int)tx;
        if(tx < 0.0f){
            tx += 1.0f;
        }
    }

    // REPEAT T
    if(ty < 0.0f || ty >= 1.0f){
        ty = ty - (int)ty;
        if(ty < 0.0f){
            ty += 1.0f;
        }
    }
    int channel,size = samp.width * samp.height;
    switch (samp.color_format) {
        case FORMAT_COLOR_8UC3:
            channel = 3;
            break;
        case FORMAT_COLOR_8UC4:
            channel = 4;
            // printf("using RGBA\n");
            break;
        default:
            throw std::runtime_error("invalid color format\n");
            break;
    }
    if (samp.filter == filter_type::NEAREST)
    {
        int x = tx * samp.width;
        int y = ty * samp.height;
        int index = y * samp.width + x;
        for (int i = 0; i < channel; ++i) {
            res[i] = ((float)samp.data[index * channel + i]) * inv_255;
        }
    }
    else if (samp.filter == filter_type::BILINEAR)
    {
        // TODO
        float x = tx * samp.width;
        float y = ty * samp.height;
        glm::vec4 u00, u01, u10, u11;
        int i00, i01, i11, i10;
        i00 = (int)y * samp.width + (int)x;
        i01 = (int)(y + 0.5f) * samp.width + (int)x;
        i10 = (int)y * samp.width + (int)(x + 0.5f);
        i11 = (int)(y + 0.5f) * samp.width + (int)(x + 0.5f);
        i01 = i00 >= size ? size - 1 : i00;
        i10 = i10 >= size ? size - 1 : i10;
        i11 = i11 >= size ? size - 1 : i11;
        for (int i = 0;i < channel;++i){
            u00[i] = ((float)samp.data[i00 * channel + i]) * inv_255;
            u01[i] = ((float)samp.data[i01 * channel + i]) * inv_255;
            u10[i] = ((float)samp.data[i10 * channel + i]) * inv_255;
            u11[i] = ((float)samp.data[i11 * channel + i]) * inv_255;
        }
        float s = x - (int)x;
        float t = y - (int)y;
        glm::vec4 u0 = u00 + s * (u10 - u00);
        glm::vec4 u1 = u01 + s * (u11 - u01);

        // lerp
        res = u0 + t * (u1 - u0);
    }

    return res;
}

#endif

#endif