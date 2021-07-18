#include "translate.h"

using namespace std;

static string prefix = \
"#include \"vec_math.h\" \n"
"#include <stdio.h> \n"
"#include <map> \n"
"#include <string> \n"
"using namespace glm; \n"
"#ifdef __cplusplus \n"
"extern \"C\" { \n"
"#endif \n"
"__declspec(dllexport) void glsl_main(); \n"
"__declspec(dllexport) void input_port(std::map<std::string, data_t>& indata); \n"
"__declspec(dllexport) void output_port(std::map<std::string, data_t>& outdata); \n"
"__declspec(dllexport) void input_uniform_dispatch(int idx, data_t data); \n"
"__declspec(dllexport) data_t output_uniform_dispatch(int idx); \n"
"vec4 gl_Position;\n"
;

static string postfix = \
"#ifdef __cplusplus \n"
"} \n"
"#endif \n"
;

void cpp_code_generate(string& src, string& dest){
    dest = prefix + src + postfix;
}