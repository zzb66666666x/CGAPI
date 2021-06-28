#include "configs.h"
#include "geometry.h"
#include "globj.h"

bool Triangle::inside(float x, float y)
{
    glm::vec3 f0, f1, f2;
    glm::vec3 v[3];
    for (int i = 0; i < 3; ++i)
    {
        v[i] = {screen_pos[i].x, screen_pos[i].y, 1.0};
    }
    f0 = glm::cross(v[1], v[0]);
    f1 = glm::cross(v[2], v[1]);
    f2 = glm::cross(v[0], v[2]);
    glm::vec3 p(x, y, 1.0f);
    if ((glm::dot(p, f0) * glm::dot(f0, v[2]) > 0) && (glm::dot(p, f1) * glm::dot(f1, v[0]) > 0) && (glm::dot(p, f2) * glm::dot(f2, v[1]) > 0))
    {
        return true;
    }
    return false;
}

glm::vec3 Triangle::computeBarycentric2D(float x, float y)
{
    glm::vec4 *v = screen_pos;
    float c1 = (x * (v[1].y - v[2].y) + (v[2].x - v[1].x) * y + v[1].x * v[2].y - v[2].x * v[1].y) / (v[0].x * (v[1].y - v[2].y) + (v[2].x - v[1].x) * v[0].y + v[1].x * v[2].y - v[2].x * v[1].y);
    float c2 = (x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * y + v[2].x * v[0].y - v[0].x * v[2].y) / (v[1].x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * v[1].y + v[2].x * v[0].y - v[0].x * v[2].y);
    float c3 = (x * (v[0].y - v[1].y) + (v[1].x - v[0].x) * y + v[0].x * v[1].y - v[1].x * v[0].y) / (v[2].x * (v[0].y - v[1].y) + (v[1].x - v[0].x) * v[2].y + v[0].x * v[1].y - v[1].x * v[0].y);
    // assert(_sanity_check(c1, c2, c3) == 1 );
    return {c1, c2, c3};
}

int TriangleCrawler::crawl(char* source, int buf_size, int first_vertex, glProgram& shader){
    void* input_ptr;
    char* buf;
    for (int i = 0; i < shader.layout_cnt; i++) {
        if (shader.layouts[i] > 3) {
            throw std::runtime_error("invalid layout\n");
        }
        switch (shader.layouts[i]) {
        case LAYOUT_POSITION:
            input_ptr = &(shader.input_Pos);
            break;
        case LAYOUT_COLOR:
            input_ptr = &(shader.vert_Color);
            break;
        case LAYOUT_TEXCOORD:
            input_ptr = &(shader.iTexcoord);
            break;
        case LAYOUT_NORMAL:
            input_ptr = &(shader.vert_Normal);
            break;
        case LAYOUT_INVALID:
        default:
            input_ptr = nullptr;
            break;
        }
        if (input_ptr == nullptr)
            continue;
        int layout = shader.layouts[i];
        buf = source + first_vertex * (config.strides[layout]) +
                (config.indices[layout] + config.offsets[layout]);
        switch (config.dtypes[layout]) {
        case GL_FLOAT:
            switch (config.sizes[layout]) {
            case 2: {
                glm::vec2* vec2 = (glm::vec2*)input_ptr;
                vec2->x = *(float*)(buf + 0);
                vec2->y = *(float*)(buf + sizeof(float) * 1);
                break;
            }
            case 3: {
                glm::vec3* vec3 = (glm::vec3*)input_ptr;
                vec3->x = *(float*)(buf + 0);
                vec3->y = *(float*)(buf + sizeof(float) * 1);
                vec3->z = *(float*)(buf + sizeof(float) * 2);
                break;
            }
            default:
                throw std::runtime_error("not supported size\n");
            }
            break;
        default:
            throw std::runtime_error("not supported type\n");
        }
        config.indices[layout] += config.strides[layout];
        if (config.indices[layout] >= buf_size) {
            return GL_FAILURE;
        }
    }
    return GL_SUCCESS; 
}
