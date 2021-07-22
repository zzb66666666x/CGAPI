#include <iostream>
#include "../gl/glsl/shader.hpp"

using namespace std;

int main(){
    Shader myshader("code.glsl", 1);
    myshader.compile("unit_test_glsl.dll");
    myshader.load_shader();
    ShaderInterface* intf = myshader.get_shader_utils(0);
    map<string, data_t> input_data;
    input_data.emplace("aPos", (data_t){.vec3_var = glm::vec3(1.0f, 1.0f, 1.0f)});
    input_data.emplace("aNormal", (data_t){.vec3_var = glm::vec3(2.0f, 2.0f, 2.0f)});
    input_data.emplace("aTexCoord", (data_t){.vec2_var = glm::vec2(3.0f, 3.0f)});
    // cout<<input_data["aPos"].vec3_var.x<<input_data["aPos"].vec3_var.y<<input_data["aPos"].vec3_var.z;
    intf->input_port(input_data);
    data_t model = {.mat4_var = glm::mat4(1.0f)};
    data_t view = {.mat4_var = glm::mat4(2.0f)};
    data_t projection = {.mat4_var = glm::mat4(3.0f)};
    intf->input_uniform_dispatch(myshader.uniform_map["model"], model);
    intf->input_uniform_dispatch(myshader.uniform_map["view"], view);
    intf->input_uniform_dispatch(myshader.uniform_map["projection"], projection);
    map<string, data_t> output;
    intf->glsl_main();
    intf->output_port(output);
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
    return 0;
}