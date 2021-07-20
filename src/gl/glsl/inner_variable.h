#ifndef _INNER_VARIABLE_H
#define _INNER_VARIABLE_H

#include "vec_math.h"

#define INNER_GL_POSITION   1
#define INNER_GL_FRAGCOLOR  2

#ifdef GLSL_CODE
extern glm::vec4 gl_Position;
extern glm::vec4 gl_FragColor;

__declspec(dllexport) void set_inner_variable(int variable, data_t& data);
__declspec(dllexport) void get_inner_variable(int variable, data_t& data);

void set_inner_variable(int variable, data_t& data){
    switch(variable){
        case INNER_GL_POSITION:
            gl_Position = data.vec4_var;
            break;
        case INNER_GL_FRAGCOLOR:
            gl_FragColor = data.vec4_var;
            break;
        default:
            break;
    }
}

void get_inner_variable(int variable, data_t& data){
    switch(variable){
        case INNER_GL_POSITION:
            data.vec4_var = gl_Position;
            break;
        case INNER_GL_FRAGCOLOR:
            data.vec4_var = gl_FragColor;
            break;
        default:
            break;
    }
}

#endif

#endif