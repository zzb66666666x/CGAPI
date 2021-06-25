#ifndef _glsl_texture_h_
#define _glsl_texture_h_

#include "../configs.h"
#include <glm/glm.hpp>
#include "../formats.h"

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
    sampler_config config;
    unsigned char *data;
    filter_type filter;
};

glm::vec4 texture2D(sampler2D &texture, glm::vec2 &texcoord);

#endif