#include "configs.h"
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include "../../include/gl/gl.h"
#include "../../include/gl/common.h"
#include "glcontext.h"
#include "formats.h"
#include "glsl/texture.h"

#define FILL_ZERO   -1
#define FILL_ONE    -2
#define SKIP_INPUT  -3

typedef struct{
    char in_channels;
    char out_channels;
    char order[4];
    bool direct_pass;
}format_desc_t;

//static helpers
static void _pass_tex_data(int ID, glObject* ptr, GLenum internalFormat, int width, int height, GLenum format, GLenum type, void* data);
static void _tex_data_formatter(GLenum internalFormat, GLenum format, format_desc_t* desc);

// helloworld
void glHelloWorld(){
    std::cout<<"hello and welcome to gl world written by ZJU student!"<<std::endl;
}

// Gen
void glGenBuffers(int num, unsigned int * ID){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto& bufs = C->share.buffers;
    int ret;
    for (int i=0; i<num; i++){
        ret = bufs.insertStorage(GL_BYTE, 0, GL_UNDEF);
        ID[i] = ret;
    }
}

void glGenVertexArrays(int num, unsigned int* ID){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto& attribs = C->share.vertex_attribs;
    int ret;
    // use glStorage<vertex_attrib_t> 
    // different VAO ids are maintained by glManager
    // each VAO owns on set of properties, stored in one glStorage<vertex_attrib_t>
    // each property are entries to the data array of the glStorage<vertex_attrib_t>
    // within each property, the config are wrapped in struct vertex_attrib_t
    for (int i=0; i<num; i++){
        ret = attribs.insertStorage(GL_VERTEX_ATTRIB_CONFIG, GL_MAX_VERTEX_ATTRIB_NUM,  GL_BIND_VAO);
        ID[i] = ret;    // if failure, then ID will be -1
    }
}

void glGenTextures(int num, unsigned int* ID){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto& texs = C->share.textures;
    int ret;
    for (int i=0; i<num; i++){
        ret = texs.insertStorage(GL_BYTE, 0, GL_UNDEF);
        ID[i] = ret;
    }
}

// Bind & Activate
void glBindBuffer(GLenum buf_type, unsigned int ID){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    if (ID <= 0) {
        return;
    }
    auto& bufs = C->share.buffers;
    int ret;
    glObject* ptr;
    switch(buf_type){
        case GL_ARRAY_BUFFER:
            // try search out the ID
            ret = bufs.searchStorage(&ptr, ID);
            if (ret == GL_FAILURE){
                return;
            }
            ptr->bind = GL_ARRAY_BUFFER;
            C->payload.renderMap[GL_ARRAY_BUFFER] = ID;
            break;
        case GL_ELEMENT_ARRAY_BUFFER:
            ret = bufs.searchStorage(&ptr, ID);
            if (ret == GL_FAILURE) {
                return;
            }
            ptr->bind = GL_ELEMENT_ARRAY_BUFFER;
            C->payload.renderMap[GL_ELEMENT_ARRAY_BUFFER] = ID;
            break;
        default:
            break;
    }
}

void glBindVertexArray(unsigned int ID){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto& mgr = C->share.vertex_attribs;
    if (ID<0)
        return;
    else if (ID == 0){
        //unbind
        C->payload.renderMap[GL_BIND_VAO] = -1;
    }
    else{
        // verify that the ID is valid
        glObject* ptr;
        int ret;
        ret = mgr.searchStorage(&ptr, ID);
        if (ret == GL_FAILURE){
            return;
        }
        C->payload.renderMap[GL_BIND_VAO] = ID;
    }
}

void glBindTexture(GLenum target,  unsigned int ID){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto& texs = C->share.textures;
    int ret;
    glObject* ptr;  
    switch(target){
        case GL_TEXTURE_2D:
            if (ID<0){
                return;
            }  
            else if (ID == 0){
                C->payload.renderMap[GL_TEXTURE_2D] = -1;
            }
            else{
                ret = texs.searchStorage(&ptr, ID);
                if (ret == GL_FAILURE)
                    return;
                C->payload.renderMap[GL_TEXTURE_2D] = ID;
            }
            break;
        default:
            break;
    }
    // after filling the render map, check out the render path
    for (auto& iter:C->payload.tex_units){
        if (iter.second == TEXTURE_UNIT_TBD){
            assert(ID>=1);
            iter.second = ID;
        }
    }
}

void glActiveTexture(GLenum unit){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto iter = C->payload.tex_units.find(unit);
    if(iter->second == TEXTURE_UNIT_CLOSE){
        iter->second = TEXTURE_UNIT_TBD;
    }
}

// Pass data
void glBufferData(GLenum buf_type, int nbytes, const void* data, GLenum usage){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto& bufs = C->share.buffers;
    int ID, ret;
    glObject* ptr;
    switch (usage){
        case GL_STATIC_DRAW:
            switch (buf_type){
                case GL_ARRAY_BUFFER:
                    ID = C->payload.renderMap[GL_ARRAY_BUFFER];
                    if (ID<0)
                        return; // not binded yet 
                    ret = bufs.searchStorage(&ptr, ID);
                    if (ret == GL_FAILURE){
                        return;
                    }
                    // copy data inside glStorage<char>
                    ptr->allocByteSpace(nbytes);
                    ptr->loadBytes(data, nbytes);
                    ptr->usage = usage;
                    // if GL_STATIC_DRAW notice cache to delete indices data
                    if(usage == GLenum::GL_STATIC_DRAW){
                        C->pipeline.indexCache.removeCacheData(C->payload.renderMap[GL_BIND_VAO]);
                    }
                    break;
                case GL_ELEMENT_ARRAY_BUFFER:
                    ID = C->payload.renderMap[GL_ELEMENT_ARRAY_BUFFER];
                    if (ID < 0) {
                        return;
                    }
                    ret = bufs.searchStorage(&ptr, ID);
                    if (ret == GL_FAILURE) {
                        return;
                    }
                    ptr->allocByteSpace(nbytes);
                    ptr->loadBytes(data, nbytes);
                    ptr->usage = usage;
                    // if GL_STATIC_DRAW notice cache to delete indices data
                    if (usage == GLenum::GL_STATIC_DRAW) {
                        C->pipeline.indexCache.removeCacheData(C->payload.renderMap[GL_BIND_VAO]);
                    }
                    // throw std::runtime_error("not written for EBO\n");
                    break;
                default:
                    break;
            }
            break;
        default: 
            break;
    }
}

void glTexImage2D(GLenum target, int level,  GLenum internalFormat, int width, int height, int border, GLenum format, GLenum type, void * data){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    if (border != 0 || 
        level !=0 || 
        (internalFormat!= GL_RGB)  )
        throw std::runtime_error("using invalid params when passing tex data \n");
    int ID, ret;
    glObject* tex_ptr;
    auto& texs = C->share.textures;
    switch(target){
        case GL_TEXTURE_2D:
            ID = C->payload.renderMap[GL_TEXTURE_2D];
            if (ID<0)
                return;
            ret = texs.searchStorage(&tex_ptr, ID);
            if (ret == GL_FAILURE)
                return;
            _pass_tex_data(ID, tex_ptr, internalFormat, width, height, format, type, data);
            break;
        default:
            break;
    }
}

static void _pass_tex_data(int ID, glObject* ptr, GLenum internalFormat, int width, int height, GLenum format, GLenum type, void* data){
    GET_CURRENT_CONTEXT(C);
    format_desc_t f_desc;
    switch(type){
        case GL_BYTE:
            // forced unsigned, otherwise the 255 will be treated as -1
        case GL_UNSIGNED_BYTE:{
            unsigned char* tex_data = (unsigned char*) data;
            int nbytes;
            _tex_data_formatter(internalFormat, format, &f_desc);
            if (f_desc.out_channels == 3){
                C->share.tex_config_map.emplace(ID, sampler_config(width, height, (int)FORMAT_COLOR_8UC3));
            }
            else{
                C->share.tex_config_map.emplace(ID, sampler_config(width, height, (int)FORMAT_COLOR_8UC4));
            }
            if (f_desc.direct_pass){
                nbytes = width * height * f_desc.out_channels;
                ptr->allocByteSpace(nbytes);
                memcpy(ptr->getDataPtr(), tex_data, nbytes);
            }
            else{
                throw std::runtime_error("not supporting format auto conversion yet\n");
            }
            }break;
        default:
            break;
    }
}

static void _tex_data_formatter(GLenum internalFormat, GLenum format, format_desc_t* desc){
    switch(format){
        case GL_RGB:
            desc->in_channels = 3;
            switch(internalFormat){
                case GL_RGB:
                    desc->out_channels = 3;
                    desc->direct_pass = true;
                    break;
                case GL_RGBA:
                    desc->out_channels = 4;
                    desc->order[0] = 0;
                    desc->order[1] = 1;
                    desc->order[2] = 2;
                    desc->order[3] = FILL_ONE;
                    break;
                default: 
                    break;
            }
            break;
        case GL_RGBA:
            desc->in_channels = 4;
            switch(internalFormat){
                case GL_RGB:
                    desc->out_channels = 3;
                    desc->direct_pass = false;
                    desc->order[0] = 0;
                    desc->order[1] = 1;
                    desc->order[2] = 2;
                    desc->order[3] = SKIP_INPUT;
                    break;
                case GL_RGBA:
                    desc->out_channels = 4;
                    desc->direct_pass = true;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

// Attrib 
void glVertexAttribPointer(int index, int size, GLenum dtype, bool normalized, int stride, void * pointer){
    // first reach out to render payload, fetch out the id
    // based on the id, modify the glStorage<vertex_attrib_t>
    // if we haven't bind, the setting here has not effect
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    if (dtype != GL_FLOAT)
        throw std::runtime_error("not supporting data types beside float\n");
    auto& mgr = C->share.vertex_attribs;
    auto& payload_map = C->payload.renderMap;
    int ID = payload_map[GL_BIND_VAO];
    if (ID<0){
        printf("You haven't bind the VAO yet, pay attention!\n");
        return;
    }
    glObject * ptr;
    int ret = mgr.searchStorage(&ptr, ID);
    if (ret == GL_FAILURE){
        printf("There is no such entry in the glManager for vertex attribs, pay attention!\n");
        return;
    }
    if (index >= ptr->getSize()){
        printf("Cannot hold that many vertex attribs!\n");
        return;
    }
    assert(ptr->bind == GL_BIND_VAO);
    vertex_attrib_t* data = (vertex_attrib_t*)ptr->getDataPtr();
    vertex_attrib_t new_entry = {index, size, dtype, normalized, false, stride, pointer};
    data[index] = new_entry;
}

void glTexParameteri(GLenum target,unsigned int pname,int param){
    
}

// Enable
void glEnableVertexAttribArray(unsigned int idx){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    glObject* ptr;
    int ret;
    auto& mgr = C->share.vertex_attribs;
    int ID = C->payload.renderMap[GL_BIND_VAO];
    if (idx<0 || ID<0){
        return;
    }
    ret = mgr.searchStorage(&ptr, ID);
    if (ret == GL_FAILURE || (unsigned)ptr->getSize()<=idx || ptr->getDataPtr()==nullptr)
        return;
    // mark the activated vertex attribs
    vertex_attrib_t* data = (vertex_attrib_t*)ptr->getDataPtr();
    data[idx].activated = true;
}

void glEnable(GLenum cap){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    switch(cap){
        case GL_DEPTH_TEST:
            C->use_z_test = true;
            C->zbuf = &C->zbuf_1;
            break;
        case GL_CULL_FACE:
            C->cull_face = true;
        default:
            break;
    }
}

static void check_set_layouts(){
    GET_CURRENT_CONTEXT(C);
    vertex_attrib_t* vattrib_data = (vertex_attrib_t*)C->pipeline.vao_ptr->getDataPtr();
    // printf("vao_ptr.size = %d\n",ppl->vao_ptr->getSize());
    // check if the config is activated
    for (int i = 0; i < C->shader.layout_cnt; ++i) {
        if (C->shader.layouts[i] != LAYOUT_INVALID
            && C->shader.layouts[i] >= C->pipeline.vao_ptr->getSize()) {
            C->shader.layouts[i] = LAYOUT_INVALID;
        } else if (C->shader.layouts[i] != LAYOUT_INVALID 
            && !vattrib_data[C->shader.layouts[i]].activated) {
            C->shader.layouts[i] = LAYOUT_INVALID;
        }
    }
}

// draw
void glDrawArrays(GLenum mode, int first, int count){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    glManager& bufs = C->share.buffers;
    glManager& vaos = C->share.vertex_attribs;
    glManager& texs = C->share.textures;
    glObject* vao_ptr;
    glObject* vbo_ptr;
    glObject* tex_ptr;
    int ret, vao_id, vbo_id;
    // sanity checks for VAO
    vao_id = C->payload.renderMap[GL_BIND_VAO];
    if (vao_id < 0)
        return;
    ret = vaos.searchStorage(&vao_ptr, vao_id);
    if (ret ==GL_FAILURE || 
        vao_ptr->bind != GL_BIND_VAO)
        return;
    // valid vao_id, check the vertex config data inside
    vertex_attrib_t* va_data = (vertex_attrib_t*) vao_ptr->getDataPtr();
    if (va_data == nullptr || vao_ptr->getSize()<=0)
        return;
    // sanity check for GL_ARRAY_BUFFER
    vbo_id = C->payload.renderMap[GL_ARRAY_BUFFER];
    if (vbo_id<0)
        return;
    ret = bufs.searchStorage(&vbo_ptr, vbo_id);
    if (ret == GL_FAILURE ||
        vbo_ptr->bind != GL_ARRAY_BUFFER)
        return;
    char* vb_data = (char*) vbo_ptr->getDataPtr();
    if (vb_data == nullptr || vbo_ptr->getSize()<=0)
        return;

    // sanity check for texture resources
    // check active textures, tell pipeline what are the useful textures
    int cnt = 0;
    for (auto& it:C->payload.tex_units){
        if(it.second > 0){
            // check if the texture ID is valid
            ret = texs.searchStorage(&tex_ptr, it.second);
            if (ret == GL_FAILURE || tex_ptr->getSize() <= 0 || tex_ptr->getDataPtr()==nullptr)
                return;
            C->pipeline.textures[cnt] = tex_ptr;
            C->shader.set_diffuse_texture((GLenum)((int)GL_TEXTURE0+cnt));
        }
        else{
            C->pipeline.textures[cnt] = nullptr;
        }
        cnt++;
    }
    // prepare pipeline environment
    C->pipeline.vao_ptr = vao_ptr;
    C->pipeline.vbo_ptr = vbo_ptr;
    C->pipeline.first_vertex = first;
    C->pipeline.vertex_num = count;
    C->pipeline.use_indices = false;

    check_set_layouts();

    // draw
    // std::cout<<"drawing one frame"<<std::endl;
    auto& exec_list = C->pipeline.exec; 
    auto iter = exec_list.begin();
    switch(mode){
        case GL_TRIANGLES:
            while (iter != exec_list.end()){
                (*iter)();
                ++iter;
            }
            break;
        default:
            break;
    }
}

void glDrawElements(GLenum mode, int count, unsigned int type, const void* indices)
{
    GET_CURRENT_CONTEXT(ctx);
    if (ctx == nullptr){
        return;
    }
    glManager& bufs = ctx->share.buffers;
    glManager& vaos = ctx->share.vertex_attribs;
    glManager& texs = ctx->share.textures;
    glObject* vao_ptr;
    glObject* vbo_ptr;
    glObject* ebo_ptr;
    glObject* tex_ptr;
    int ret, vao_id, vbo_id, ebo_id;
    // sanity checks for VAO
    vao_id = ctx->payload.renderMap[GL_BIND_VAO];
    vbo_id = ctx->payload.renderMap[GL_ARRAY_BUFFER];
    ebo_id = ctx->payload.renderMap[GL_ELEMENT_ARRAY_BUFFER];
    if (vao_id < 0 || vbo_id < 0 || ebo_id < 0) {
        return;
    }
    ret = vaos.searchStorage(&vao_ptr, vao_id);
    if (ret == GL_FAILURE
        || vao_ptr->bind != GL_BIND_VAO
        || vao_ptr->getDataPtr() == nullptr
        || vao_ptr->getSize() <= 0) {
        return;
    }
    ret = bufs.searchStorage(&vbo_ptr, vbo_id);
    if (ret == GL_FAILURE
        || vbo_ptr->bind != GL_ARRAY_BUFFER
        || vbo_ptr->getDataPtr() == nullptr
        || vbo_ptr->getSize() <= 0) {
        return;
    }
    ret = bufs.searchStorage(&ebo_ptr, ebo_id);
    if (ret == GL_FAILURE
        || ebo_ptr->bind != GL_ELEMENT_ARRAY_BUFFER
        || ebo_ptr->getDataPtr() == nullptr
        || ebo_ptr->getSize() <= 0) {
        return;
    }
    // sanity check for texture resources
    // check active textures, tell pipeline what are the useful textures
    int cnt = 0;
    for (auto& it : ctx->payload.tex_units) {
        if (it.second > 0) {
            // check if the texture ID is valid
            ret = texs.searchStorage(&tex_ptr, it.second);
            if (ret == GL_FAILURE || tex_ptr->getSize() <= 0 || tex_ptr->getDataPtr() == nullptr)
                return;
            ctx->pipeline.textures[cnt] = tex_ptr;
            ctx->shader.set_diffuse_texture((GLenum)((int)GL_TEXTURE0 + cnt));
        } else {
            ctx->pipeline.textures[cnt] = nullptr;
        }
        ++cnt; 
    }
    // prepare pipeline environment
    ctx->pipeline.vao_ptr = vao_ptr;
    ctx->pipeline.vbo_ptr = vbo_ptr;
    ctx->pipeline.vertex_num = count;
    ctx->pipeline.ebo_config.ebo_ptr = ebo_ptr;
    ctx->pipeline.ebo_config.first_indices = indices;
    ctx->pipeline.ebo_config.indices_type = type;
    ctx->pipeline.use_indices = true;

    check_set_layouts();

    // draw
    std::list<render_fp>& exec_list = ctx->pipeline.exec;
    auto iter = exec_list.begin();
    switch (mode) {
        case GL_TRIANGLES:
            while (iter != exec_list.end()) {
                (*iter)();
                ++iter;
            }
            break;
        default:
            break;
    }
}

void glClearColor(float R, float G, float B, float A){
    // baseline version note:
    // first ignore the alpha channel
    // and don't wait for glClear() to clear the buffer
    // for simplicity, just clear the framebuffer by the RGB here
    color_t color = {R*255.0f, G*255.0f, B*255.0f};
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    // std::cout<<"current context is: "<<C<<std::endl;
    C->clear_color = color;
}

void glClear(unsigned int bitfields){
    GET_CURRENT_CONTEXT(C);
    // clear depth
    if((bitfields & GL_DEPTH_BUFFER_BIT) == GL_DEPTH_BUFFER_BIT){
        float* zbuf = (float *)C->zbuf->getDataPtr();
        std::fill(zbuf ,zbuf + C->zbuf->getSize(), 1.0f);
    }
    if ((bitfields & GL_COLOR_BUFFER_BIT) == GL_COLOR_BUFFER_BIT){
        if (C->framebuf == nullptr)
            return;
        color_t * data = (color_t*) (C->framebuf->getDataPtr());
        // assert(C->framebuf == &(C->framebuf_1));
        int size = C->framebuf->getSize();
        // std::cout<<"data array: "<<data<<std::endl;
        // std::cout<<"size: "<<size<<std::endl;
        for (int i=0; i<size; i++){
            data[i] = C->clear_color;
        }
    }
}

// IO
void glReadPixels(int x, int y, int width, int height, GLenum format, GLenum type, void* data){

}

// shader API
unsigned int glCreateShader(GLenum shaderType){
    return 0;
}


void glShaderSource(unsigned int shader, int count, char** string, int* length){

}

void glCompileShader(unsigned int shader){

}

unsigned int glCreateProgram(){
    return 0;
}


void glAttachShader(unsigned int shaderProgram, unsigned int shader){

}

void glLinkProgram(unsigned int shaderProgram){

}

void glUseProgram(unsigned int shaderProgram){

}

