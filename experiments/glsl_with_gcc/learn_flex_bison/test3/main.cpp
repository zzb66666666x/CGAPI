#include <iostream>
#include <stdio.h>
#include "parse.h"
#include "symbols.h"

using namespace std;

const char* prog = \
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aNormal;\n"
"layout (location = 2) in vec2 aTexCoord;\n"
"out vec2 TexCoord;\n"
"out vec3 Normal;\n"
"out vec3 FragPos;\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"uniform mat4 model;\n"
"int main(void)\n"
"{\n"
"    FragPos = vec3(model*vec4(aPos, 1.0));\n"
"    gl_Position = projection * view * vec4(FragPos,1.0);\n"
"    Normal = mat3(transpose(inverse(model))) * aNormal;\n"
"    TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
"}\n"
;

int main(){
    char* output_code_buffer;
    int output_code_buffer_size;
    parse_file("code.glsl", &output_code_buffer, &output_code_buffer_size);
    cout<<output_code_buffer<<endl;
    free(output_code_buffer);
    output_code_buffer = NULL;
    clear_profile();
    cout<<"##### next testing parsing string in memory #####"<<endl<<endl;
    parse_string(prog, &output_code_buffer, &output_code_buffer_size);
    cout<<output_code_buffer<<endl;
    free(output_code_buffer);
    output_code_buffer = NULL;
    clear_profile();
    return 0;
}