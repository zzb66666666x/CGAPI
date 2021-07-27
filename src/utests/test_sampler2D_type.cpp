#include <iostream>
#include <stdio.h>
#include "../gl/glsl/shader.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using namespace std;

#define GET_INDEX(x, y, width) (y * width + (x))

sampler_data_pack pack;

sampler_data_pack register_sampler(int texunit_id){
    // ignore the texture unit id here
    return pack;
}

typedef struct{
    float R;
    float G;
    float B;
}pixel_color_t;

int main(){
    Shader myshader(GL_FRAGMENT_SHADER, "../shader/sampler.glsl", 1);
    myshader.compile("unit_test_sampler_type.dll");
    myshader.load_shader();
    ShaderInterface* intf = myshader.get_shader_utils(0);

    int width, height, nrChannels;
    unsigned char *data = stbi_load("../resources/textures/container.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        // go on testing sampler2D functionality
        if (nrChannels != 3)
            throw std::runtime_error("not suitable color format\n");
        printf("image data: %x\n", data);
        pack.color_format = FORMAT_COLOR_8UC3;
        pack.filter = NEAREST;
        pack.height = height;
        pack.width = width;
        pack.tex_data = data;
        intf->set_sampler2D_callback(register_sampler);
        data_t input_uniform_data = (data_t){.sampler2D_var = 0};
        intf->input_uniform_dispatch(myshader.uniform_map[string("texture_diffuse1")], input_uniform_data);
        vector<pixel_color_t> framebuf;
        framebuf.resize(width*height);
        for (int x = 0; x < width; x++){
            for (int y = 0; y<height; y++){
                map<string, data_t> input;
                float tex_x = (float)x / (float)width;
                float tex_y = (float)y / (float)height;
                input.emplace("texcoord", (data_t){.vec2_var = glm::vec2(tex_x, tex_y)});
                intf->input_port(input);
                intf->glsl_main();
                data_t gl_frag_out;
                intf->get_inner_variable(INNER_GL_FRAGCOLOR, gl_frag_out);
                int idx = GET_INDEX(x,y,width);
                framebuf[idx].R = gl_frag_out.vec4_var.x*255.0f;
                framebuf[idx].G = gl_frag_out.vec4_var.y*255.0f;
                framebuf[idx].B = gl_frag_out.vec4_var.z*255.0f;
            }
        }
        cv::Mat frame(height, width, CV_32FC3, &(framebuf[0]));
        frame.convertTo(frame, CV_8UC3, 1.0f);
        cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
        int key = 0;
        while (key != 27){
            cv::imshow("test sampler", frame);
            key = cv::waitKey(1);
        }
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    return 0;
}