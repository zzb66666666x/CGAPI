#include "configs.h"
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <float.h>
#include "../../include/gl/gl.h"
#include "../../include/gl/common.h"
#include "glcontext.h"
#include "formats.h"
#include "glsl/texture.h"
#include "glsl/parse.h"
#include "glsl/vec_math.h"
#include <glm/gtc/type_ptr.hpp>
#include "glsl/os.h"

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
    // auto& attribs = C->share.vertex_attribs;
    auto& vaos = C->share.vertex_array_objects;
    int ret;
    // use glStorage<vertex_attrib_t> 
    // different VAO ids are maintained by glManager
    // each VAO owns on set of properties, stored in one glStorage<vertex_attrib_t>
    // each property are entries to the data array of the glStorage<vertex_attrib_t>
    // within each property, the config are wrapped in struct vertex_attrib_t
    glObject* ptr;
    for (int i=0; i<num; i++){
        // ret = attribs.insertStorage(GL_VERTEX_ATTRIB_CONFIG, GL_MAX_VERTEX_ATTRIB_NUM,  GL_BIND_VAO);
        ret = vaos.insertStorage(GL_VERTEX_ARRAY_OBJECT, 1, GL_BIND_VAO);
        vaos.searchStorage(&ptr, ret);
        ((vertex_array_object_t*)ptr->getDataPtr())->vertex_buffer_object_id = -1;
        ((vertex_array_object_t*)ptr->getDataPtr())->element_buffer_object_id = -1;
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

void glGenFramebuffers(int num, unsigned int* ID){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto& framebufs = C->share.framebuf_attachments;
    int ret;
    for (int i=0; i<num; i++){
        ret = framebufs.insertStorage(GL_FRAMEBUFFER_ATTACHMENT_CONFIG, 1, GL_FRAMEBUFFER);
        ID[i] = ret;
    }
}

// Bind & Activate
void glBindBuffer(GLenum buf_type, unsigned int ID){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto& vaos = C->share.vertex_array_objects;
    int vaoId = C->payload.renderMap[GL_BIND_VAO];
    if(vaoId == -1){
        return;
    }
    auto& bufs = C->share.buffers;
    int ret;
    glObject* ptr;
    glObject* vaoPtr;
    ret = vaos.searchStorage(&vaoPtr, vaoId);
    if(ret == GL_FAILURE){
        return;
    }
    vertex_array_object_t *vao_data = (vertex_array_object_t *)vaoPtr->getDataPtr();
    switch(buf_type){
        case GL_ARRAY_BUFFER:
            if (ID == 0){
                C->payload.renderMap[GL_ARRAY_BUFFER] = 0;
                return;
            }
            // try search out the ID
            ret = bufs.searchStorage(&ptr, ID);
            if (ret == GL_FAILURE){
                return;
            }
            vao_data->vertex_buffer_object_id = ID;
            ptr->bind = GL_ARRAY_BUFFER;
            C->payload.renderMap[GL_ARRAY_BUFFER] = ID;
            break;
        case GL_ELEMENT_ARRAY_BUFFER:
            if (ID == 0){
                C->payload.renderMap[GL_ARRAY_BUFFER] = 0;
                return;                
            }
            ret = bufs.searchStorage(&ptr, ID);
            if (ret == GL_FAILURE) {
                return;
            }
            vao_data->element_buffer_object_id = ID;
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
    // auto& mgr = C->share.vertex_attribs;
    auto& vaoMgr = C->share.vertex_array_objects;
    auto& bufMgr = C->share.buffers;
    if (ID == 0){
        //unbind
        // C->payload.renderMap[GL_BIND_VAO] = -1;

        C->payload.renderMap[GL_BIND_VAO] = -1;
        C->payload.renderMap[GL_ARRAY_BUFFER] = -1;
        C->payload.renderMap[GL_ELEMENT_ARRAY_BUFFER] = -1;
    }
    else{
        // verify that the ID is valid
        // glObject* ptr;
        // int ret;
        // ret = mgr.searchStorage(&ptr, ID);
        // if (ret == GL_FAILURE){
        //     return;
        // }
        // C->payload.renderMap[GL_BIND_VAO] = ID;

        glObject* vaoPtr, *vboPtr, *eboPtr;
        int vboId, eboId, ret;
        if (vaoMgr.searchStorage(&vaoPtr, ID) == GL_FAILURE) {
            return;
        }

        bufMgr.searchStorage(&vboPtr, vboId = ((vertex_array_object_t*)vaoPtr->getDataPtr())->vertex_buffer_object_id);
        bufMgr.searchStorage(&eboPtr, eboId = ((vertex_array_object_t*)vaoPtr->getDataPtr())->element_buffer_object_id);

        C->payload.renderMap[GL_BIND_VAO] = ID;
        C->payload.renderMap[GL_ARRAY_BUFFER] = vboId;
        C->payload.renderMap[GL_ELEMENT_ARRAY_BUFFER] = eboId;
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
            if (ID == 0){
                C->payload.renderMap[GL_TEXTURE_2D] = -1;
            }
            else{
                ret = texs.searchStorage(&ptr, ID);
                ptr->bind = GL_TEXTURE_2D;
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

void glBindFramebuffer(GLenum target, unsigned int ID){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto& framebufs = C->share.framebuf_attachments;
    auto& render_map = C->payload.renderMap;
    int ret;
    glObject* ptr;  
    switch(target){
        case GL_FRAMEBUFFER:    
            if (ID==0){
                // restore to default framebuf
                C->override_default_framebuf = false;
                C->override_color_buf = nullptr;
                C->override_depth_buf = nullptr;
                C->override_buf_npixels = 0;
                render_map[GL_FRAMEBUFFER] = 0;
                C->draw_color_buf = true;
                C->flip_image = true;
                C->cur_sync_unit = &(C->sync_unit);
            }else{
                // make sure that the ID exists
                ret = framebufs.searchStorage(&ptr, ID);
                if (ret == GL_FAILURE || ptr->bind != GL_FRAMEBUFFER)
                    return;
                render_map[GL_FRAMEBUFFER] = ID;
                C->override_default_framebuf = true;
                framebuf_attachment_t* config = (framebuf_attachment_t*)ptr->getDataPtr();
                C->override_color_buf = config->color_buf;
                C->override_depth_buf = config->depth_buf;
                C->override_buf_npixels = config->buffer_npixels;
                C->draw_color_buf = config->draw_color_buf;
                C->cur_sync_unit = &(config->sync_unit);
                C->flip_image = config->flip_image;
            }
            break;
        default:
            break;
    }
}

void glActiveTexture(GLenum unit){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto iter = C->payload.tex_units.find(unit);
    if(iter->second != TEXTURE_UNIT_TBD){
        iter->second = TEXTURE_UNIT_TBD;
    }
}

void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, unsigned int texture, int level){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    if (textarget != GL_TEXTURE_2D)
        throw std::runtime_error("not supported texture form\n");
    if (level != 0)
        throw std::runtime_error("mip map is not supported for now");
    if (target != GL_FRAMEBUFFER)
        throw std::runtime_error("please don't bind texture to target except GL_FRAMEBUFFER\n");
    int framebuf_obj_id = C->payload.renderMap[GL_FRAMEBUFFER];
    if (framebuf_obj_id <= 0){
        printf("you haven't binded any framebuffer other than the default one\n");
        return;
    }
    bool detach_texture = false;
    glObject* tex_ptr = nullptr;
    sampler_config* tex_conf = nullptr;
    if (texture == 0){
        detach_texture = true;
    }else{
        // search the texture
        int ret = C->share.textures.searchStorage(&tex_ptr, texture);
        if (ret == GL_FAILURE){
            printf("invalid texture passed in to render frame to texture\n");
            return;
        }
        tex_conf = &(C->share.tex_config_map[texture]);
    }
    glObject* framebuf_config_ptr;
    C->share.framebuf_attachments.searchStorage(&framebuf_config_ptr, framebuf_obj_id);
    switch(attachment){
        case GL_DEPTH_ATTACHMENT:{
            // check the format of texture
            if (tex_conf->color_format != FORMAT_COLOR_32FC1){
                throw std::runtime_error("using invalid texture for depth attachment\n");
            }
            framebuf_attachment_t* attachment_configs = (framebuf_attachment_t*)framebuf_config_ptr->getDataPtr();
            if (detach_texture){
                attachment_configs->attached_obj_for_depth = -1;
                attachment_configs->depth_buf = nullptr;
                C->override_depth_buf = nullptr;
            }else{
                int npixels = tex_conf->height * tex_conf->width;
                attachment_configs->attachment_type = textarget;
                attachment_configs->attached_obj_for_depth = texture;
                attachment_configs->depth_buf = (float*)tex_ptr->getDataPtr();
                attachment_configs->buffer_npixels = npixels;
                attachment_configs->init_sync_unit(npixels);
                attachment_configs->flip_image = false;
                if (C->override_default_framebuf){
                    C->override_depth_buf = attachment_configs->depth_buf;
                    C->override_buf_npixels = npixels;
                    C->cur_sync_unit = &(attachment_configs->sync_unit);
                    C->flip_image  = false;
                }
            }
        }break;
        default:
            throw std::runtime_error("not supported attachment to framebuffer\n");
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
        level !=0)
        throw std::runtime_error("using invalid params about level and border when passing tex data\n");
    // if (internalFormat != GL_RGB && internalFormat != GL_RGBA)
    //     throw std::runtime_error("the internal format has to be RGB or RGBA\n");
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
            else if (f_desc.out_channels == 4){
                C->share.tex_config_map.emplace(ID, sampler_config(width, height, (int)FORMAT_COLOR_8UC4));
            }else{
                throw std::runtime_error("not supporting color format whose num_channel is not 3 or 4\n");
            }
            if (f_desc.direct_pass){
                nbytes = width * height * f_desc.out_channels;
                ptr->allocByteSpace(nbytes);
                if (tex_data != nullptr)
                    memcpy(ptr->getDataPtr(), tex_data, nbytes);
            }
            else{
                throw std::runtime_error("not supporting format auto conversion yet\n");
            }
        }break;
        case GL_FLOAT:{
            float* tex_data = (float*) data;
            int nbytes;
            _tex_data_formatter(internalFormat, format, &f_desc);
            if (f_desc.in_channels == 1 && f_desc.out_channels == 1 && f_desc.direct_pass){
                C->share.tex_config_map.emplace(ID, sampler_config(width, height, (int)FORMAT_COLOR_32FC1));
                nbytes = width * height * sizeof(float);
                ptr->allocByteSpace(nbytes);
                if (tex_data != nullptr)
                    memcpy(ptr->getDataPtr(), tex_data, nbytes);
            }else{
                throw std::runtime_error("for GL_FLOAT texture data, we assume it to be float depth for simplicity\n");
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
                    desc->direct_pass = false;
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
        case GL_DEPTH_COMPONENT:
            desc->in_channels = 1;
            if (internalFormat != GL_DEPTH_COMPONENT)
                throw std::runtime_error("invalid internal format\n");
            desc->direct_pass = true;
            desc->out_channels = 1;
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
    // auto& mgr = C->share.vertex_attribs;
    auto& mgr = C->share.vertex_array_objects;
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
    if (index >= GL_MAX_VERTEX_ATTRIB_NUM) {
        printf("Cannot hold that many vertex attribs!\n");
        return;
    }
    assert(ptr->bind == GL_BIND_VAO);
    // vertex_attrib_t* data = (vertex_attrib_t*)ptr->getDataPtr();
    vertex_array_object_t* vao_data = (vertex_array_object_t*)ptr->getDataPtr();
    vertex_attrib_t* attribs = vao_data->attribs;
    vertex_attrib_t new_entry = {index, size, dtype, normalized, false, stride, pointer};
    attribs[index] = new_entry;
}

void glTexParameteri(GLenum target, GLenum pname, int param){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    if (target != GL_TEXTURE_2D){
        throw std::runtime_error("not supporting setting params for texture other than texture_2D");
    }
    int tex_id = C->payload.renderMap[GL_TEXTURE_2D];
    if (tex_id <=0 ){
        throw std::runtime_error("haven't binded any texture\n");
    }
    auto tex_config = C->share.tex_config_map.find(tex_id);
    if (tex_config == C->share.tex_config_map.end())
        throw std::runtime_error("invalid texture binded\n");
    if (param != GL_REPEAT && param != GL_CLAMP_TO_BORDER){
        throw std::runtime_error("not supported param value for setting the texture parameter\n");
    }
    switch(pname){
        case GL_TEXTURE_WRAP_S:
            tex_config->second.wrap_s = param;
            break;
        case GL_TEXTURE_WRAP_T:
            tex_config->second.wrap_t = param;
            break;
        default:
            throw std::runtime_error("not supporting this parameter settings\n");
    }
}

void glTexParameterfv(GLenum target,GLenum pname,const float * params){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    if (target != GL_TEXTURE_2D){
        throw std::runtime_error("not supporting setting params for texture other than texture_2D");
    }
    int tex_id = C->payload.renderMap[GL_TEXTURE_2D];
    if (tex_id <=0 ){
        throw std::runtime_error("haven't binded any texture\n");
    }
    auto tex_config = C->share.tex_config_map.find(tex_id);
    if (tex_config == C->share.tex_config_map.end())
        throw std::runtime_error("invalid texture binded\n");
    switch(pname){
        case GL_TEXTURE_BORDER_COLOR:
            tex_config->second.border_val = glm::make_vec4(params);
            break;
        default:
            throw std::runtime_error("not supporting this parameter settings\n");
    }
}

// Enable
void glEnableVertexAttribArray(unsigned int idx){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    glObject* ptr;
    int ret;
    
    // auto& mgr = C->share.vertex_attribs;
    auto& vaoMgr = C->share.vertex_array_objects;

    int ID = C->payload.renderMap[GL_BIND_VAO];
    if (idx < 0 || ID < 0 || idx >= GL_MAX_VERTEX_ATTRIB_NUM) {
        return;
    }
    // ret = mgr.searchStorage(&ptr, ID);
    // if (ret == GL_FAILURE || (unsigned)ptr->getSize()<=idx || ptr->getDataPtr()==nullptr)
    //     return;

    ret = vaoMgr.searchStorage(&ptr, ID);
    if(ret == GL_FAILURE || ptr->getDataPtr() == nullptr){
        return;
    }

    // mark the activated vertex attribs
    // vertex_attrib_t* data = (vertex_attrib_t*)ptr->getDataPtr();
    // data[idx].activated = true;

    vertex_attrib_t* data = ((vertex_array_object_t*)ptr->getDataPtr())->attribs;
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
            C->cull_face.open = true;
            C->cull_face.cull_face_mode = GL_BACK;
            C->cull_face.front_face_mode = GL_CCW;
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

void glViewport(int x, int y, int width, int height){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    C->width = width;
    C->height = height;
    C->start_pos_x = x;
    C->start_pos_y = y;
}

void glDrawBuffer(GLenum mode){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    // find the current framebuffer
    switch(mode){
        case GL_NONE:{
            C->draw_color_buf = false;
            if (C->override_default_framebuf && C->payload.renderMap[GL_FRAMEBUFFER] > 0){
                glObject * ptr;
                C->share.framebuf_attachments.searchStorage(&ptr, C->payload.renderMap[GL_FRAMEBUFFER]);
                framebuf_attachment_t* config = (framebuf_attachment_t*)ptr->getDataPtr();
                config->draw_color_buf = false;
            }
        }   break;
        default:
            break;
    }
}

void glReadBuffer(GLenum mode){

}

// draw
void glDrawArrays(GLenum mode, int first, int count){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    glManager& bufs = C->share.buffers;
    // glManager& vaos = C->share.vertex_attribs;
    glManager& vaos = C->share.vertex_array_objects;
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

    // sanity check for texture and layout (the method depends on whether we use programmable shaders)
#if (ENABLE_PROGRAMMABLE_PIPELINE == 0)
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
    check_set_layouts();
#endif
    // prepare pipeline environment
    C->pipeline.vao_ptr = vao_ptr;
    C->pipeline.vbo_ptr = vbo_ptr;
    C->pipeline.first_vertex = first;
    C->pipeline.vertex_num = count;
    C->pipeline.use_indices = false;

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
    if (ctx == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    glManager& bufs = ctx->share.buffers;
    // glManager& vaos = ctx->share.vertex_attribs;
    glManager& vaos = ctx->share.vertex_array_objects;
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
    // sanity check for texture and layout (the method depends on whether we use programmable shaders)
#if (ENABLE_PROGRAMMABLE_PIPELINE == 0)
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
    check_set_layouts();
#endif
    // prepare pipeline environment
    ctx->pipeline.vao_ptr = vao_ptr;
    ctx->pipeline.vbo_ptr = vbo_ptr;
    ctx->pipeline.vertex_num = count;
    ctx->pipeline.ebo_config.ebo_ptr = ebo_ptr;
    ctx->pipeline.ebo_config.first_indices = indices;
    ctx->pipeline.ebo_config.indices_type = type;
    ctx->pipeline.use_indices = true;

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
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    // clear depth
    if((bitfields & GL_DEPTH_BUFFER_BIT) == GL_DEPTH_BUFFER_BIT){
        float* zbuf;
        int size;
        if (C->override_default_framebuf){
            zbuf = (float *)C->override_depth_buf;
            size = C->override_buf_npixels;
        }else{
            zbuf = (float *)C->zbuf->getDataPtr();
            size = C->zbuf->getSize();
        }
        if (zbuf != nullptr){
            #pragma omp parallel for
            for (int i=0; i<size; i++){
                zbuf[i] = 1.0f;
            }
        }
    }
    if ((bitfields & GL_COLOR_BUFFER_BIT) == GL_COLOR_BUFFER_BIT){
        if (C->framebuf == nullptr)
            return;
        color_t* data;
        int size;
        if (C->override_default_framebuf){
            data = C->override_color_buf;
            size = C->override_buf_npixels; 
        }else{
            data = (color_t*) (C->framebuf->getDataPtr());
            size = C->framebuf->getSize();
        }
        if (data != nullptr){
            #pragma omp parallel for
            for (int i=0; i<size; i++){
                data[i] = C->clear_color;
            }
        }
    }
}

//////////////////////////// cull face /////////////////////////////////
extern void glCullFace(unsigned int mode){
    GET_CURRENT_CONTEXT(ctx);
    if(ctx == nullptr){
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    if (mode != GL_FRONT && mode != GL_BACK && mode != GL_FRONT_AND_BACK){
        return;
    }
    ctx->cull_face.cull_face_mode = mode;
    // TODO specify func
}

extern void glFrontFace(unsigned int mode){
    GET_CURRENT_CONTEXT(ctx);
    if (ctx == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    if(mode != GL_CCW && mode != GL_CW){
        return;
    }
    ctx->cull_face.front_face_mode = mode;
    // TODO specify func
}

/////////////////////////// IO ///////////////////////////
void glReadPixels(int x, int y, int width, int height, GLenum format, GLenum type, void* data){

}

/////////////////////////// shader API ///////////////////////////
unsigned int glCreateShader(GLenum shaderType){
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    return C->glsl_shaders.create_shader(shaderType, C->pipeline.cpu_num);
}


void glShaderSource(unsigned int shader, int count, const char* const* string, const int* length){
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    auto it = C->glsl_shaders.shader_cache_map.find(shader);
    if (it == C->glsl_shaders.shader_cache_map.end())
        return;
    Shader* shader_ptr = it->second;
    buffer_t code_buf;
    init_buffer(&code_buf, 1000);
    for (int i=0; i<count; i++){
        const char* str = string[i];
        int str_len = strlen(str);
        if (length != NULL){
            str_len = (str_len < length[i])? str_len : length[i]; 
        }
        char* space = new char[str_len+1];
        memcpy(space, str, str_len);
        space[str_len] = '\0';
        register_code(&code_buf, space);
        delete[] space;
    }
    shader_ptr->set_glsl_code(std::string(code_buf.data));
    free_buffer(&code_buf);
}

sampler_data_pack get_sampler2D_data_fdef(int texunit_id){
    // this function will be called when we set the uniform texture
    // which is outside of the render loop
    GET_CURRENT_CONTEXT(C);
    sampler_data_pack pack;
    if (texunit_id >= GL_MAX_TEXTURE_UNITS || texunit_id < 0)
        throw std::runtime_error("terminated because you use invlaid texture unit id\n");
    int tex_obj_id = C->payload.tex_units[(GLenum)(GL_TEXTURE0+texunit_id)];
    glObject* obj_ptr;
    int ret = C->share.textures.searchStorage(&obj_ptr, tex_obj_id);
    auto it = C->share.tex_config_map.find(tex_obj_id);
    if (it == C->share.tex_config_map.end() || ret==GL_FAILURE)
        throw std::runtime_error("terminated because the texture cannot be found\n");
    pack.tex_data = obj_ptr->getDataPtr();
    pack.color_format = it->second.color_format;
    pack.filter = NEAREST;
    // pack.filter = filter_type::BILINEAR;
    pack.height = it->second.height;
    pack.width = it->second.width;
    pack.wrap_s = it->second.wrap_s;
    pack.wrap_t = it->second.wrap_t;
    pack.border_val = it->second.border_val;
    return pack;
}

void glCompileShader(unsigned int shader){
    // compile a shader in the shader cache
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    auto it = C->glsl_shaders.shader_cache_map.find(shader);
    if (it == C->glsl_shaders.shader_cache_map.end())
        return;
    Shader* shader_ptr = it->second;
    std::string output_fname;
    switch(shader_ptr->shader_type){
        case GL_VERTEX_SHADER:
            output_fname = "vshader";
            break;
        case GL_FRAGMENT_SHADER:
            output_fname = "fshader";
            break;
        default:
            throw std::runtime_error("not supported shader\n");
    }
    #ifdef OS_WIN
    std::string libfiletype(".dll");
    #endif
    #ifdef OS_LINUX
    std::string libfiletype(".so");
    #endif
    output_fname += (std::to_string(shader) + libfiletype);
    shader_ptr->compile(output_fname);
    shader_ptr->load_shader();
    shader_ptr->set_sampler2D_callback(get_sampler2D_data_fdef);
}

unsigned int glCreateProgram(){
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    return (unsigned)C->glsl_shaders.create_program();
}


void glAttachShader(unsigned int shaderProgram, unsigned int shader){
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    C->glsl_shaders.attach(shaderProgram, shader);
}

void glLinkProgram(unsigned int shaderProgram){
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    auto it = C->glsl_shaders.shader_program_map.find(shaderProgram);
    if (it != C->glsl_shaders.shader_program_map.end()){
        glProgrammableShader& program = it->second;
        program.link_programs();
    }
}

void glUseProgram(unsigned int shaderProgram){
    // first check if that program exists
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    auto it = C->glsl_shaders.shader_program_map.find(shaderProgram);
    if(it == C->glsl_shaders.shader_program_map.end())
        return;
    C->payload.shader_program_in_use = shaderProgram;    
    C->payload.cur_shader_program_ptr = &(it->second);
}

void glUniform1i(int location, int val){
    if (location < 0)
        return;
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    // auto it_prog = C->glsl_shaders.shader_program_map.find(C->payload.shader_program_in_use);
    // if(it_prog == C->glsl_shaders.shader_program_map.end())
    //     throw std::runtime_error("using invalid shader program\n");
    // glProgrammableShader& program = it_prog->second;
    glProgrammableShader& program = *(C->payload.cur_shader_program_ptr);
    auto it_id = program.uniform_id_to_name.find(location);
    if (it_id == program.uniform_id_to_name.end())
        return;
    uniform_varaible_t& uniform_var_info = program.merged_uniform_maps[it_id->second];
    data_t& data = uniform_var_info.data;
    // the sampler2D_var and int_var shares the same space in an anonymous union
    data.sampler2D_var = val;
    for (auto uniform_ftable_pair : uniform_var_info.uniform_ftable_idx){
        GLenum shader_type = uniform_ftable_pair.first;
        int ftable_idx = uniform_ftable_pair.second;
        Shader* shader_ptr = program.shaders[shader_type];
        shader_ptr->set_uniform_variables(ftable_idx, data);
    }
}

void glUniformMatrix4fv(int location, int count, bool transpose, const float * value){
    if (location < 0)
        return;    
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    if (count != 1)
        throw std::runtime_error("the glsl cannot support uniform varaible array now\n");
    // fetch current program id
    // auto it_prog = C->glsl_shaders.shader_program_map.find(C->payload.shader_program_in_use);
    // if(it_prog == C->glsl_shaders.shader_program_map.end())
    //     throw std::runtime_error("using invalid shader program\n");
    // glProgrammableShader& program = it_prog->second;
    glProgrammableShader& program = *(C->payload.cur_shader_program_ptr);
    auto it_id = program.uniform_id_to_name.find(location);
    if (it_id == program.uniform_id_to_name.end())
        return;
    uniform_varaible_t& uniform_var_info = program.merged_uniform_maps[it_id->second];
    data_t& data = uniform_var_info.data;
    glm::mat4 mat = glm::make_mat4(value);
    if (transpose){
        mat = glm::transpose(mat);
    }
    data.mat4_var = mat;
    for (auto uniform_ftable_pair : uniform_var_info.uniform_ftable_idx){
        GLenum shader_type = uniform_ftable_pair.first;
        int ftable_idx = uniform_ftable_pair.second;
        Shader* shader_ptr = program.shaders[shader_type];
        shader_ptr->set_uniform_variables(ftable_idx, data);
    }
}

void glUniform3fv(int location, int count, const float* value){
    if (location < 0)
        return;
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    if (count != 1)
        throw std::runtime_error("the glsl cannot support uniform varaible array now\n");
    // fetch current program id
    // auto it_prog = C->glsl_shaders.shader_program_map.find(C->payload.shader_program_in_use);
    // if (it_prog == C->glsl_shaders.shader_program_map.end())
    //     throw std::runtime_error("using invalid shader program\n");
    // glProgrammableShader& program = it_prog->second;
    glProgrammableShader& program = *(C->payload.cur_shader_program_ptr);
    auto it_id = program.uniform_id_to_name.find(location);
    if (it_id == program.uniform_id_to_name.end())
        return;
    uniform_varaible_t& uniform_var_info = program.merged_uniform_maps[it_id->second];
    data_t& data = uniform_var_info.data;
    glm::vec3 vec = glm::make_vec3(value);
    data.vec3_var = vec;
    for (auto uniform_ftable_pair : uniform_var_info.uniform_ftable_idx) {
        GLenum shader_type = uniform_ftable_pair.first;
        int ftable_idx = uniform_ftable_pair.second;
        Shader* shader_ptr = program.shaders[shader_type];
        shader_ptr->set_uniform_variables(ftable_idx, data);
    }
}

int glGetUniformLocation(unsigned int program, const char* name){
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    auto it = C->glsl_shaders.shader_program_map.find(program);
    if (it != C->glsl_shaders.shader_program_map.end()){
        glProgrammableShader& program = it->second;
        auto it_uniform_var = program.merged_uniform_maps.find(std::string(name));
        if(it_uniform_var != program.merged_uniform_maps.end()){
            return it_uniform_var->second.uniform_id;
        }else{
            return GL_FAILURE;
        }
    }else{
        return GL_FAILURE;
    }
}

void glDebugflag(bool debug_flag){
    GET_CURRENT_CONTEXT(C);
    if (C == nullptr) {
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    }
    C->debug_flag = debug_flag;
}
