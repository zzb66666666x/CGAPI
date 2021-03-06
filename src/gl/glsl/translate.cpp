#include "os.h"
#include "translate.h"
#include "parse.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdio.h>

using namespace std;

#define CONST_HEADER  0

static string macros = \
"#define GLSL_CODE \n" 
"#include <stdio.h> \n"
"#include <map> \n"
"#include <string> \n"
"#include <omp.h> \n"
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
"void set_mipmap_sampler_callback(get_mipmap_sampler_fptr func){\n"
"    get_mipmap_sampler = func; \n"
"}\n"
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

#if(CONST_HEADER)
static string const_header = \
R""(
#ifndef _OS_H
#define _OS_H
#if defined(__linux__) || defined(__linux)
#  define OS_LINUX
#endif
#if !defined(SAG_COM) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY==WINAPI_FAMILY_DESKTOP_APP) && (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#  define OS_WIN32
#  define OS_WIN64
#elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  if defined(WINCE) || defined(_WIN32_WCE)
#    define OS_WINCE
#  elif defined(WINAPI_FAMILY)
#    if defined(WINAPI_FAMILY_PHONE_APP) && WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
#      define OS_WINPHONE
#      define OS_WINRT
#    elif WINAPI_FAMILY==WINAPI_FAMILY_APP
#      define OS_WINRT
#    else
#      define OS_WIN32
#    endif
#  else
#    define OS_WIN32
#  endif
#endif
#if defined(OS_WIN32) || defined(OS_WIN64) || defined(OS_WINCE) || defined(OS_WINRT)
#  define OS_WIN
#endif
#endif
#ifndef _FORMATS_H
#define _FORMATS_H
// F: float
// U: unsigned
// C: channel
// <depth> <F|U> <C> <channel_num>
typedef struct{
float R;
float G;
float B;
}COLOR_32FC3;
typedef struct{
unsigned char R;
unsigned char G;
unsigned char B;
}COLOR_8UC3;
typedef struct{
float R;
float G;
float B;
float A;
}COLOR_32FC4;
typedef struct{
unsigned char R;
unsigned char G;
unsigned char B;
unsigned char A;
}COLOR_8UC4;
typedef struct{
float R;
}COLOR_32FC1;
#define FORMAT_COLOR_32FC3  1
#define FORMAT_COLOR_8UC3   2
#define FORMAT_COLOR_32FC4  3
#define FORMAT_COLOR_8UC4   4
#define FORMAT_COLOR_32FC1  5
#endif

#ifndef _COMMON_H
#define _COMMON_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#define GL_FAILURE              -1
#define GL_SUCCESS               0
#define GL_FALSE                 0
#define GL_TRUE                  1
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_COLOR_BUFFER_BIT 0x00004000
// data type
#define GL_UNSIGNED_INT 0x1405
// texture wrapping parameters
#define GL_REPEAT 0x2901
#define GL_MIRRORED_REPEAT 0x8370
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_CLAMP_TO_BORDER 0x812D
// texture filtering parameters
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
// cull face. The initial value is GL_BACK.
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_FRONT_AND_BACK 0x0408
// front face. The initial value is GL_CCW.
#define GL_CW 0x0900
#define GL_CCW 0x0901
typedef enum{
GL_UNDEF = -1,
// input mode
GL_ARRAY_BUFFER,
GL_BIND_VAO,
GL_ELEMENT_ARRAY_BUFFER,
// output draw mode
GL_TRIANGLES,
// target
GL_TEXTURE_2D, 
// params
GL_TEXTURE_WRAP_S,
GL_TEXTURE_WRAP_T,
GL_TEXTURE_BORDER_COLOR,
//fotmat
GL_RGB,
GL_RGBA,
GL_DEPTH_COMPONENT,
// usage
GL_STATIC_DRAW,
//data types (dtype)
GL_FLOAT,
GL_INT,
GL_BYTE,
GL_UNSIGNED_BYTE,
GL_VERTEX_ATTRIB_CONFIG,
GL_VERTEX_ARRAY_OBJECT,
GL_FRAMEBUFFER_ATTACHMENT_CONFIG,
//shader
GL_VERTEX_SHADER,
GL_FRAGMENT_SHADER,
// shader program
GL_BIND_PROGRAM,
// framebuf
GL_FRAMEBUFFER,
GL_DEPTH_ATTACHMENT,
// color buffer to draw
GL_NONE,
// capbilities
GL_DEPTH_TEST,
GL_CULL_FACE,
// texture units
GL_TEXTURE0,
GL_TEXTURE1,
GL_TEXTURE2,
GL_TEXTURE3,
GL_TEXTURE4,
GL_TEXTURE5,
GL_TEXTURE6,
GL_TEXTURE7,
GL_TEXTURE8,
GL_TEXTURE9,
GL_TEXTURE10,
GL_TEXTURE11,
GL_TEXTURE12,
GL_TEXTURE13,
GL_TEXTURE14,
GL_TEXTURE15
}GLenum;
#define THREAD_NUM  30
#define GL_MAX_VERTEX_ATTRIB_NUM    16
#define GL_MAX_TEXTURE_UNITS        16
#ifdef __cplusplus
}
#endif 
#endif
#ifndef _VEC_MATH_H
#define _VEC_MATH_H
#define GLM_FORCE_AVX2
#define GLM_FORCE_INLINE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
union data_t{
glm::vec2 vec2_var;
glm::vec3 vec3_var;
glm::vec4 vec4_var;
glm::mat2 mat2_var;
glm::mat3 mat3_var;
glm::mat4 mat4_var;
// if you want to pass int, still use the field sampler2D_var, they can share
union{
int sampler2D_var;
int int_var;
};
};
#endif
#ifndef _INNER_VARIABLE_H
#define _INNER_VARIABLE_H
#define INNER_GL_POSITION   1
#define INNER_GL_FRAGCOLOR  2
#endif
#ifndef _INNER_SUPPORT_H
#define _INNER_SUPPORT_H
#include <map>
#include <stdio.h>
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
void* tex_data;
int width;
int height;
int color_format;
int wrap_s;
int wrap_t;
glm::vec4 border_val;
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
get_sampler2D_data_fptr get_sampler2D;
class sampler2D{
public:
sampler2D():loaded_texture(false){}
bool loaded_texture;
int texunit_id;
int width;
int height;
// size = width * height
int size;
int color_format;
int wrap_s;
int wrap_t;
glm::vec4 border_val;
void* data;
filter_type filter;
sampler2D& operator=(int val){
texunit_id = val;
loaded_texture = false;
// printf("define value for sampler2D\n");
return *this;
}
inline void load_texture(){
if (loaded_texture)
return;
sampler_data_pack tmp = get_sampler2D(texunit_id);
width = tmp.width;
height = tmp.height;
size = width * height;
color_format = tmp.color_format;
data = tmp.tex_data;
filter = tmp.filter;
wrap_s = tmp.wrap_s;
wrap_t = tmp.wrap_t;
loaded_texture = true;
border_val = tmp.border_val;
}
};
glm::vec4 texture(sampler2D &samp, glm::vec2 &texcoord)
{
samp.load_texture();
glm::vec4 res = glm::vec4(1.0f);
if (samp.height == 0 || samp.width == 0)
throw std::runtime_error("invalid texture used in shader\n");
float scale = 1.0f / 255.0f;
int channel;
bool is_char = true;
switch (samp.color_format) {
case FORMAT_COLOR_8UC3:
channel = 3;
break;
case FORMAT_COLOR_8UC4:
channel = 4;
// printf("using RGBA\n");
break;
case FORMAT_COLOR_32FC1:
channel = 1;
is_char = false;
scale = 1.0f;
break;
default:
throw std::runtime_error("invalid color format\n");
break;
}
// direction s of texcoord
float tx = texcoord.x, ty = texcoord.y;
bool flag = false;
if(tx < 0.0f || tx >= 1.0f){
switch(samp.wrap_s){
case GL_CLAMP_TO_BORDER:
return samp.border_val;
case GL_REPEAT:
default:
tx = tx - (int)tx;
if(tx < 0.0f){
tx += 1.0f;
}
break;
}
}
// direction t of texcoord
if(ty < 0.0f || ty >= 1.0f){
switch(samp.wrap_t){
case GL_CLAMP_TO_BORDER:
return samp.border_val;
case GL_REPEAT:
default:
ty = ty - (int)ty;
if(ty < 0.0f){
ty += 1.0f;
}
break;
}
}
if (samp.filter == filter_type::NEAREST)
{
int x = tx * samp.width;
int y = ty * samp.height;
int index = y * samp.width + x;
for (int i = 0; i < channel; ++i) {
if (is_char){
unsigned char* sampler_data_array = (unsigned char*)samp.data;
res[i] = ((float)sampler_data_array[index * channel + i]) * scale;
}else{
float * sampler_data_array = (float*)samp.data;
res[i] = ((float)sampler_data_array[index * channel + i]) * scale;
}
}
}
else if (samp.filter == filter_type::BILINEAR)
{
float x = tx * samp.width;
float y = ty * samp.height;
glm::vec4 u00, u01, u10, u11;
int i00, i01, i11, i10;
i00 = (int)y * samp.width + (int)x;
i01 = (int)(y + 0.5f) * samp.width + (int)x;
i10 = (int)y * samp.width + (int)(x + 0.5f);
i11 = (int)(y + 0.5f) * samp.width + (int)(x + 0.5f);
i01 = i00 >= samp.size ? samp.size - 1 : i00;
i10 = i10 >= samp.size ? samp.size - 1 : i10;
i11 = i11 >= samp.size ? samp.size - 1 : i11;
for (int i = 0;i < channel;++i){
if (is_char){
unsigned char* sampler_data_array = (unsigned char*)samp.data;
u00[i] = ((float)sampler_data_array[i00 * channel + i]) * scale;
u01[i] = ((float)sampler_data_array[i01 * channel + i]) * scale;
u10[i] = ((float)sampler_data_array[i10 * channel + i]) * scale;
u11[i] = ((float)sampler_data_array[i11 * channel + i]) * scale;
}else{
float * sampler_data_array = (float*)samp.data;
u00[i] = ((float)sampler_data_array[i00 * channel + i]) * scale;
u01[i] = ((float)sampler_data_array[i01 * channel + i]) * scale;
u10[i] = ((float)sampler_data_array[i10 * channel + i]) * scale;
u11[i] = ((float)sampler_data_array[i11 * channel + i]) * scale;
}
}
float s = x - (int)x;
float t = y - (int)y;
glm::vec4 u0 = u00 + s * (u10 - u00);
glm::vec4 u1 = u01 + s * (u11 - u01);
// lerp
res = u0 + t * (u1 - u0);
}
return res;
}
#endif
#endif
)"";
#endif

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
#if (CONST_HEADER == 0)
    vector<string> headers;
    headers.push_back(string("../src/gl/glsl/os.h"));
    headers.push_back(string("../src/gl/formats.h"));
    headers.push_back(string("../include/gl/common.h"));
    headers.push_back(string("../src/gl/glsl/vec_math.h"));
    headers.push_back(string("../src/gl/glsl/inner_variable.h"));
    headers.push_back(string("../src/gl/glsl/inner_support.h"));
    dest = macros + extract_headers(headers) + prefix + class_shader_code + src + postfix;
    // printf("generate!\n");
#else
    dest = macros + const_header + prefix + class_shader_code + src + postfix;
#endif
}

