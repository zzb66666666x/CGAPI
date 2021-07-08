#include <stdio.h>
#include <libtcc.h>

const char* program = \
    "#include <tcclib.h>"
    "#include <glsl_math.h>"
    "#include <hello.h>"
    "mat4 m = {"
    "2, 0, 0, 0,"
    "0, 2, 0, 0,"
    "0, 0, 2, 0,"
    "0, 0, 0, 2"
    "};"
    "vec4 v = {1,2,3,4};"
    "int glsl_main(){"
    "   int i;"
    "   for (i=0; i<4; i++){"
    "      printf(\"%f  \", v[i]);"
    "   }"
    "   int val = hello();"
    "   printf(\"%f \n\", val);"
    // "   vec4 out;"
    // "   mat4_mulv(m, v, out);"
    // "   printf(%f, %f, %f, %f, out[0], out[1], out[2], out[3]);"
    "   return 0;"
    "}"
    ;

typedef int (*glsl_func)(void);

int main(int argc, char **argv)
{
    // tcc usage
    TCCState* s = tcc_new();
    if(!s){
        printf("Can’t create a TCC context\n");
        return 1;
    }
    tcc_add_include_path(s, "../include");
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    tcc_add_library_path(s, "../lib");
    tcc_add_library(s, "hello");

    if (tcc_compile_string(s, program) > 0) {
        printf("Compilation error !\n");
        return 2;
    }
    tcc_relocate(s, TCC_RELOCATE_AUTO);
    // interact with C program in the string
    glsl_func shader = (glsl_func)tcc_get_symbol(s, "glsl_main");
    shader();
    
    tcc_delete(s);
    return 0;
}