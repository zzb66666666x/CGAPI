#include <iostream>
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
    auto& bufs = C->share.buffers;
    int ret;
    for (int i=0; i<num; i++){
        ret = bufs.insertPlaceHolder(GLOBJ_VERTEX_BUFFER);
        ID[i] = ret;
    }
}

void glGenVertexArrays(int num, int* ID){
    GET_CURRENT_CONTEXT(C);
    auto& attribs = C->share.vertex_attribs;
    int ret;
    // use glStorage<vertex_attrib_t> 
    // each VAO owns on set of properties, stored in one glStorage
    // each property are entries to the data array of the glStorage
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

}

void glBindVertexArray(int ID){

}

void glBindTexture(GLenum target,  int ID){

}

// Pass data
void glBufferData(GLenum buf_type, int nbytes, const void* data, GLenum usage){

}

void glTexImage2D(GLenum target, int level,  GLenum internalFormat, int width, int height, int border, GLenum format, GLenum type, void * data){

}

// Attrib
void glVertexAttribPointer(int index, int size, GLenum dtype, bool normalized, int stride, void * pointer){

}

// Enable
void glEnableVertexAttribArray(int ID){

}

void glEnable(GLenum cap){

}

// draw
void glDrawArrays(GLenum mode, int first, int count){

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

