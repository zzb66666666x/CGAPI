#include <iostream>
#include <stdio.h>
#include <assert.h>
#include "../../include/gl/gl.h"
#include "../../include/gl/common.h"
#include "glcontext.h"

// helloworld
void glHelloWorld(){
    std::cout<<"hello and welcome to gl world written by ZJU student!"<<std::endl;
}

// Gen
void glGenBuffers(int num, int * ID){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto& bufs = C->share.buffers;
    int ret;
    for (int i=0; i<num; i++){
        ret = bufs.insertStorage(GL_BYTE, 0, true, GLOBJ_VERTEX_BUFFER, GL_UNDEF);
        ID[i] = ret;
    }
}

void glGenVertexArrays(int num, int* ID){
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
        ret = attribs.insertStorage(GL_VERTEX_ATTRIB_CONFIG, DEFAULT_VERTEX_ATTRIB_NUM, false, GLOBJ_VERTEX_ATTRIB, GL_BIND_VAO);
        ID[i] = ret;    // if failure, then ID will be -1
    }
}

void glGenTexture(int num, int* ID){

}

// Bind
void glBindBuffer(GLenum buf_type,  int ID){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    auto& bufs = C->share.buffers;
    int ret;
    glObject* ptr;
    switch(buf_type){
        case GL_ARRAY_BUFFER:
            if (ID<0){
                return;
            }
            else if (ID==0){
                // unbind the GL_ARRAY_BUFFER
                C->payload.renderMap[GL_ARRAY_BUFFER] = -1;
            }
            else{
                // try search out the ID
                ret = bufs.searchStorage(&ptr, ID);
                if (ret==GL_FAILURE)
                    return;
                ptr->bind = GL_ARRAY_BUFFER;
                if (ptr->type != GLOBJ_VERTEX_BUFFER){
                    printf("The GL OBJ type is surpisingly not GLOBJ_VERTEX_BUFFER");
                    // ptr->type = GLOBJ_VERTEX_BUFFER;
                }
                C->payload.renderMap[GL_ARRAY_BUFFER] = ID;
            }
        case GL_ELEMENT_ARRAY_BUFFER:
            break;
        default:
            break;
    }
}

void glBindVertexArray(int ID){
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

void glBindTexture(GLenum target,  int ID){

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
                    if (ret == GL_FAILURE)
                        return;
                    // copy data inside glStorage<char>
                    ptr->allocByteSpace(nbytes);
                    ptr->loadBytes(data, nbytes);
                    break;
                case GL_ELEMENT_ARRAY_BUFFER:
                    throw std::runtime_error("not written for EBO\n");
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

}

// Attrib
void glVertexAttribPointer(int index, int size, GLenum dtype, bool normalized, int stride, void * pointer){
    // first reach out to render payload, fetch out the id
    // based on the id, modify the glStorage<vertex_attrib_t>
    // if we haven't bind, the setting here has not effect
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
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
    assert(ptr->bind == GL_BIND_VAO && ptr->type == GLOBJ_VERTEX_ATTRIB);
    vertex_attrib_t* data = (vertex_attrib_t*)ptr->getDataPtr();
    vertex_attrib_t new_entry = {index, size, dtype, normalized, stride};
    data[index] = new_entry;
}

// Enable
void glEnableVertexAttribArray(int ID){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    glObject* ptr;
    int ret;
    auto& mgr = C->share.vertex_attribs;
    if (ID<0){
        return;
    }
    ret = mgr.searchStorage(&ptr, ID);
    if (ret == GL_FAILURE)
        return;
    ptr->activated = true;
}

void glEnable(GLenum cap){
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    switch(cap){
        case GL_DEPTH_TEST:
            C->use_z_test = true;
            break;
        default:
            break;
    }
}

// draw
void glDrawArrays(GLenum mode, int first, int count){
    // GET_CURRENT_CONTEXT(C);
    // if (C==nullptr)
    //     throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    // auto& bufs = C->share.buffers;
    // auto& vaos = C->share.vertex_attribs;
    // auto& texs = C->share.textures;
    // glObject* ptr;
    // int ret;
    // int attrib_tot_nbytes = 0;
    // int num_attribs;
    // // check if the VAO is ready
    // int vao_id = C->payload.renderMap[GL_BIND_VAO];
    // if (vao_id == -1)
    //     return; //or do other things?
    // ret = vaos.searchStorage(&ptr, vao_id);
    // if (ret ==GL_FAILURE)
    //     return;
    // // valid vao_id, check the vertex config data inside
    // num_attribs = ptr->getSize(); // in the unit of struct vertex_attribs_t
    // vertex_attrib_t* va_data = (vertex_attrib_t*) ptr->getDataPtr();
    // if (va_data == nullptr)
    //     return;
    // attrib_tot_nbytes = va_data[0].stride;
    // // parse the vertex data and process them by MVP
    // switch(mode){
    //     case GL_TRIANGLE:
            
    //         break;
    //     default:
    //         break;
    // }
}

void glClearColor(float R, float G, float B, float A){
    // baseline version note:
    // first ignore the alpha channel
    // and don't wait for glClear() to clear the buffer
    // for simplicity, just clear the framebuffer by the RGB here
    color_t color = {R*255,G*255,B*255};
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    std::cout<<"current context is: "<<C<<std::endl;
    if (C->framebuf == nullptr)
        return;
    color_t * data = (color_t*) (C->framebuf->getDataPtr());
    // assert(C->framebuf == &(C->framebuf_1));
    int size = C->framebuf->getSize();
    std::cout<<"data array: "<<data<<std::endl;
    std::cout<<"size: "<<size<<std::endl;
    for (int i=0; i<size; i++){
        data[i] = color;
    }
}

void glClear(int bitfields){

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

