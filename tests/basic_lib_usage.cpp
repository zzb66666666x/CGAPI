#include <iostream>
#include <vector>
#include "../include/gl/gl.h"
#include "../include/glv/glv.h"
#include <chrono>
// #include <windows.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "OBJ_Loader.h"
#define GLM_FORCE_AVX2
#define GLM_FORCE_INLINE 
#include <glm/glm.hpp>

using namespace std;

const int WIDTH = 800, HEIGHT = 600;

float cubeVertices[] = {
     // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f};

float cubeVerticesNoTex[] = {
     // positions          // colors         
    -0.5f,  -0.5f,  -0.5f,  0.0f,   0.0f,   1.0f,
    0.5f,   -0.5f,  -0.5f,  1.0f,   0.0f,   1.0f,
    0.5f,   0.5f,   -0.5f,  1.0f,   1.0f,   1.0f,
    0.5f,   0.5f,   -0.5f,  1.0f,   1.0f,   1.0f,
    -0.5f,  0.5f,   -0.5f,  0.0f,   1.0f,   1.0f,
    -0.5f,  -0.5f,  -0.5f,  0.0f,   0.0f,   1.0f,

    -0.5f,  -0.5f,  0.5f,   0.0f,   0.0f,   1.0f,
    0.5f,   -0.5f,  0.5f,   1.0f,   0.0f,   1.0f,
    0.5f,   0.5f,   0.5f,   1.0f,   1.0f,   1.0f,
    0.5f,   0.5f,   0.5f,   1.0f,   1.0f,   1.0f,
    -0.5f,  0.5f,   0.5f,   0.0f,   1.0f,   1.0f,
    -0.5f,  -0.5f,  0.5f,   0.0f,   0.0f,   1.0f,

    -0.5f,  0.5f,   0.5f,   1.0f,   0.0f,   1.0f,
    -0.5f,  0.5f,   -0.5f,  1.0f,   1.0f,   1.0f,
    -0.5f,  -0.5f,  -0.5f,  0.0f,   1.0f,   1.0f,
    -0.5f,  -0.5f,  -0.5f,  0.0f,   1.0f,   1.0f,
    -0.5f,  -0.5f,  0.5f,   0.0f,   0.0f,   1.0f,
    -0.5f,  0.5f,   0.5f,   1.0f,   0.0f,   1.0f,

    0.5f,   0.5f,   0.5f,   1.0f,   0.0f,   1.0f,
    0.5f,   0.5f,   -0.5f,  1.0f,   1.0f,   1.0f,
    0.5f,   -0.5f,  -0.5f,  0.0f,   1.0f,   1.0f,
    0.5f,   -0.5f,  -0.5f,  0.0f,   1.0f,   1.0f,
    0.5f,   -0.5f,  0.5f,   0.0f,   0.0f,   1.0f,
    0.5f,   0.5f,   0.5f,   1.0f,   0.0f,   1.0f,

    -0.5f,  -0.5f,  -0.5f,  0.0f,   1.0f,   1.0f,
    0.5f,   -0.5f,  -0.5f,  1.0f,   1.0f,   1.0f,
    0.5f,   -0.5f,  0.5f,   1.0f,   0.0f,   1.0f,
    0.5f,   -0.5f,  0.5f,   1.0f,   0.0f,   1.0f,
    -0.5f,  -0.5f,  0.5f,   0.0f,   0.0f,   1.0f,
    -0.5f,  -0.5f,  -0.5f,  0.0f,   1.0f,   1.0f,

    -0.5f,  0.5f,   -0.5f,  0.0f,   1.0f,   1.0f,
    0.5f,   0.5f,   -0.5f,  1.0f,   1.0f,   1.0f,
    0.5f,   0.5f,   0.5f,   1.0f,   0.0f,   1.0f,
    0.5f,   0.5f,   0.5f,   1.0f,   0.0f,   1.0f,
    -0.5f,  0.5f,   0.5f,   0.0f,   0.0f,   1.0f,
    -0.5f,  0.5f,   -0.5f,  0.0f,   1.0f,   1.0f
};

int load_vertices(std::vector<float> & vertices){
    objl::Loader Loader;
    int ret =0;
    int num_triangles = 0;
    bool loadout = Loader.LoadFile("../resources/spot/spot_triangulated_good.obj");
    for(auto mesh:Loader.LoadedMeshes)
    {
        for (int i = 0; i < mesh.Vertices.size(); i += 3) {
            for(int j = 0;j < 3;j++)
            {
                // vertices.push_back(mesh.Vertices[i+j].Position.X);
                // vertices.push_back(mesh.Vertices[i+j].Position.Y);
                // vertices.push_back(mesh.Vertices[i+j].Position.Z);
                vertices.push_back(mesh.Vertices[mesh.Indices[i + j]].Position.X);
                vertices.push_back(mesh.Vertices[mesh.Indices[i + j]].Position.Y);
                vertices.push_back(mesh.Vertices[mesh.Indices[i + j]].Position.Z);
                vertices.push_back(0.5f);
                vertices.push_back(0.5f);
                vertices.push_back(0.5f);
                // vertices.push_back(mesh.Vertices[i+j].Normal.X);
                // vertices.push_back(mesh.Vertices[i+j].Normal.Y);
                // vertices.push_back(mesh.Vertices[i+j].Normal.Z);
                vertices.push_back(mesh.Vertices[mesh.Indices[i + j]].TextureCoordinate.X);
                vertices.push_back(mesh.Vertices[mesh.Indices[i + j]].TextureCoordinate.Y);
                // vertices.push_back(mesh.Vertices[i + j].TextureCoordinate.X);
                // vertices.push_back(mesh.Vertices[i + j].TextureCoordinate.Y);
                ret+=3;
            }
            num_triangles ++;
        }
    }
    std::cout<<"num triangles: "<<num_triangles<<std::endl;
    return ret;
}

static void testWheel(){
    if (!glvInit()) {
        std::cout << "glv Init failed\n";
        return;
    }

    GLVStream* window = glvCreateStream(WIDTH, HEIGHT, "wheel", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);

    // Initialize Loader
    objl::Loader Loader;

    bool loadout = Loader.LoadFile("../resources/wheel/wheel.obj");

    if (!loadout) {
        printf("wheel failed to load");
        return;
    }

    std::vector<unsigned int> indices;
    std::vector<float> vertices;
    // Go through each loaded mesh and out its contents
    for (int i = 0; i < Loader.LoadedMeshes.size(); i++) {
        // Copy one of the loaded meshes to be our current mesh
        objl::Mesh curMesh = Loader.LoadedMeshes[i];

        for (int j = 0; j < curMesh.Vertices.size(); ++j) {
            vertices.push_back(curMesh.Vertices[j].Position.X);
            vertices.push_back(curMesh.Vertices[j].Position.Y);
            vertices.push_back(curMesh.Vertices[j].Position.Z);

            vertices.push_back(0.3f);
            vertices.push_back(0.4f);
            vertices.push_back(0.8f);

            vertices.push_back(curMesh.Vertices[j].Normal.X);
            vertices.push_back(curMesh.Vertices[j].Normal.Y);
            vertices.push_back(curMesh.Vertices[j].Normal.Z);
        }

        // Go through every 3rd index and print the
        //	triangle that these indices represent
        for (int j = 0; j < curMesh.Indices.size(); j += 3) {
            indices.push_back(curMesh.Indices[j]);
            indices.push_back(curMesh.Indices[j + 1]);
            indices.push_back(curMesh.Indices[j + 2]);
            // file << "T" << j / 3 << ": " << curMesh.Indices[j] << ", " << curMesh.Indices[j + 1] << ", " << curMesh.Indices[j + 2] << "\n";
        }
    }

    printf("vertices size: %u\n", vertices.size() / 9);
    printf("indices size: %u\n", indices.size());
    printf("triangle size: %u\n", indices.size() / 3);

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // 将vertices数据复制到缓冲种
    // GL_STATIC_DRAW / GL_DYNAMIC_DRAW、GL_STREAM_DRAW
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // 解析缓冲数据
    // 顶点位置，顶点大小(vec3)，顶点类型，是否normalize，步长，偏移量
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));

    // 激活顶点属性0
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    double fps = 0.0;
    int frameCount = 0;
    auto lastTime = std::chrono::system_clock::now();
    auto curTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
    double duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    while (1) {

        glBindVertexArray(VAO);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        // glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        glvWriteStream(window);

        curTime = std::chrono::system_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
        duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
        if (duration_s > 2) //2秒之后开始统计FPS
        {
            fps = frameCount / duration_s;
            frameCount = 0;
            lastTime = curTime;
            std::cout << "fps: " << fps << "\n";
        }

        ++frameCount;
    }

    glvTerminate();
}

static void testStandfordBunny(){

    if (!glvInit()) {
        std::cout << "glv Init failed\n";
        return;
    }

    GLVStream* window = glvCreateStream(WIDTH, HEIGHT, "bunny_test", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);

    // Initialize Loader
    objl::Loader Loader;

    bool loadout = Loader.LoadFile("../resources/bunny/bunny.obj");

    if(!loadout){
        printf("bunny failed to load");
        return;
    }

    std::vector<unsigned int> indices;
    std::vector<float> vertices;
    // Go through each loaded mesh and out its contents
    for (int i = 0; i < Loader.LoadedMeshes.size(); i++) {
        // Copy one of the loaded meshes to be our current mesh
        objl::Mesh curMesh = Loader.LoadedMeshes[i];

        for (int j = 0; j < curMesh.Vertices.size(); j++) {
            vertices.push_back(curMesh.Vertices[j].Position.X);
            vertices.push_back(curMesh.Vertices[j].Position.Y);
            vertices.push_back(curMesh.Vertices[j].Position.Z);

            vertices.push_back(0.3f);
            vertices.push_back(0.4f);
            vertices.push_back(0.8f);

            vertices.push_back(curMesh.Vertices[j].Normal.X);
            vertices.push_back(curMesh.Vertices[j].Normal.Y);
            vertices.push_back(curMesh.Vertices[j].Normal.Z);
        }

        // Go through every 3rd index and print the
        //	triangle that these indices represent
        for (int j = 0; j < curMesh.Indices.size(); j += 3) {
            indices.push_back(curMesh.Indices[j]);
            indices.push_back(curMesh.Indices[j + 1]);
            indices.push_back(curMesh.Indices[j + 2]);
            // file << "T" << j / 3 << ": " << curMesh.Indices[j] << ", " << curMesh.Indices[j + 1] << ", " << curMesh.Indices[j + 2] << "\n";
        }
    }
    printf("vertices size: %u\n", vertices.size() / 9);
    printf("indices size: %u\n", indices.size());
    printf("triangle size: %u\n", indices.size() / 3);
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // 将vertices数据复制到缓冲种
    // GL_STATIC_DRAW / GL_DYNAMIC_DRAW、GL_STREAM_DRAW
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // 解析缓冲数据
    // 顶点位置，顶点大小(vec3)，顶点类型，是否normalize，步长，偏移量
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));

    // 激活顶点属性0
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    double fps = 0.0;
    int frameCount = 0;
    auto lastTime = std::chrono::system_clock::now();
    auto curTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
    double duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    while (1) {

        glBindVertexArray(VAO);
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        // glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        glvWriteStream(window);

        curTime = std::chrono::system_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
        duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
        if (duration_s > 2) //2秒之后开始统计FPS
        {
            fps = frameCount / duration_s;
            frameCount = 0;
            lastTime = curTime;
            std::cout << "fps: " << fps << "\n";
        }

        ++frameCount;
    }

    glvTerminate();
}

static void testEBO()
{
    if (!glvInit()) {
        std::cout << "glv Init failed\n";
        return;
    }
    GLVStream* window = glvCreateStream(WIDTH, HEIGHT, "ebo_test", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);
    
    // glDrawElements
    unsigned int indices[] = {
        // 注意索引从0开始!
        0, 1, 3, // 第一个三角形
        1, 2, 3 // 第二个三角形
    };
    float vertices[] = {
        0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // 右上角
        0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // 右下角
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, // 左下角
        -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f // 左上角
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // 将vertices数据复制到缓冲种
    // GL_STATIC_DRAW / GL_DYNAMIC_DRAW、GL_STREAM_DRAW
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // 解析缓冲数据
    // 顶点位置，顶点大小(vec3)，顶点类型，是否normalize，步长，偏移量
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    // 激活顶点属性0
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    double fps = 0.0;
    int frameCount = 0;
    auto lastTime = std::chrono::system_clock::now();
    auto curTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
    double duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    while (1) {

        glBindVertexArray(VAO);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // glDrawArrays(GL_TRIANGLES, 0, 36);

        glvWriteStream(window);

        curTime = std::chrono::system_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
        duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
        if (duration_s > 2) //2秒之后开始统计FPS
        {
            fps = frameCount / duration_s;
            frameCount = 0;
            lastTime = curTime;
            std::cout << "fps: " << fps << "\n";
        }

        ++frameCount;
    }

    glvTerminate();
}

static void testTexture()
{
    if (!glvInit())
    {
        std::cout << "glv Init failed\n";
        return;
    }
    GLVStream *window = glvCreateStream(WIDTH, HEIGHT, "texture_test", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);
    
    // Gen
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // VAO config
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));

    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));

    // activate VAO attribs
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // load and create a texture
    // -------------------------
    unsigned int texture1;
    // texture 1
    // ---------
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    // set the texture wrapping parameters
    // GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    // GL_LINEAR, GL_NEAREST
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    // The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
    unsigned char *data = stbi_load("../resources/textures/container.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    glBindVertexArray(0);

    double fps = 0.0;
    int frameCount = 0;
    auto lastTime = std::chrono::system_clock::now();
    auto curTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
    double duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    while (1)
    {
        
        glBindVertexArray(VAO);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        glvWriteStream(window);

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

static void testDrawInWindow()
{
    int frame_count = 0;

    if (!glvInit())
    {
        std::cout << "glv Init failed\n";
        return;
    }
    GLVStream *window = glvCreateStream(WIDTH, HEIGHT, "cube", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);

    // Gen
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerticesNoTex), cubeVerticesNoTex, GL_STATIC_DRAW);

    // VAO config
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));

    // activate VAO attribs
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    while (1)
    {
        glBindVertexArray(VAO);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, 36);

        glvWriteStream(window);
    }

    glvTerminate();
}
static void testDrawNaiveImage()
{
    // const char *vertexShaderSource = "#version 330 core\n"
    //                                  "layout (location = 0) in vec3 aPos;\n"
    //                                  "layout (location = 1) in vec3 color;\n"
    //                                  "out vec4 vertexColor;\n"
    //                                  "void main()\n"
    //                                  "{\n"
    //                                  "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    //                                  "   vertexColor = vec4(color.x, color.y, color.z, 1.0);\n"
    //                                  "}\0";
    // const char *fragmentShaderSource = "#version 330 core\n"
    //                                    "out vec4 FragColor;\n"
    //                                    "in vec4 vertexColor;\n"
    //                                    ""
    //                                    "void main()\n"
    //                                    "{\n"
    //                                    "    FragColor = vertexColor;\n"
    //                                    "}\n";

    if (!glvInit())
    {
        std::cout << "glv Init failed\n";
        return;
    }
    GLVStream *file = glvCreateStream(WIDTH, HEIGHT, "cube", GLV_STREAM_FILE);
    glEnable(GL_DEPTH_TEST);

    // unsigned int vertexShader;
    // vertexShader = glCreateShader(GL_VERTEX_SHADER);
    // glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    // glCompileShader(vertexShader);

    // unsigned int fragmentShader;
    // fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    // glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    // glCompileShader(fragmentShader);

    // unsigned int shaderProgram;

    // glAttachShader(shaderProgram, vertexShader);
    // glAttachShader(shaderProgram, fragmentShader);
    // glLinkProgram(shaderProgram);

    // glDeleteShader(vertexShader);
    // glDeleteShader(fragmentShader);

    // Gen
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerticesNoTex), cubeVerticesNoTex, GL_STATIC_DRAW);

    // VAO config
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));

    // activate VAO attribs
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // glEnable(GL_DEPTH_TEST);

    // render
    // glUseProgram(shaderProgram);
    glClearColor(0.2f, 0.3f, 0.7f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    if (glvWriteStream(file))
    {
        std::cout << "Renderer draws a naive image successfully!";
    }

    glvTerminate();
}

static void testBasic()
{
    glvInit();
    GLVStream *file = glvCreateStream(300, 300, "result", GLV_STREAM_FILE);
    cout << "finish creating file" << endl;
    glClearColor(0.2f, 0.5f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    cout << "finish clearing framebuffer, write to output image" << endl;
    if (glvWriteStream(file))
    {
        cout << "success!";
    }
    glvTerminate();
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
    // set the texture wrapping parameters
    // GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    // GL_LINEAR, GL_NEAREST
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    // activate VAO attribs
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);


    glBindVertexArray(0);
    double fps = 0.0;
    int frameCount = 0;
    auto lastTime = std::chrono::system_clock::now();
    auto curTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
    double duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    while (1)
    {
        glBindVertexArray(VAO);
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
        
        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

        glDrawArrays(GL_TRIANGLES, 0, vertex_num);
        glvWriteStream(window);

        curTime = std::chrono::system_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
        duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
        if (duration_s > 2) //2秒之后开始统计FPS
        {
            fps = frameCount / duration_s;
            frameCount = 0;
            lastTime = curTime;
            std::cout << "fps: " << fps << "\n";
        }

        ++frameCount;
    }

    glvTerminate(); 
}

int main()
{
    // testBasic();
    // testDrawNaiveImage();
    // testDrawInWindow();
    // testTexture();
    // testDrawCowWindow();
    // testEBO();
    // testDrawCowFile();
    // testDrawCowWindow();
    // testEBO();
    testStandfordBunny();
    // testWheel();
    return 0;
}