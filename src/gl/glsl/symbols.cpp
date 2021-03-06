#include "symbols.h"
#include <map>
#include <iostream>
#include <stdio.h>

using namespace std;

map<string, io_attrib> io_profile;
map<string, int> uniform_map;

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
    "bool",
    "sampler2D"
};

static string io_string_map[] = {
    "normal",
    "in",
    "out",
    "uniform",
    "inner"
};

static const char* input_port_proto = "\nvoid input_port(std::map<std::string, data_t>& indata){\n";

static const char* output_port_proto = "\nvoid output_port(std::map<std::string, data_t>& outdata){\n";

static const char* input_uniform_dispatch = "\nvoid input_uniform_dispatch(int idx, data_t& data){\n    (this->*this->input_uniform_fmap[idx])(data);\n}\n";

static const char* output_uniform_dispatch = "\ndata_t output_uniform_dispatch(int idx){\n    return (this->*this->output_uniform_fmap[idx])();\n}\n";

static const char* input_uniform_dispatch_empty = "\nvoid input_uniform_dispatch(int idx, data_t& data){}\n";

static const char* output_uniform_dispatch_empty = "\ndata_t output_uniform_dispatch(int idx){\n    data_t ret; return ret;\n}\n";

static const char* input_uniform_fmap = "\nset_uniform input_uniform_fmap[";

static const char*  output_uniform_fmap = "\nget_uniform output_uniform_fmap[";

static bool is_inner_type(int dtype){
    switch(dtype){
        case TYPE_SAMPLER2D:
            return true;
        default:
            break;
    }
    return false;
}

static void assign_input_value(string& code, const char* name, int dtype){
    string var_name = name;
    string type_name = type_string_map[dtype];
    code = string("    ") + var_name + string(" = indata[\"") + var_name + string("\"].") + type_name + string("_var;\n");
}

static void fetch_output_value(string& code, const char* name, int dtype){
    string var_name = name;
    string type_name = type_string_map[dtype];
    // code = string("    outdata[\"") + var_name + string("\"] = (data_t){.") + type_name + string("_var = ") + var_name + string("};\n");
    code = string("    outdata[\"") + var_name + string("\"].") + type_name + string("_var = ") + var_name + string(";\n");
}

static string uniform_var_input_port(const char* name, int dtype){
    string var_name = name;
    string type_name = type_string_map[dtype];    
    return string("\nvoid set_uniform_") + var_name + string("(data_t& data){\n    ") + 
           var_name + string(" = data.") + type_name + string("_var;\n}\n");
}

static string uniform_var_output_port(const char* name, int dtype){
    string var_name = name;
    string type_name = type_string_map[dtype];  
    if (is_inner_type(dtype)){
        return string("\ndata_t get_uniform_") + var_name + string("(){\n    ") + 
               string("return (data_t){.") + type_name + "_var = " + var_name + string(".texunit_id};\n}\n");
    }else{
        return string("\ndata_t get_uniform_") + var_name + string("(){\n    ") + 
               string("return (data_t){.") + type_name + "_var = " + var_name + string("};\n}\n");
    }
}

void emplace_profile(const char* name, int io, int dtype, int layout){
    string key = name;
    // io_profile.emplace(key, (io_attrib){io, dtype, layout});
    io_profile[key] = (io_attrib) { io, dtype, layout };
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
    int cnt = 0;
    string uni_in_fmap = input_uniform_fmap;
    string uni_out_fmap = output_uniform_fmap;
    for (auto it = io_profile.begin(); it != io_profile.end(); it++){
        if (it->second.io == UNIFORM_VAR){
            cnt++;
        }
    }
    if (cnt > 0){
        uni_in_fmap += (to_string(cnt) + string("] = {"));
        uni_out_fmap += (to_string(cnt) + string("] = {"));
        cnt = 0;
        for (auto it = io_profile.begin(); it != io_profile.end(); it++){
            if (it->second.io == UNIFORM_VAR){
                string s1 = uniform_var_input_port(it->first.c_str(), it->second.dtype);
                string s2 = uniform_var_output_port(it->first.c_str(), it->second.dtype);
                register_code(&uniform_port, s1.c_str());
                register_code(&uniform_port, s2.c_str());
                uniform_map.emplace(it->first, cnt);
                uni_in_fmap += (string("set_uniform_") + it->first + string(","));
                uni_out_fmap += (string("get_uniform_") + it->first + string(","));
                cnt++;
            }
        }
        uni_in_fmap[uni_in_fmap.size()-1] = '}';
        uni_out_fmap[uni_out_fmap.size()-1] = '}';    
        uni_in_fmap += string(";\n");
        uni_out_fmap += string(";\n");
        string func_map_size = to_string(cnt);
        register_code(&uniform_port, uni_in_fmap.c_str());
        register_code(&uniform_port, uni_out_fmap.c_str());
        register_code(&uniform_port, input_uniform_dispatch);
        register_code(&uniform_port, output_uniform_dispatch);
    }else{
        register_code(&uniform_port, input_uniform_dispatch_empty);
        register_code(&uniform_port, output_uniform_dispatch_empty);
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

void* get_uniform_map(){
    return (void*)(&uniform_map);
}

void clear_uniform_map(){
    uniform_map.clear();
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