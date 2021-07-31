#include "os.h"
#include "translate.h"
#include "parse.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

static string macros = \
"#define GLSL_CODE \n" 
"#include <stdio.h> \n"
"#include <map> \n"
"#include <string> \n"
;

static string prefix = \
"using namespace glm; \n"
;

static string class_shader_code = \
"class GLSLShader: public ShaderInterface{ \n "
"public: \n"
"vec4 gl_Position; \n"
"vec4 gl_FragColor; \n"
"void set_inner_variable(int variable, data_t& data){ \n"
"    switch(variable){ \n"
"        case INNER_GL_POSITION: \n"
"            gl_Position = data.vec4_var; \n"
"            break; \n"
"        case INNER_GL_FRAGCOLOR: \n"
"            gl_FragColor = data.vec4_var; \n"
"            break; \n"
"        default: \n"
"            break; \n"
"    } \n"
"} \n"
"void get_inner_variable(int variable, data_t& data){ \n"
"    switch(variable){ \n"
"        case INNER_GL_POSITION: \n"
"            data.vec4_var = gl_Position; \n"
"            break; \n"
"        case INNER_GL_FRAGCOLOR: \n"
"            data.vec4_var = gl_FragColor; \n"
"            break; \n"
"        default: \n"
"            break; \n"
"    } \n"
"} \n"
"void set_sampler2D_callback(get_sampler2D_data_fptr func){\n "
"    get_sampler2D = func; \n "
"}\n "
;

static string postfix = \
"\n};\n"
"extern \"C\"{ \n "
"#ifdef OS_WIN \n"
"__declspec(dllexport) \n"
"#endif \n"
"ShaderInterface* create_shader_inst(){ \n "
"    ShaderInterface* ret = new GLSLShader; \n "
"    return ret; \n "
"} \n "
"#ifdef OS_WIN \n"
"__declspec(dllexport) \n"
"#endif \n"
"void destroy_shader_inst(ShaderInterface* inst){ \n "
"    delete inst; \n "
"} \n "
"} \n "
;

static string extract_headers(vector<string>& headers){
    buffer_t code;
    init_buffer(&code, 3000);
    for (auto it = headers.begin(); it != headers.end(); it++){
        string& s = *it;
        ifstream tmp_stream;
        tmp_stream.open(s);
        stringstream tmp_str_stream;
        tmp_str_stream << tmp_stream.rdbuf();		
        tmp_stream.close();
        // cout<<tmp_str_stream.str()<<endl;
        register_code(&code, tmp_str_stream.str().c_str());
        register_code(&code, "\n");
    }
    string ret = string(code.data);
    free_buffer(&code);
    return ret;
}

void cpp_code_generate(string& src, string& dest){
    vector<string> headers;
    headers.push_back(string("../src/gl/glsl/os.h"));
    headers.push_back(string("../src/gl/formats.h"));
    headers.push_back(string("../src/gl/glsl/vec_math.h"));
    headers.push_back(string("../src/gl/glsl/inner_variable.h"));
    headers.push_back(string("../src/gl/glsl/inner_support.h"));
    dest = macros + extract_headers(headers) + prefix + class_shader_code + src + postfix;
}

