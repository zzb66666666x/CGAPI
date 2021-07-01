#include <stdio.h>
#include <libtcc.h>
#include "support.h"

const char* program = \
    "#include <tcclib.h>"
    "#include \"support.h\""
    "SHADER_IN_PORT mydata;"
    "void foo(SHADER_IN_PORT input){"
    "   mydata = input;"
    "}"
    "void disp(){"
    "   printf(\"%f, %f, %f\", mydata.x, mydata.y, mydata.z);"
    "}"
    ;

typedef void (*fun_ptr)(SHADER_IN_PORT);
typedef void (*fun_ptr2)(void);

int main(int argc, char **argv)
{
    TCCState* s = tcc_new();
    if(!s){
        printf("Can’t create a TCC context\n");
        return 1;
    }
    tcc_add_include_path(s, "../include");
    tcc_add_include_path(s, "../");
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    tcc_add_library_path(s, "../lib");
    if (tcc_compile_string(s, program) > 0) {
        printf("Compilation error !\n");
        return 2;
    }

    tcc_relocate(s, TCC_RELOCATE_AUTO);

    SHADER_IN_PORT port_data;
    port_data.x = 1;
    port_data.y = 2;
    port_data.z = 3;
    fun_ptr foo = (fun_ptr)tcc_get_symbol(s, "foo");
    foo(port_data);

    fun_ptr2 disp = (fun_ptr2)tcc_get_symbol(s, "disp");
    disp();
   
    tcc_delete(s);
    return 0;
}