#include "configs.h"
#include "geometry.h"
#include "render.h"
#include "glcontext.h"

bool Triangle::inside(float x, float y)
{
    glm::vec3 f0, f1, f2;
    glm::vec3 v[3];
    for (int i = 0; i < 3; ++i) {
        v[i] = { screen_pos[i].x, screen_pos[i].y, 1.0 };
    }
    f0 = glm::cross(v[1], v[0]);
    f1 = glm::cross(v[2], v[1]);
    f2 = glm::cross(v[0], v[2]);
    glm::vec3 p(x, y, 1.0f);
    if ((glm::dot(p, f0) * glm::dot(f0, v[2]) > 0) && (glm::dot(p, f1) * glm::dot(f1, v[0]) > 0) && (glm::dot(p, f2) * glm::dot(f2, v[1]) > 0)) {
        return true;
    }
    return false;
}

glm::vec3 Triangle::computeBarycentric2D(float x, float y)
{
    glm::vec4* v = screen_pos;
    float c1 = (x * (v[1].y - v[2].y) + (v[2].x - v[1].x) * y + v[1].x * v[2].y - v[2].x * v[1].y) / (v[0].x * (v[1].y - v[2].y) + (v[2].x - v[1].x) * v[0].y + v[1].x * v[2].y - v[2].x * v[1].y);
    float c2 = (x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * y + v[2].x * v[0].y - v[0].x * v[2].y) / (v[1].x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * v[1].y + v[2].x * v[0].y - v[0].x * v[2].y);
    float c3 = (x * (v[0].y - v[1].y) + (v[1].x - v[0].x) * y + v[0].x * v[1].y - v[1].x * v[0].y) / (v[2].x * (v[0].y - v[1].y) + (v[1].x - v[0].x) * v[2].y + v[0].x * v[1].y - v[1].x * v[0].y);
    return { c1, c2, c3 };
}

TriangleCrawler::TriangleCrawler()
{
    data_float_vec4.emplace(VSHADER_OUT_POSITION, std::queue<glm::vec4>());
    data_float_vec3.emplace(VSHADER_OUT_COLOR, std::queue<glm::vec3>());
    data_float_vec3.emplace(VSHADER_OUT_NORMAL, std::queue<glm::vec3>());
    data_float_vec3.emplace(VSHADER_OUT_FRAGPOS, std::queue<glm::vec3>());
    data_float_vec2.emplace(VSHADER_OUT_TEXCOORD, std::queue<glm::vec2>());
}

int TriangleCrawler::crawl(char* source, int buf_size, int first_vertex, glProgram& shader)
{
    void* input_ptr;
    char* buf;
    GET_CURRENT_CONTEXT(C);
    for (int i = 0; i < shader.layout_cnt; i++) {
        // looping over the layout and prepare for one vertex shader call
        int layout = shader.layouts[i];
        if (layout > 3) {
            throw std::runtime_error("invalid layout\n");
        }
        if (config.indices[layout] >= buf_size) {
            return GL_FAILURE;
        }
        switch (layout) {
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
        buf = source + first_vertex * (config.strides[layout]) + (config.indices[layout] + config.offsets[layout]);
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
        config.indices[layout] += config.strides[layout] * PROCESS_VERTEX_THREAD_COUNT;
        // finish moving the crawler's forward for current layout
    }
    // finish setting up the thread local shader
    shader.default_vertex_shader();
    shader.gl_Position.x /= shader.gl_Position.w;
    shader.gl_Position.y /= shader.gl_Position.w;
    shader.gl_Position.z /= shader.gl_Position.w;
    shader.gl_Position.x = 0.5 * C->width * (shader.gl_Position.x + 1.0);
    shader.gl_Position.y = 0.5 * C->height * (shader.gl_Position.y + 1.0);
    // if (shader.gl_Position.x < 0 || shader.gl_Position.y < 0 || shader.gl_Position.x > C->width || shader.gl_Position.y > C->height){
    //     throw std::runtime_error("outside of screen?\n");
    // }
    shader.gl_Position.z = shader.gl_Position.z * 0.5 + 0.5;
    data_float_vec4[VSHADER_OUT_POSITION].push(shader.gl_Position);
    data_float_vec3[VSHADER_OUT_COLOR].push(shader.gl_VertexColor);
    data_float_vec3[VSHADER_OUT_FRAGPOS].push(shader.frag_Pos);
    data_float_vec3[VSHADER_OUT_NORMAL].push(shader.gl_Normal);
    data_float_vec2[VSHADER_OUT_TEXCOORD].push(shader.iTexcoord);
    return GL_SUCCESS;
}

bool ProgrammableTriangle::inside(float x, float y)
{
    glm::vec3 f0, f1, f2;
    glm::vec3 v[3];
    v[0] = { screen_pos[0].x, screen_pos[0].y, 1.0 };
    v[1] = { screen_pos[1].x, screen_pos[1].y, 1.0 };
    v[2] = { screen_pos[2].x, screen_pos[2].y, 1.0 };
    f0 = glm::cross(v[1], v[0]);
    f1 = glm::cross(v[2], v[1]);
    f2 = glm::cross(v[0], v[2]);
    glm::vec3 p(x, y, 1.0f);
    return (glm::dot(p, f0) * glm::dot(f0, v[2]) > 0) && (glm::dot(p, f1) * glm::dot(f1, v[0]) > 0) && (glm::dot(p, f2) * glm::dot(f2, v[1]) > 0);
}

glm::vec3 ProgrammableTriangle::computeBarycentric2D(float x, float y)
{
    glm::vec4* v = screen_pos;
    float c1 = (x * (v[1].y - v[2].y) + (v[2].x - v[1].x) * y + v[1].x * v[2].y - v[2].x * v[1].y) / (v[0].x * (v[1].y - v[2].y) + (v[2].x - v[1].x) * v[0].y + v[1].x * v[2].y - v[2].x * v[1].y);
    float c2 = (x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * y + v[2].x * v[0].y - v[0].x * v[2].y) / (v[1].x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * v[1].y + v[2].x * v[0].y - v[0].x * v[2].y);
    float c3 = (x * (v[0].y - v[1].y) + (v[1].x - v[0].x) * y + v[0].x * v[1].y - v[1].x * v[0].y) / (v[2].x * (v[0].y - v[1].y) + (v[1].x - v[0].x) * v[2].y + v[0].x * v[1].y - v[1].x * v[0].y);
    return { c1, c2, c3 };
}

bool ProgrammableTriangle::outside_clip_space()
{
#ifndef GL_SCANLINE
    for (int i = 0; i < 3; ++i) {
        if ((screen_pos[0][i] > screen_pos[0].w && screen_pos[1][i] > screen_pos[1].w && screen_pos[2][i] > screen_pos[2].w)
            || (screen_pos[0][i] < -screen_pos[0].w && screen_pos[1][i] < -screen_pos[1].w && screen_pos[2][i] < -screen_pos[2].w)) {
            return true;
        }
    }
#else
    for (int i = 0; i < 3; ++i) {
        if ((vertices[0].screen_pos[i] > vertices[0].screen_pos.w && vertices[1].screen_pos[i] > vertices[1].screen_pos.w && vertices[2].screen_pos[i] > vertices[2].screen_pos.w)
            || (vertices[0].screen_pos[i] < -vertices[0].screen_pos.w && vertices[1].screen_pos[i] < -vertices[1].screen_pos.w && vertices[2].screen_pos[i] < -vertices[2].screen_pos.w)) {
            return true;
        }
    }
#endif
    return false;
}
bool ProgrammableTriangle::all_inside_clip_space()
{
#ifndef GL_SCANLINE
    for (int i = 0; i < 3; ++i) {
        if (!(screen_pos[i].x < screen_pos[i].w && screen_pos[i].x > -screen_pos[i].w
                && screen_pos[i].y < screen_pos[i].w && screen_pos[i].y > -screen_pos[i].w
                && screen_pos[i].z < screen_pos[i].w && screen_pos[i].z > -screen_pos[i].w)) {
            return false;
        }
    }
#else
    for (int i = 0; i < 3; ++i) {
        if (!(vertices[i].screen_pos.x < vertices[i].screen_pos.w && vertices[i].screen_pos.x > -vertices[i].screen_pos.w
                && vertices[i].screen_pos.y < vertices[i].screen_pos.w && vertices[i].screen_pos.y > -vertices[i].screen_pos.w
                && vertices[i].screen_pos.z < vertices[i].screen_pos.w && vertices[i].screen_pos.z > -vertices[i].screen_pos.w)) {
            return false;
        }
    }
#endif
    return true;
}
bool ProgrammableTriangle::inside_plane(const glm::vec4& plane, glm::vec4& pos)
{
    return glm::dot(pos, plane) < 0;
}
ProgrammableVertex ProgrammableTriangle::intersect(ProgrammableVertex& v1, ProgrammableVertex& v2, const glm::vec4& plane)
{
    float d1 = glm::dot(v1.screen_pos, plane);
    float d2 = glm::dot(v2.screen_pos, plane);
    float weight = d1 / (d1 - d2);
    return ProgrammableVertex::lerp(cur_shader, v1, v2, weight);
}

void ProgrammableTriangle::view_frustum_culling(const std::vector<glm::vec4>& planes, std::vector<ProgrammableTriangle*>& res)
{
    // if all vertices are outside or inside, it will return directly.
    if (all_inside_clip_space() || (this->culling = outside_clip_space())) {
        return;
    }
    // for loop
    int i, j, len, jlen;
    // current triangle will be culling.
    this->culling = true;

    std::vector<ProgrammableVertex> vertex_list(3);
    for (int i = 0; i < 3; ++i) {
        // vertex_list[i] = t.getVertex(i);
#ifndef GL_SCANLINE
        vertex_list[i].screen_pos = this->screen_pos[i];
        vertex_list[i].vertex_attrib = this->vertex_attribs[i];
#else
        vertex_list[i].screen_pos = this->vertices[i].screen_pos;
        vertex_list[i].vertex_attrib = this->vertices[i].vertex_attrib;
#endif
    }
    int size = 3;
    for (i = 0, len = planes.size(); i < len; ++i) {
        std::vector<ProgrammableVertex> input;
        input.insert(input.end(), vertex_list.begin(), vertex_list.begin() + size);
        size = 0;
        for (j = 0, jlen = input.size(); j < jlen; ++j) {
            ProgrammableVertex& current = input[j];
            ProgrammableVertex& last = input[(j + jlen - 1) % jlen];
            if (inside_plane(planes[i], current.screen_pos)) {
                if (!inside_plane(planes[i], last.screen_pos)) {
                    if (size < vertex_list.size()) {
                        vertex_list[size] = this->intersect(last, current, planes[i]);
                    } else { 
                        vertex_list.push_back(this->intersect(last, current, planes[i]));
                    }
                    ++size;
                }
                if (size < vertex_list.size()) {
                    vertex_list[size] = current;
                } else {
                    vertex_list.push_back(current);
                }
                ++size;
            } else if (inside_plane(planes[i], last.screen_pos)) {
                if (size < vertex_list.size()) {
                    vertex_list[size] = this->intersect(last, current, planes[i]);
                } else {
                    vertex_list.push_back(this->intersect(last, current, planes[i]));
                }
                ++size;
            }
        }
    }
    // printf("vertex_list size: %d\n", vertex_list.size());
    if (size < 3) {
        return;
    }

    ProgrammableTriangle* tri = nullptr;
    res.resize(size - 3 + 1);
    int next = 1;
    for (i = 0, len = res.size(); i < len; ++i) {
        tri = new ProgrammableTriangle();
        tri->setVertex(0, vertex_list[0]);
        tri->setVertex(1, vertex_list[next]);
        tri->setVertex(2, vertex_list[++next]);
        res[i] = tri;
    }
}

void ProgrammableTriangle::view_frustum_culling(const std::vector<glm::vec4>& planes, std::vector<ProgrammableTriangle*>& res, int &vfc_size)
{
    vfc_size = 0;
    // if all vertices are outside or inside, it will return directly.
    if (all_inside_clip_space() || (this->culling = outside_clip_space())) {
        return;
    }
    // for loop
    int i, j, len, jlen;
    // current triangle will be culling.
    this->culling = true;

    std::vector<ProgrammableVertex> vertex_list(3);
    for (int i = 0; i < 3; ++i) {
        // vertex_list[i] = t.getVertex(i);
#ifndef GL_SCANLINE
        vertex_list[i].screen_pos = this->screen_pos[i];
        vertex_list[i].vertex_attrib = this->vertex_attribs[i];
#else
        vertex_list[i].screen_pos = this->vertices[i].screen_pos;
        vertex_list[i].vertex_attrib = this->vertices[i].vertex_attrib;
#endif
    }
    int size = 3;
    for (i = 0, len = planes.size(); i < len; ++i) {
        std::vector<ProgrammableVertex> input;
        input.insert(input.end(), vertex_list.begin(), vertex_list.begin() + size);
        size = 0;
        for (j = 0, jlen = input.size(); j < jlen; ++j) {
            ProgrammableVertex& current = input[j];
            ProgrammableVertex& last = input[(j + jlen - 1) % jlen];
            if (inside_plane(planes[i], current.screen_pos)) {
                if (!inside_plane(planes[i], last.screen_pos)) {
                    if (size < vertex_list.size()) {
                        vertex_list[size] = this->intersect(last, current, planes[i]);
                    } else {
                        vertex_list.push_back(this->intersect(last, current, planes[i]));
                    }
                    ++size;
                }
                if (size < vertex_list.size()) {
                    vertex_list[size] = current;
                } else {
                    vertex_list.push_back(current);
                }
                ++size;
            } else if (inside_plane(planes[i], last.screen_pos)) {
                if (size < vertex_list.size()) {
                    vertex_list[size] = this->intersect(last, current, planes[i]);
                } else {
                    vertex_list.push_back(this->intersect(last, current, planes[i]));
                }
                ++size;
            }
        }
    }
    // printf("vertex_list size: %d\n", vertex_list.size());
    if (size < 3) {
        return;
    }

    vfc_size = size - 3 + 1;
    if(res.size() < vfc_size){
        i = res.size();
        res.resize(vfc_size);
        for (; i < vfc_size; ++i) {
            res[i] = new ProgrammableTriangle();
        }
    }

    int next = 1;
    for (i = 0; i < vfc_size; ++i) {
        res[i]->setVertex(0, vertex_list[0]);
        res[i]->setVertex(1, vertex_list[next]);
        res[i]->setVertex(2, vertex_list[++next]);
    }
}