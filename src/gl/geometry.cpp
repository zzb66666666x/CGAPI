#include "configs.h"
#include "geometry.h"
#include "render.h"
#include "glcontext.h"

bool Triangle::inside(float x, float y)
{
    glm::vec3 f0, f1, f2;
    glm::vec3 v[3];
    for (int i = 0; i < 3; ++i)
    {
        #if(SAVE_POINTERS_IN_TRIANGLE == 0)
        v[i] = {screen_pos[i].x, screen_pos[i].y, 1.0};
        #endif
        #if(SAVE_POINTERS_IN_TRIANGLE == 1)
        v[i] = {screen_pos_ptrs[i]->x, screen_pos_ptrs[i]->y, 1.0};
        #endif
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
    #if(SAVE_POINTERS_IN_TRIANGLE == 0)
    glm::vec4 *v = screen_pos;
    float c1 = (x*(v[1].y - v[2].y) + (v[2].x - v[1].x)*y + v[1].x*v[2].y - v[2].x*v[1].y) / (v[0].x*(v[1].y - v[2].y) + (v[2].x - v[1].x)*v[0].y + v[1].x*v[2].y - v[2].x*v[1].y);
    float c2 = (x*(v[2].y - v[0].y) + (v[0].x - v[2].x)*y + v[2].x*v[0].y - v[0].x*v[2].y) / (v[1].x*(v[2].y - v[0].y) + (v[0].x - v[2].x)*v[1].y + v[2].x*v[0].y - v[0].x*v[2].y);
    float c3 = (x*(v[0].y - v[1].y) + (v[1].x - v[0].x)*y + v[0].x*v[1].y - v[1].x*v[0].y) / (v[2].x*(v[0].y - v[1].y) + (v[1].x - v[0].x)*v[2].y + v[0].x*v[1].y - v[1].x*v[0].y);
    #endif
    #if(SAVE_POINTERS_IN_TRIANGLE == 1)
    glm::vec4 ** v = screen_pos_ptrs;
    float c1 = (x*(v[1]->y - v[2]->y) + (v[2]->x - v[1]->x)*y + v[1]->x*v[2]->y - v[2]->x*v[1]->y) / (v[0]->x*(v[1]->y - v[2]->y) + (v[2]->x - v[1]->x)*v[0]->y + v[1]->x*v[2]->y - v[2]->x*v[1]->y);
    float c2 = (x*(v[2]->y - v[0]->y) + (v[0]->x - v[2]->x)*y + v[2]->x*v[0]->y - v[0]->x*v[2]->y) / (v[1]->x*(v[2]->y - v[0]->y) + (v[0]->x - v[2]->x)*v[1]->y + v[2]->x*v[0]->y - v[0]->x*v[2]->y);
    float c3 = (x*(v[0]->y - v[1]->y) + (v[1]->x - v[0]->x)*y + v[0]->x*v[1]->y - v[1]->x*v[0]->y) / (v[2]->x*(v[0]->y - v[1]->y) + (v[1]->x - v[0]->x)*v[2]->y + v[0]->x*v[1]->y - v[1]->x*v[0]->y);
    #endif
    return {c1, c2, c3};
}

TriangleCrawler::TriangleCrawler(){
    data_float_vec4.emplace(VSHADER_OUT_POSITION, std::vector<glm::vec4>());
    data_float_vec3.emplace(VSHADER_OUT_COLOR, std::vector<glm::vec3>());
    data_float_vec3.emplace(VSHADER_OUT_NORMAL, std::vector<glm::vec3>());
    data_float_vec3.emplace(VSHADER_OUT_FRAGPOS, std::vector<glm::vec3>());
    data_float_vec2.emplace(VSHADER_OUT_TEXCOORD, std::vector<glm::vec2>());
    indices.emplace(VSHADER_OUT_POSITION, 0);
    indices.emplace(VSHADER_OUT_COLOR, 0);
    indices.emplace(VSHADER_OUT_FRAGPOS, 0);
    indices.emplace(VSHADER_OUT_NORMAL, 0);
    indices.emplace(VSHADER_OUT_TEXCOORD, 0);
}

int TriangleCrawler::crawl(char* source, int buf_size, int first_vertex, glProgram& shader){
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
    data_float_vec4[VSHADER_OUT_POSITION].push_back(shader.gl_Position);
    data_float_vec3[VSHADER_OUT_COLOR].push_back(shader.gl_VertexColor);
    data_float_vec3[VSHADER_OUT_FRAGPOS].push_back(shader.frag_Pos);
    data_float_vec3[VSHADER_OUT_NORMAL].push_back(shader.gl_Normal);
    data_float_vec2[VSHADER_OUT_TEXCOORD].push_back(shader.iTexcoord);
    return GL_SUCCESS; 
}

void merge_crawlers(TriangleCrawler* crawlers){
    Triangle * tri = new Triangle;
    int cnt = 0;
    int crawlerID = 0;
    int flag = 1;
    std::map<int, int>::iterator index_iter;
    while (flag){
        TriangleCrawler& crawler = crawlers[crawlerID];
        crawlerID = (crawlerID+1) % PROCESS_VERTEX_THREAD_COUNT;
        if (cnt%3 == 0 && cnt>0){
            // for compatibility of old functions (they use queue to store triangle pointers)
            glapi_ctx->pipeline.triangle_stream.push(tri);
            // for parallel computation in binning and rasterizing, use std vector to store tri pointers
            // since using a queue requires a mutex lock
            glapi_ctx->pipeline.triangle_ptrs.push_back(tri);
            tri = new Triangle;
        }
        if(flag){
            // vec2
            std::map<int, std::vector<glm::vec2>>::iterator it;
            glm::vec2 * vec_ptr;
            for (it = crawler.data_float_vec2.begin(); it != crawler.data_float_vec2.end(); it++){
                vec_ptr = nullptr;
                index_iter = crawler.indices.find(it->first);
                switch(it->first){
                    case VSHADER_OUT_TEXCOORD:
                        vec_ptr = &(tri->texcoord[cnt%3]);
                        break;
                    default:
                        break;
                }
                if (index_iter->second >= it->second.size() || vec_ptr == nullptr){
                    flag = 0;
                    delete tri;
                    tri = nullptr;
                    break;
                }
                else{
                    *vec_ptr = it->second[index_iter->second];
                    index_iter->second ++;
                }
            }
        }
        if (flag){
            // vec3 
            std::map<int,std::vector<glm::vec3>>::iterator it;
            glm::vec3* vec_ptr;
            for (it = crawler.data_float_vec3.begin(); it != crawler.data_float_vec3.end(); it++){
                vec_ptr = nullptr;
                index_iter = crawler.indices.find(it->first);
                switch(it->first){
                    case VSHADER_OUT_COLOR:
                        vec_ptr = &(tri->color[cnt%3]);
                        break;
                    case VSHADER_OUT_NORMAL:
                        vec_ptr = &(tri->vert_normal[cnt%3]);
                        break;
                    case VSHADER_OUT_FRAGPOS:
                        vec_ptr = &(tri->frag_shading_pos[cnt%3]);
                        break;
                    default:
                        break;
                }
                if (index_iter->second >= it->second.size() || vec_ptr==nullptr){
                    flag = 0;
                    delete tri;
                    tri = nullptr;
                    break;
                }
                else{
                    *vec_ptr = it->second[index_iter->second];
                    index_iter->second ++;
                }
            }
        }
        if (flag){
            // vec4
            std::map<int, std::vector<glm::vec4>>::iterator it;
            glm::vec4* vec_ptr;
            for (it = crawler.data_float_vec4.begin(); it != crawler.data_float_vec4.end(); it++){
                vec_ptr = nullptr;
                index_iter = crawler.indices.find(it->first);
                switch(it->first){
                    case VSHADER_OUT_POSITION:
                        vec_ptr = &(tri->screen_pos[cnt%3]);
                        break;
                    default:
                        break;
                }
                if (index_iter->second >= it->second.size() || vec_ptr==nullptr){
                    flag = 0;
                    delete tri;
                    tri = nullptr;
                    break;
                }
                else{
                    *vec_ptr = it->second[index_iter->second];
                    index_iter->second ++;
                }
            }
        }
        cnt++;
    }
}

void merge_crawlers_faster(TriangleCrawler* crawlers){
    Triangle * tri = new Triangle;
    int cnt = 0;
    int crawlerID = 0;
    int flag = 1;
    std::map<int, int>::iterator index_iter;
    while (flag){
        TriangleCrawler& crawler = crawlers[crawlerID];
        crawlerID = (crawlerID+1) % PROCESS_VERTEX_THREAD_COUNT;
        if (cnt%3 == 0 && cnt>0){
            glapi_ctx->pipeline.triangle_ptrs.push_back(tri);
            tri = new Triangle;
        }
        if(flag){
            // vec2
            std::map<int, std::vector<glm::vec2>>::iterator it;
            glm::vec2 ** vec_ptr;
            for (it = crawler.data_float_vec2.begin(); it != crawler.data_float_vec2.end(); it++){
                vec_ptr = nullptr;
                index_iter = crawler.indices.find(it->first);
                switch(it->first){
                    case VSHADER_OUT_TEXCOORD:
                        vec_ptr = &(tri->texcoord_ptrs[cnt%3]);
                        break;
                    default:
                        break;
                }
                if (index_iter->second >= it->second.size() || vec_ptr == nullptr){
                    flag = 0;
                    delete tri;
                    tri = nullptr;
                    break;
                }
                else{
                    *vec_ptr = &(it->second[index_iter->second]);
                    index_iter->second ++;
                }
            }
        }
        if (flag){
            // vec3 
            std::map<int,std::vector<glm::vec3>>::iterator it;
            glm::vec3 ** vec_ptr;
            for (it = crawler.data_float_vec3.begin(); it != crawler.data_float_vec3.end(); it++){
                vec_ptr = nullptr;
                index_iter = crawler.indices.find(it->first);
                switch(it->first){
                    case VSHADER_OUT_COLOR:
                        vec_ptr = &(tri->color_ptrs[cnt%3]);
                        break;
                    case VSHADER_OUT_NORMAL:
                        vec_ptr = &(tri->vert_normal_ptrs[cnt%3]);
                        break;
                    case VSHADER_OUT_FRAGPOS:
                        vec_ptr = &(tri->frag_shading_pos_ptrs[cnt%3]);
                        break;
                    default:
                        break;
                }
                if (index_iter->second >= it->second.size() || vec_ptr==nullptr){
                    flag = 0;
                    delete tri;
                    tri = nullptr;
                    break;
                }
                else{
                    *vec_ptr = &(it->second[index_iter->second]);
                    index_iter->second ++;
                }
            }
        }
        if (flag){
            // vec4
            std::map<int, std::vector<glm::vec4>>::iterator it;
            glm::vec4 ** vec_ptr;
            for (it = crawler.data_float_vec4.begin(); it != crawler.data_float_vec4.end(); it++){
                vec_ptr = nullptr;
                index_iter = crawler.indices.find(it->first);
                switch(it->first){
                    case VSHADER_OUT_POSITION:
                        vec_ptr = &(tri->screen_pos_ptrs[cnt%3]);
                        break;
                    default:
                        break;
                }
                if (index_iter->second >= it->second.size() || vec_ptr==nullptr){
                    flag = 0;
                    delete tri;
                    tri = nullptr;
                    break;
                }
                else{
                    *vec_ptr = &(it->second[index_iter->second]);
                    index_iter->second ++;
                }
            }
        }
        cnt++;
    }
}