#ifndef _INNER_SUPPORT_H
#define _INNER_SUPPORT_H

#ifdef GLSL_CODE
enum filter_type{
    NEAREST,
    BILINEAR
};
#else
#include "texture.h"
#include "vec_math.h"
#include "../formats.h"
#endif

typedef struct{
    unsigned char* tex_data;
    int width;
    int height;
    filter_type filter;
}sampler_data_pack;

// the call back to enable glsl code to fetch data from graphics pipeline
typedef sampler_data_pack (*get_sampler2D_data_fptr)(int texunit_id);

class ShaderInterface{
    public:
    virtual ~ShaderInterface(){}
    virtual void glsl_main() = 0;
    virtual void input_port(std::map<std::string, data_t>& indata) = 0; 
    virtual void output_port(std::map<std::string, data_t>& outdata) = 0; 
    virtual void input_uniform_dispatch(int idx, data_t& data) = 0; 
    virtual data_t output_uniform_dispatch(int idx) = 0; 
    virtual void set_inner_variable(int variable, data_t& data) = 0;
    virtual void get_inner_variable(int variable, data_t& data) = 0;
    virtual void set_sampler2D_callback(get_sampler2D_data_fptr func) = 0;
};

#ifdef GLSL_CODE

class GLSLShader;

// fp to member function
typedef void (GLSLShader::*set_uniform)(data_t& var);
typedef data_t (GLSLShader::*get_uniform)(void);

extern get_sampler2D_data_fptr get_sampler2D;

class sampler2D{
    public:
    sampler2D(){}
    int texunit_id;
    int width;
    int height;
    unsigned char *data;
    filter_type filter;
    sampler2D& operator=(int val){
        texunit_id = val;
        sampler_data_pack tmp = get_sampler2D(val);
        width = tmp.width;
        height = tmp.height;
        data = tmp.tex_data;
        filter = tmp.filter;
        return *this;
    }
};




#endif

#endif