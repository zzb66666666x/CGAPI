#include <iostream>
#include "../gl/glsl/shader.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

int main(){
    Shader myshader(GL_FRAGMENT_SHADER, "../shader/sampler.glsl", 1);
    myshader.compile("unit_test_sampler_type.dll");
    // myshader.load_shader();
    // ShaderInterface* intf = myshader.get_shader_utils(0);

    // int width, height, nrChannels;
    // unsigned char *data = stbi_load("../resources/textures/container.jpg", &width, &height, &nrChannels, 0);
    // if (data)
    // {
    //     // go on testing sampler2D functionality
    // }
    // else
    // {
    //     std::cout << "Failed to load texture" << std::endl;
    // }
    // stbi_image_free(data);

    return 0;
}