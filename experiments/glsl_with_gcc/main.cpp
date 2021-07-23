#include <stdio.h>
#include <string.h>
#include <map>
#include <string>

#include "shader.hpp"

using namespace std;

int data_for_dll[5] = {1,2,3,4,5};

int* get_data(){
    return data_for_dll;
}

int main() {
    Shader myshader("code.glsl");
    myshader.compile();
    myshader.load_shader();
    // for (auto it = myshader.io_profile.begin(); it != myshader.io_profile.end(); it++){
    //     if (it->second.io == INPUT_VAR){

    //     }
    // }
    map<string, data_t> input_data;
    input_data.emplace("aPos", (data_t){.vec3_var = glm::vec3(1.0f, 1.0f, 1.0f)});
    input_data.emplace("aNormal", (data_t){.vec3_var = glm::vec3(2.0f, 2.0f, 2.0f)});
    input_data.emplace("aTexCoord", (data_t){.vec2_var = glm::vec2(3.0f, 3.0f)});
    // cout<<input_data["aPos"].vec3_var.x<<input_data["aPos"].vec3_var.y<<input_data["aPos"].vec3_var.z;
    myshader.input_port(input_data);
    glm::mat4 model(1.0f);
    glm::mat4 view(2.0f);
    glm::mat4 projection(3.0f);
    myshader.input_uniform_dispatch(myshader.uniform_map["model"], (data_t){.mat4_var = model});
    myshader.input_uniform_dispatch(myshader.uniform_map["view"], (data_t){.mat4_var = view});
    myshader.input_uniform_dispatch(myshader.uniform_map["projection"], (data_t){.mat4_var = projection});
    map<string, data_t> output;
    myshader.glsl_main();
    myshader.output_port(output);
    for (auto it = output.begin(); it != output.end(); it++){
        switch(myshader.io_profile[it->first].dtype){
            case TYPE_VEC2:
                {
                glm::vec2 tmp= it->second.vec2_var;
                cout<<it->first<<": ";
                cout<<tmp.x<<" , "<<tmp.y<<endl;
                }
                break;
            case TYPE_VEC3:
                {
                glm::vec3 tmp = it->second.vec3_var;
                cout<<it->first<<": ";
                cout<<tmp.x<<" , "<<tmp.y<<" , "<<tmp.z<<endl;
                }
                break;
            case TYPE_VEC4:
                {
                glm::vec4 tmp = it->second.vec4_var;
                cout<<it->first<<": ";
                cout<<tmp.x<<" , "<<tmp.y<<" , "<<tmp.z<<" , "<<tmp.w<<endl;                
                }
            default:
                break;
        }
    }
    // testing communication between main program and shader
    myshader.get_main_program_data(get_data);
    return 0;
}