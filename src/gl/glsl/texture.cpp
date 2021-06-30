#include "texture.h"
#include "../glcontext.h"
#include <math.h>

glm::vec4 texture2D(sampler2D &texture, glm::vec2 &texcoord)
{
    glm::vec4 res = glm::vec4(1.0f);
    if (texture.config.height == 0 || texture.config.width == 0)
        throw std::runtime_error("invalid texture used in shader\n");
    float x = texcoord.x * texture.config.width;
    float y = texcoord.y * texture.config.height;
    int channel;
    if (texture.filter == filter_type::NEAREST)
    {
        x += 0.5f;
        y += 0.5f;
        int index = ((int)y % texture.config.height) * texture.config.width + ((int)x % texture.config.width);
        switch(texture.config.color_format){
            case FORMAT_COLOR_8UC3:
                channel = 3;
                for (int i = 0; i < channel; ++i)
                {
                    res[i] = (float) texture.data[index * channel + i];
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