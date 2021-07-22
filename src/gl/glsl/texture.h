#ifndef _glsl_texture_h_
#define _glsl_texture_h_

#include "../configs.h"
#include <glm/glm.hpp>
#include "../formats.h"

// class debug_texture{
// private:
//     cv::Mat image_data;

// public:
//     debug_texture(const std::string& name);

//     int width, height;

//     glm::vec3 getColor(float u, float v);
// };

enum filter_type{
    NEAREST,
    BILINEAR
};

class sampler_config{
    public:
    sampler_config()
    : width(0),
      height(0),
      color_format(FORMAT_COLOR_8UC3){}
    sampler_config(int w, int h, int cf)
        : width(w),
          height(h),
          color_format(cf){}
    int width;
    int height;
    int color_format;
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