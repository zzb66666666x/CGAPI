#ifndef _SHADER_HPP_
#define _SHADER_HPP_
#include "os.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iostream>
#include <assert.h>
#include <map>
#include <vector>
#ifdef OS_WIN
// windows API
#include <wtypes.h>   
#include <winbase.h>   
#endif
#ifdef OS_LINUX
// linux API
#include <dlfcn.h>
#endif
// parser
#include "parse.h"
#include "symbols.h"
#include "translate.h"
#include "inner_variable.h"
#include "inner_support.h"
// OpenGL consts
#include "../../../include/gl/common.h"
#include "../formats.h"

#ifdef OS_WIN
__declspec(dllimport) ShaderInterface* create_shader_inst();
__declspec(dllimport) void destroy_shader_inst(ShaderInterface* inst);
#endif
#ifdef OS_LINUX
ShaderInterface* create_shader_inst();
void destroy_shader_inst(ShaderInterface* inst);
#endif

typedef ShaderInterface* (*func_create_shader_inst)();
typedef void (*func_destroy_shader_inst)(ShaderInterface*);

class Shader{
    public:
    Shader(GLenum type, int cpu_num){
        glsl_shader_insts.resize(cpu_num);
        shader_type = type;
        inst_num = cpu_num;
        compiled = false;
        loaded_inst = false;
        has_source = false;
    }
    Shader(GLenum type, const char* glsl_path, int cpu_num){
        shader_type = type;
        std::ifstream glsl_file;
        glsl_file.open(glsl_path);
        std::stringstream glsl_string_stream;
        // read file's buffer contents into streams
        glsl_string_stream << glsl_file.rdbuf();		
        // close file handlers
        glsl_file.close();
        // convert stream into string
        glsl_code = glsl_string_stream.str();
        glsl_shader_insts.resize(cpu_num);
        inst_num = cpu_num;
        loaded_inst = false;
        compiled = false;
        has_source = true;
    }

    ~Shader(){
        if (loaded_inst){
            for (int i=0; i<inst_num; i++){
                destroy_shader_inst(glsl_shader_insts[i]);
            }
        }
        FreeLibrary(hDLL);
    }

    inline void set_glsl_code(std::string code){
        glsl_code = code;
        has_source = true;
    }

    inline void load_shader(){   
        if (compiled == false)
            return;
        #ifdef OS_WIN
        hDLL = LoadLibrary(TEXT(output_name.c_str())); 
        if(hDLL == NULL)
            throw std::runtime_error("fail to load shader dynamic lib\n");
        create_shader_inst = (func_create_shader_inst)GetProcAddress(hDLL,"create_shader_inst"); 
        destroy_shader_inst = (func_destroy_shader_inst)GetProcAddress(hDLL,"destroy_shader_inst");    
        #endif
        #ifdef OS_LINUX
        handle = dlopen(output_name.c_str(), RTLD_LAZY);
        if (handle == NULL)
            throw std::runtime_error("fail to load shader dynamic lib\n");
        create_shader_inst = (func_create_shader_inst)dlsym(handle, "create_shader_inst");
        destroy_shader_inst = (func_destroy_shader_inst)dlsym(handle, "destroy_shader_inst");
        #endif
        for (int i=0; i<inst_num; i++){
            glsl_shader_insts[i] = create_shader_inst();
        }
        loaded_inst = true;
    }

    inline void compile(std::string out){
        if (has_source == false)
            return;
        char* output_code_buffer;
        int output_code_buffer_size;
        int syntax = parse_string(glsl_code.c_str(), &output_code_buffer, &output_code_buffer_size);
        if (syntax < 0)
            throw std::runtime_error("syntax error in glsl\n");
        std::string parser_out_string = output_code_buffer;
        // std::cout<<"////////// PARSER RAW OUTPUT //////////"<<std::endl;
        // std::cout<<"code buffer size: "<<output_code_buffer_size<<std::endl;
        // std::cout<<"code:"<<std::endl;
        // std::cout<<parser_out_string<<std::endl;
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
        std::string compile_cmd_1("g++ -fPIC -shared -std=c++17 -O3 -o ");
        std::string compile_cmd_2(" -x c++ -");
        std::string compile_cmd = compile_cmd_1 + out + compile_cmd_2;
        // FILE *proc = popen("g++ -fPIC -shared -o test.dll -x c++ -", "w");
        FILE *proc = popen(compile_cmd.c_str(), "w");
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
        output_name = out;
        compiled = true;
    }

    inline ShaderInterface* get_shader_utils(int thread_id){
        if (thread_id >= glsl_shader_insts.size())
            return nullptr;
        return glsl_shader_insts[thread_id];
    }

    inline void set_uniform_variables(int uniform_id, data_t data){
        for (int thread_id = 0; thread_id<glsl_shader_insts.size(); thread_id++){
            glsl_shader_insts[thread_id]->input_uniform_dispatch(uniform_id, data);
        }
    }

    inline void set_sampler2D_callback(get_sampler2D_data_fptr func){
        if (inst_num < 1 || !loaded_inst)
            return;
        glsl_shader_insts[0]->set_sampler2D_callback(func);
    }

    inline void print_cpp_code(){
        std::cout<<cpp_code<<std::endl;
    }

    GLenum shader_type;
    std::map<std::string, io_attrib> io_profile;
    std::map<std::string, int> uniform_map;
    std::map<std::string, io_attrib*> layouts;
    func_create_shader_inst create_shader_inst;
    func_destroy_shader_inst destroy_shader_inst;

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
    std::string output_name;
    // dynamic lib
    #ifdef OS_WIN
    HINSTANCE hDLL;
    #endif
    #ifdef OS_LINUX
    void* handle;
    #endif
    int inst_num;
    bool loaded_inst;
    bool compiled;
    bool has_source;
    std::vector<ShaderInterface*> glsl_shader_insts;
};

#endif
