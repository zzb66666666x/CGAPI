#include "symbols.h"
#include <map>
#include <iostream>
#include <stdio.h>

using namespace std;

map<string, io_attrib> io_profile;

static string type_string_map[] = {
    "void",
    "vec2",
    "vec3",
    "vec4",
    "mat2",
    "mat3",
    "mat4",
    "float",
    "int",
    "double",
    "bool"
};

static string io_string_map[] = {
    "normal",
    "in",
    "out",
    "uniform"
};

static const char* input_port_proto = "\nvoid input_port(std::map<std::string, data_t>& indata){\n";

static const char* output_port_proto = "\nvoid output_port(std::map<std::string, data_t>& outdata){\n";

static void assign_input_value(string& code, const char* name, int dtype){
    string var_name = name;
    string type_name = type_string_map[dtype];
    code = string("    ") + var_name + string(" = indata[\"") + var_name + string("\"].") + type_name + string("_var;\n");
}

static void fetch_output_value(string& code, const char* name, int dtype){
    string var_name = name;
    string type_name = type_string_map[dtype];
    code = string("    outdata.emplace(\"") + var_name + string("\", (data_t){.") + type_name + string("_var = ") + var_name + string("});\n");
}

static string uniform_var_input_port(const char* name, int dtype){
    string var_name = name;
    string type_name = type_string_map[dtype];    
    return string("\nvoid set_uniform_") + var_name + string("(data_t data){\n    ") + 
           var_name + string(" = data.") + type_name + string("_var;\n}\n");
}

static string uniform_var_output_port(const char* name, int dtype){
    string var_name = name;
    string type_name = type_string_map[dtype];  
    return string("\ndata_t get_uniform_") + var_name + string("(){\n    ") + 
           string("return (data_t){.") + type_name + "_var = " + var_name + string("};\n}\n");
}

void emplace_profile(const char* name, int io, int dtype, int layout){
    string key = name;
    io_profile.emplace(key, (io_attrib){io, dtype, layout});
}

buffer_t code_for_input(){
    buffer_t input_port;
    init_buffer(&input_port, 500);
    register_code(&input_port, input_port_proto);
    for (auto it = io_profile.begin(); it != io_profile.end(); it++){
        if (it->second.io == INPUT_VAR){
            string s;
            assign_input_value(s, it->first.c_str(), it->second.dtype);
            register_code(&input_port, s.c_str());
        }
    }
    register_code(&input_port, "}\n");
    return input_port;
}

buffer_t code_for_output(){
    buffer_t output_port;
    init_buffer(&output_port, 500);
    register_code(&output_port, output_port_proto);
    for(auto it = io_profile.begin(); it != io_profile.end(); it++){
        if (it->second.io == OUTPUT_VAR){
            string s;
            fetch_output_value(s, it->first.c_str(), it->second.dtype);
            register_code(&output_port, s.c_str());
        }
    }
    register_code(&output_port, "}\n");
    return output_port;
}

buffer_t code_for_uniform(){
    buffer_t uniform_port;
    init_buffer(&uniform_port, 500);
    for (auto it = io_profile.begin(); it != io_profile.end(); it++){
        if (it->second.io == UNIFORM_VAR){
            string s1 = uniform_var_input_port(it->first.c_str(), it->second.dtype);
            string s2 = uniform_var_output_port(it->first.c_str(), it->second.dtype);
            register_code(&uniform_port, s1.c_str());
            register_code(&uniform_port, s2.c_str());
        }
    }
    return uniform_port;
}

void* get_profile(){
    return (void*)(&io_profile);
}

void clear_profile(){
    io_profile.clear();
    map<string, io_attrib> empty;
    io_profile.swap(empty);
}

void print_profile(){
    auto it = io_profile.begin();
    cout<<"////////// SHOWING IO PROFILE //////////"<<endl;
    for(; it!= io_profile.end(); it++){
        cout<<it->first<<": "<<type_string_map[it->second.dtype]<<"\t"
            <<io_string_map[it->second.io]<<"\tlayout = "<<it->second.layout<<endl;
    }
    cout<<"////////// END OF IO PROFILE //////////"<<endl;
}