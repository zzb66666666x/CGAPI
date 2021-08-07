#ifndef _glsl_texture_h_
#define _glsl_texture_h_

#include "../configs.h"
#include <glm/glm.hpp>
#include "../formats.h"
#include "../../../include/gl/common.h"
#include <vector>

// class debug_texture{
// private:
//     cv::Mat image_data;

// public:
//     debug_texture(const std::string& name);

//     int width, height;

//     glm::vec3 getColor(float u, float v);
// };

enum filter_type {
    NEAREST,
    LINEAR,
    NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR
};

class sampler_config{
    public:
    sampler_config()
    :   width(0),
        height(0),
        color_format(FORMAT_COLOR_8UC3),
        wrap_s(GL_REPEAT),
        wrap_t(GL_REPEAT),
        border_val(glm::vec4(0.0f)){}
    sampler_config(int w, int h, int cf)
    :   width(w),
        height(h),
        color_format(cf),
        wrap_s(GL_REPEAT),
        wrap_t(GL_REPEAT),
        border_val(glm::vec4(0.0f)){}
    sampler_config(int w, int h, int cf, int ws, int wt)
    :   width(w),
        height(h),
        color_format(cf),
        wrap_s(ws),
        wrap_t(wt),
        border_val(glm::vec4(0.0f)){}
    int width;
    int height;
    int color_format;
    int wrap_s;
    int wrap_t;
    int min_filter;
    int mag_filter;
    glm::vec4 border_val;
};

class sampler2D{
    public:
    sampler2D(){}
    sampler_config config;
    unsigned char *data;
    filter_type filter;
};

// 2D texture
glm::vec4 texture2D(sampler2D &texture, glm::vec2 &texcoord);

#endif