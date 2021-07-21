#ifndef _SHADER_HPP_
#define _SHADER_HPP_
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iostream>
#include <assert.h>
#include <map>
#include <vector>
// windows API
#include <wtypes.h>   
#include <winbase.h>   
// parser
#include "parse.h"
#include "symbols.h"
#include "vec_math.h"
#include "translate.h"
#include "inner_variable.h"

__declspec(dllimport) void glsl_main();
__declspec(dllimport) void input_port(std::map<std::string, data_t>& indata); 
__declspec(dllimport) void output_port(std::map<std::string, data_t>& outdata); 
__declspec(dllimport) void input_uniform_dispatch(int idx, data_t& data); 
__declspec(dllimport) data_t output_uniform_dispatch(int idx); 
__declspec(dllimport) void set_inner_variable(int variable, data_t& data);
__declspec(dllimport) void get_inner_variable(int variable, data_t& data);

typedef void (*shader_main)(void);
typedef void (*shader_input_port)(std::map<std::string, data_t>& indata);
typedef void (*shader_output_port)(std::map<std::string, data_t>& outdata);
typedef void (*shader_input_uniform_dispatch)(int idx, data_t& data); 
typedef data_t (*shader_output_uniform_dispatch)(int idx); 
typedef void (*shader_set_inner_variable)(int variable, data_t& data);
typedef void (*shader_get_inner_variable)(int variable, data_t& data);


typedef struct{
    shader_main main;
    shader_input_port input;
    shader_output_port output;
    shader_set_inner_variable set_inner;
    shader_get_inner_variable get_inner;
}ftable_t;

class Shader{
    public:
    Shader(int cpu_num){
        hDLL_list.resize(cpu_num);
        glsl_main_list.resize(cpu_num);
        input_port_list.resize(cpu_num);
        output_port_list.resize(cpu_num);
        input_uniform_dispatch_list.resize(cpu_num);
        output_uniform_dispatch_list.resize(cpu_num);
        set_inner_variable_list.resize(cpu_num);
        get_inner_variable_list.resize(cpu_num);
    }
    Shader(const char* glsl_path, int cpu_num){
        std::ifstream glsl_file;
        glsl_file.open(glsl_path);
        std::stringstream glsl_string_stream;
        // read file's buffer contents into streams
        glsl_string_stream << glsl_file.rdbuf();		
        // close file handlers
        glsl_file.close();
        // convert stream into string
        glsl_code = glsl_string_stream.str();
        hDLL_list.resize(cpu_num);
        glsl_main_list.resize(cpu_num);
        input_port_list.resize(cpu_num);
        output_port_list.resize(cpu_num);
        input_uniform_dispatch_list.resize(cpu_num);
        output_uniform_dispatch_list.resize(cpu_num);
        set_inner_variable_list.resize(cpu_num);
        get_inner_variable_list.resize(cpu_num);
    }

    ~Shader(){
        for (int i=0; i<hDLL_list.size(); i++){
            FreeLibrary(hDLL_list[i]);
        }
    }

    inline void set_glsl_code(std::string code){
        glsl_code = code;
    }

    inline void load_shader(){   
        for (int i=0; i<hDLL_list.size(); i++){
            hDLL_list[i] = LoadLibrary(TEXT("test.dll")); 
            if(hDLL_list[i] == NULL)
                std::cout<<"Error!!!\n";  
            glsl_main_list[i] = (shader_main)GetProcAddress(hDLL_list[i],"glsl_main"); 
            input_port_list[i] = (shader_input_port)GetProcAddress(hDLL_list[i],"input_port");
            output_port_list[i] = (shader_output_port)GetProcAddress(hDLL_list[i],"output_port");
            input_uniform_dispatch_list[i] = (shader_input_uniform_dispatch)GetProcAddress(hDLL_list[i],"input_uniform_dispatch");
            output_uniform_dispatch_list[i] = (shader_output_uniform_dispatch)GetProcAddress(hDLL_list[i],"output_uniform_dispatch");
            set_inner_variable_list[i] = (shader_set_inner_variable)GetProcAddress(hDLL_list[i],"set_inner_variable");
            get_inner_variable_list[i] = (shader_get_inner_variable)GetProcAddress(hDLL_list[i],"get_inner_variable");       
        }
    }

    inline void compile(){
        char* output_code_buffer;
        int output_code_buffer_size;
        parse_string(glsl_code.c_str(), &output_code_buffer, &output_code_buffer_size);
        std::string parser_out_string = output_code_buffer;
        // std::cout<<"////////// PARSER RAW OUTPUT //////////"<<std::endl;
        // std::cout<<"code buffer size: "<<output_code_buffer_size<<std::endl;
        // std::cout<<"code:"<<std::endl;
        std::cout<<parser_out_string<<std::endl;
        // std::cout<<"////////// END OF RAW PARSED CODE //////////"<<std::endl;
        free(output_code_buffer);
        io_profile = *((std::map<std::string, io_attrib>*)get_profile());
        uniform_map = *((std::map<std::string, int>*)get_uniform_map());
        get_layouts();
        clear_profile();
        clear_uniform_map();
        cpp_code_generate(parser_out_string, cpp_code);
        // std::cout<<"////////// FINAL CPP CODE //////////"<<std::endl;
        // std::cout<<cpp_code;
        // std::cout<<"////////// END OF CPP CODE //////////"<<std::endl;
        // std::cout<<"////////// UNIFORM MAP //////////"<<std::endl;
        // for (auto it = uniform_map.begin(); it != uniform_map.end(); it++){
        //     std::cout<<it->first<<"    "<<it->second<<std::endl;
        // }
        // std::cout<<"////////// END OF UNIFORM MAP //////////"<<std::endl;
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

    inline ftable_t get_shader_utils(int idx){
        ftable_t ret;
        ret.input = input_port_list[idx];
        ret.output = output_port_list[idx];
        ret.main = glsl_main_list[idx];
        ret.set_inner = set_inner_variable_list[idx];
        ret.get_inner = get_inner_variable_list[idx];
        return ret;
    }

    inline data_t fetch_uniform(int thread_id, int idx){
        return output_uniform_dispatch_list[thread_id](idx);
    }

    inline void set_uniform(int idx, data_t& data){
        for (int thread_id = 0; thread_id<hDLL_list.size(); thread_id++){
            input_uniform_dispatch_list[thread_id](idx, data);
        }
    }

    std::map<std::string, io_attrib> io_profile;
    std::map<std::string, int> uniform_map;
    std::map<std::string, io_attrib*> layouts;

    private:
    inline void get_layouts(){
        layouts.clear();
        for (auto it = io_profile.begin(); it!= io_profile.end(); it++){
            if (it->second.layout>=0){
                layouts.emplace(it->first, &(it->second));
            }
        }
    }

    std::string glsl_code;
    std::string cpp_code;
    // windows dll object
    std::vector<HINSTANCE> hDLL_list;
    // interface functions
    std::vector<shader_main> glsl_main_list;
    std::vector<shader_input_port> input_port_list;
    std::vector<shader_output_port> output_port_list;
    std::vector<shader_input_uniform_dispatch> input_uniform_dispatch_list;
    std::vector<shader_output_uniform_dispatch> output_uniform_dispatch_list;
    std::vector<shader_set_inner_variable> set_inner_variable_list;
    std::vector<shader_get_inner_variable> get_inner_variable_list;
};

#endif