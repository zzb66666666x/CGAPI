#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <map>
// windows API
#include<wtypes.h>   
#include <winbase.h>   
// parser
#include "parse.h"
#include "symbols.h"
#include "vec_math.h"
#include "translate.h"

typedef int* (*callback_t)();

__declspec(dllimport) void glsl_main();
__declspec(dllimport) void input_port(std::map<std::string, data_t>& indata); 
__declspec(dllimport) void output_port(std::map<std::string, data_t>& outdata); 
__declspec(dllimport) void input_uniform_dispatch(int idx, data_t data); 
__declspec(dllimport) data_t output_uniform_dispatch(int idx); 
__declspec(dllimport) void get_main_program_data(callback_t callback);

typedef void (*shader_main)(void);
typedef void (*shader_input_port)(std::map<std::string, data_t>& indata);
typedef void (*shader_output_port)(std::map<std::string, data_t>& outdata);
typedef void (*shader_input_uniform_dispatch)(int idx, data_t data); 
typedef data_t (*shader_output_uniform_dispatch)(int idx); 
typedef void (*shader_get_main_program_data)(callback_t callback);
class Shader{
    public:
    Shader(){}
    Shader(const char* glsl_path){
        std::ifstream glsl_file;
        glsl_file.open(glsl_path);
        std::stringstream glsl_string_stream;
        // read file's buffer contents into streams
        glsl_string_stream << glsl_file.rdbuf();		
        // close file handlers
        glsl_file.close();
        // convert stream into string
        glsl_code = glsl_string_stream.str();
        char* output_code_buffer;
        int output_code_buffer_size;
        parse_string(glsl_code.c_str(), &output_code_buffer, &output_code_buffer_size);
        std::string parser_out_string = output_code_buffer;
        std::cout<<"////////// PARSER RAW OUTPUT //////////"<<std::endl;
        std::cout<<"code buffer size: "<<output_code_buffer_size<<std::endl;
        std::cout<<"code:"<<std::endl;
        std::cout<<parser_out_string<<std::endl;
        std::cout<<"////////// END OF RAW PARSED CODE //////////"<<std::endl;
        free(output_code_buffer);
        io_profile = *((std::map<std::string, io_attrib>*)get_profile());
        uniform_map = *((std::map<std::string, int>*)get_uniform_map());
        clear_profile();
        clear_uniform_map();
        cpp_code_generate(parser_out_string, cpp_code);
        std::cout<<"////////// FINAL CPP CODE //////////"<<std::endl;
        std::cout<<cpp_code;
        std::cout<<"////////// END OF CPP CODE //////////"<<std::endl;
        std::cout<<"////////// UNIFORM MAP //////////"<<std::endl;
        for (auto it = uniform_map.begin(); it != uniform_map.end(); it++){
            std::cout<<it->first<<"    "<<it->second<<std::endl;
        }
        std::cout<<"////////// END OF UNIFORM MAP //////////"<<std::endl;
    }

    ~Shader(){
        FreeLibrary(hDLL);
    }

    inline void load_shader(){   
        hDLL = LoadLibrary(TEXT("test.dll")); 
        if(hDLL == NULL)
            std::cout<<"Error!!!\n";  
        glsl_main = (shader_main)GetProcAddress(hDLL,"glsl_main"); 
        input_port = (shader_input_port)GetProcAddress(hDLL,"input_port");
        output_port = (shader_output_port)GetProcAddress(hDLL,"output_port");
        input_uniform_dispatch = (shader_input_uniform_dispatch)GetProcAddress(hDLL,"input_uniform_dispatch");
        output_uniform_dispatch = (shader_output_uniform_dispatch)GetProcAddress(hDLL,"output_uniform_dispatch");
        get_main_program_data = (shader_get_main_program_data)GetProcAddress(hDLL, "get_main_program_data");
    }
    
    shader_main glsl_main;
    shader_input_port input_port;
    shader_output_port output_port;
    shader_input_uniform_dispatch input_uniform_dispatch;
    shader_output_uniform_dispatch output_uniform_dispatch;
    shader_get_main_program_data get_main_program_data;

    inline void compile(){
        FILE *proc = popen("g++ -fPIC -shared -o test.dll -x c++ -", "w");
        if(!proc) {
            perror("popen g++");
        }
        fwrite(cpp_code.c_str(), sizeof(char), strlen(cpp_code.c_str()), proc);
        if(ferror(proc)) {
            perror("writing prog");
        }
        if(pclose(proc) == -1) {
            perror("pclose g++");
        }
    }

    std::map<std::string, io_attrib> io_profile;
    std::map<std::string, int> uniform_map;

    private:
        std::string glsl_code;
        std::string cpp_code;
        // windows dll object
        HINSTANCE hDLL;
};