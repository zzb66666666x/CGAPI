#ifndef _glsl_texture_h_
#define _glsl_texture_h_

#include <glm/glm.hpp>

typedef struct sampler2D sampler2D;

enum filter_type{
    NEAREST,
    BILINEAR
};

struct sampler2D{
    int width;
    int height;
    // RGB, RGBA
    int channel;    
    unsigned char *data;
    filter_type filter;
};

glm::vec4 texture2D(sampler2D &texture, glm::vec2 &texcoord);

#endif