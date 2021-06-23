#include "texture.h"
#include "../glcontext.h"
#include <math.h>

glm::vec4 texture2D(sampler2D &texture, glm::vec2 &texcoord)
{
    glm::vec4 res = glm::vec4(1.0f);
    float x = texcoord.x * texture.width;
    float y = (1.0f - texcoord.y) * texture.height;

    if (texture.filter == filter_type::NEAREST)
    {
        x += 0.5f;
        y += 0.5f;
        int index = ((int)y % texture.height) * texture.width + ((int)x % texture.width);
        for (int i = 0; i < texture.channel; ++i)
        {
            res[i] = (float) texture.data[index * texture.channel + i];
        }
    }
    else if (texture.filter == filter_type::BILINEAR)
    {
        // TODO
    }

    return res;
}