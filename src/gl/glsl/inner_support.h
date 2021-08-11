#ifndef _INNER_SUPPORT_H
#define _INNER_SUPPORT_H

#include <map>
#include <vector>
#include <stdio.h>
#include <omp.h>
#include <thread>

#ifdef GLSL_CODE
enum filter_type{
    NEAREST,
    LINEAR,
    NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR
};
#else
#include "texture.h"
#include "vec_math.h"
#include "../formats.h"
#endif

struct sampler_info_t;
class MipmapStorage;

struct ProgrammablePixel {
    ProgrammablePixel()
        : write(false)
    {}
    bool write;
    std::map<std::string, data_t> frag_shader_in, frag_shader_out;
    glm::vec2 texcoord;
};


typedef struct {
    void* tex_data;
    int width;
    int height;
    int color_format;
    int wrap_s;
    int wrap_t;
    glm::vec4 border_val;
    MipmapStorage *mipmap;
    int min_filter;
    int mag_filter;
} sampler_data_pack;

struct sampler_info_t {
    int width;
    int height;
    int format_size;
    void* data;
    sampler_info_t(int w, int h, int fs, void* d)
        : width(w)
        , height(h)
        , format_size(fs)
    {
        data = nullptr;
        if (d != nullptr) {
            data = malloc(fs * w * h);
            memcpy(data, d, fs * w * h);
        }
    }

    ~sampler_info_t()
    {
        free(data);
    }
};

class MipmapStorage {
public:
    MipmapStorage()
        : nlevel(0)
    {
    }
    MipmapStorage(int nl)
        : nlevel(nl)
    {
        _pyramids.resize(nl);
        std::fill(_pyramids.begin(), _pyramids.end(), nullptr);
    }
    ~MipmapStorage()
    {
        for (int i = 0; i < nlevel; ++i) {
            delete _pyramids[i];
        }
    }
    void set_pyramid(int index, sampler_info_t* samp)
    {
        if (index < 0 || index >= nlevel) {
            return;
        }
        _pyramids[index] = samp;
    }
    int get_nlevel()
    {
        return nlevel;
    }
    sampler_info_t* get_mipmap_sampler(int level_idx)
    {
        level_idx = glm::clamp(level_idx, 0, nlevel - 1);
        // printf("level_idx: %d\n", level_idx);
        return _pyramids[level_idx];
    }

private:
    std::vector<sampler_info_t*> _pyramids;
    int nlevel;
};

// the call back to enable glsl code to fetch data from graphics pipeline
typedef sampler_data_pack (*get_sampler2D_data_fptr)(int texunit_id);
typedef sampler_info_t* (*get_mipmap_sampler_fptr)(int thread_id, MipmapStorage *mipmap);
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
    virtual void set_mipmap_sampler_callback(get_mipmap_sampler_fptr func) = 0;
};

#ifdef GLSL_CODE

class GLSLShader;

// fp to member function
typedef void (GLSLShader::*set_uniform)(data_t& var);
typedef data_t (GLSLShader::*get_uniform)(void);

get_sampler2D_data_fptr get_sampler2D;
get_mipmap_sampler_fptr get_mipmap_sampler;

class sampler2D{
    public:
    sampler2D():loaded_texture(false){}
    bool loaded_texture;
    int texunit_id;
    int width;
    int height;
    // size = width * height
    int size;
    int color_format;
    int wrap_s;
    int wrap_t;
    glm::vec4 border_val;
    void* data;
    // filter_type filter;
    MipmapStorage* mipmap;
    int min_filter;
    int mag_filter;
    sampler2D& operator=(int val){
        texunit_id = val;
        loaded_texture = false;
        // printf("define value for sampler2D\n");
        return *this;
    }
    inline void load_texture(){
        if (loaded_texture){
            return;
        }
        sampler_data_pack tmp = get_sampler2D(texunit_id);
        mipmap = tmp.mipmap;
        width = tmp.width;
        height = tmp.height;
        size = width * height;
        color_format = tmp.color_format;
        data = tmp.tex_data;
        // filter = tmp.filter;
        min_filter = tmp.min_filter;
        mag_filter = tmp.mag_filter;
        wrap_s = tmp.wrap_s;
        wrap_t = tmp.wrap_t;
        loaded_texture = true;
        border_val = tmp.border_val;
    }
};

glm::vec4 texture(sampler2D &samp, glm::vec2 &texcoord)
{
    samp.load_texture();
    glm::vec4 res = glm::vec4(1.0f);
    if (samp.height == 0 || samp.width == 0)
        throw std::runtime_error("invalid texture used in shader\n");
    float scale = 1.0f / 255.0f;
    int channel;
    bool is_char = true;
    switch (samp.color_format) {
        case FORMAT_COLOR_8UC3:
            channel = 3;
            break;
        case FORMAT_COLOR_8UC4:
            channel = 4;
            // printf("using RGBA\n");
            break;
        case FORMAT_COLOR_32FC1:
            channel = 1;
            is_char = false;
            scale = 1.0f;
            break;
        default:
            throw std::runtime_error("invalid color format\n");
            break;
    }
    // direction s of texcoord
    float tx = texcoord.x, ty = texcoord.y;
    bool flag = false;
    if(tx < 0.0f || tx >= 1.0f){
        switch(samp.wrap_s){
            case GL_CLAMP_TO_BORDER:
                return samp.border_val;
            case GL_REPEAT:
            default:
                tx = tx - (int)tx;
                if(tx < 0.0f){
                    tx += 1.0f;
                }
                break;
        }
    }

    // direction t of texcoord
    if(ty < 0.0f || ty >= 1.0f){
        switch(samp.wrap_t){
            case GL_CLAMP_TO_BORDER:
                return samp.border_val;
            case GL_REPEAT:
            default:
                ty = ty - (int)ty;
                if(ty < 0.0f){
                    ty += 1.0f;
                }
                break;
        }
    }
    int width, height;
    void* data;
    if (is_char) {
        sampler_info_t* s = get_mipmap_sampler(omp_get_thread_num(), samp.mipmap);
        data = s->data;
        width = s->width;
        height = s->height;
    } else {
        data = samp.data;
        width = samp.width;
        height = samp.height;
    }
    // if (samp.min_filter == GL_LINEAR_MIPMAP_NEAREST) {
        int x = tx * width;
        int y = ty * height;
        int index = y * width + x;
        // printf("index: %d, width: %d, height: %d\n", index, width, height);
        for (int i = 0; i < channel; ++i) {
            if (is_char){
                unsigned char* sampler_data_array = (unsigned char*)data;
                res[i] = ((float)sampler_data_array[index * channel + i]) * scale;
            }else{
                float * sampler_data_array = (float*)data;
                res[i] = ((float)sampler_data_array[index * channel + i]) * scale;
            }
        }
        // printf("res = [ %d, %d, %d ]\n",(int) res[0],(int) res[1],(int) res[2]);
    // }
    // else if (samp.filter == filter_type::LINEAR)
    // {
    //     float x = tx * samp.width;
    //     float y = ty * samp.height;
    //     glm::vec4 u00, u01, u10, u11;
    //     int i00, i01, i11, i10;
    //     i00 = (int)y * samp.width + (int)x;
    //     i01 = (int)(y + 0.5f) * samp.width + (int)x;
    //     i10 = (int)y * samp.width + (int)(x + 0.5f);
    //     i11 = (int)(y + 0.5f) * samp.width + (int)(x + 0.5f);
    //     i01 = i00 >= samp.size ? samp.size - 1 : i00;
    //     i10 = i10 >= samp.size ? samp.size - 1 : i10;
    //     i11 = i11 >= samp.size ? samp.size - 1 : i11;
    //     for (int i = 0;i < channel;++i){
    //         if (is_char){
    //             unsigned char* sampler_data_array = (unsigned char*)samp.data;
    //             u00[i] = ((float)sampler_data_array[i00 * channel + i]) * scale;
    //             u01[i] = ((float)sampler_data_array[i01 * channel + i]) * scale;
    //             u10[i] = ((float)sampler_data_array[i10 * channel + i]) * scale;
    //             u11[i] = ((float)sampler_data_array[i11 * channel + i]) * scale;
    //         }else{
    //             float * sampler_data_array = (float*)samp.data;
    //             u00[i] = ((float)sampler_data_array[i00 * channel + i]) * scale;
    //             u01[i] = ((float)sampler_data_array[i01 * channel + i]) * scale;
    //             u10[i] = ((float)sampler_data_array[i10 * channel + i]) * scale;
    //             u11[i] = ((float)sampler_data_array[i11 * channel + i]) * scale;
    //         }
    //     }
    //     float s = x - (int)x;
    //     float t = y - (int)y;
    //     glm::vec4 u0 = u00 + s * (u10 - u00);
    //     glm::vec4 u1 = u01 + s * (u11 - u01);

    //     // lerp
    //     res = u0 + t * (u1 - u0);
    // }

    return res;
}

#endif

#endif