#include <iostream>
#include <vector>
#include "../include/gl/gl.h"
#include "../include/glv/glv.h"
#include <chrono>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "OBJ_Loader.h"

using namespace std;

const int WIDTH = 800, HEIGHT = 600;

int load_vertices(std::vector<float> & vertices){
    objl::Loader Loader;
    int ret =0;
    int num_triangles = 0;
    bool loadout = Loader.LoadFile("../resources/spot/spot_triangulated_good.obj");
    for(auto mesh:Loader.LoadedMeshes)
    {
        for(int i=0;i<mesh.Vertices.size();i+=3)
        {
            for(int j=0;j<3;j++)
            {
                vertices.push_back(mesh.Vertices[i+j].Position.X);
                vertices.push_back(mesh.Vertices[i+j].Position.Y);
                vertices.push_back(mesh.Vertices[i+j].Position.Z);
                vertices.push_back(0.2f);
                vertices.push_back(0.3f);
                vertices.push_back(0.8f);
                // vertices.push_back(mesh.Vertices[i+j].Normal.X);
                // vertices.push_back(mesh.Vertices[i+j].Normal.Y);
                // vertices.push_back(mesh.Vertices[i+j].Normal.Z);
                vertices.push_back(mesh.Vertices[i+j].TextureCoordinate.X);
                vertices.push_back(mesh.Vertices[i+j].TextureCoordinate.Y);
                ret+=3;
            }
            num_triangles ++;
        }
    }
    std::cout<<"num triangles: "<<num_triangles<<std::endl;
    return ret;
}


static void testDrawCowWindow(){
    int frame_count = 0;

    if (!glvInit())
    {
        std::cout << "glv Init failed\n";
        return;
    }
    GLVStream *window = glvCreateStream(WIDTH, HEIGHT, "cow", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);

    // load model
    std::vector<float> vertices_data;
    int vertex_num;
    vertex_num = load_vertices(vertices_data);

    // Gen
    unsigned int VBO, VAO, texture1;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenTextures(1, &texture1);

    // load texture data
    glBindTexture(GL_TEXTURE_2D, texture1);
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load("../resources/spot/spot_texture.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    // Bind
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices_data.size()*sizeof(float), &vertices_data[0], GL_STATIC_DRAW);

    // VAO config
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));

    // activate VAO attribs
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // DWORD begin;
    double fps = 0.0;
    int frameCount = 0;
    auto lastTime = std::chrono::system_clock::now();
    auto curTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
    double duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    while (1)
    {
        // begin = GetTickCount();
        glBindVertexArray(VAO);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

        glDrawArrays(GL_TRIANGLES, 0, vertex_num);
        glvWriteStream(window);
        // std::cout << "fps:" << 1000.0 / (GetTickCount() - begin) << std::endl;
        curTime = std::chrono::system_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
        duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
        if (duration_s > 2)//2秒之后开始统计FPS
	    {
            fps = frameCount / duration_s;
            frameCount = 0;
            lastTime = curTime;
            std::cout<<"fps: "<<fps<<"\n";
        }

        ++frameCount;
    }

    glvTerminate(); 
}

int main(){
    testDrawCowWindow();
    return 0;
}