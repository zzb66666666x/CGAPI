#include "../include/gl/gl.h"
#include "../include/glv/glv.h"
#include "header/shader.h"
#include "header/model.h"
#include "OBJ_Loader.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

const int WIDTH = 800, HEIGHT = 600;
const float ZNEAR = 0.1f, ZFAR = 1000.0f;

static void testStandfordBunny()
{

    if (!glvInit()) {
        std::cout << "glv Init failed\n";
        return;
    }

    GLVStream* window = glvCreateStream(WIDTH, HEIGHT, "bunny_test", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);

    // Initialize Loader
    objl::Loader Loader;

    bool loadout = Loader.LoadFile("../resources/bunny/bunny.obj");

    if (!loadout) {
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
    printf("vertices size: %u\n", vertices.size() / 6);
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    // 激活顶点属性0
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
    float angle = 0.0f;
    glm::mat4 model(1.0f), view(1.0f), projection(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    // model = glm::scale(model, glm::vec3(0.9f, 0.9f, 0.9f));
    model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));

    glm::vec3 eyepos(0.0f, 0.0f, 5.0f);
    glm::vec3 front(0.0f, 0.0f, -1.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    view = glm::lookAt(eyepos, eyepos + front, up);
    projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, ZNEAR, ZFAR);

    Shader shader("../shader/bunny_vert.glsl", "../shader/bunny_frag.glsl");
    shader.use();
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    double fps = 0.0;
    int frameCount = 0;
    auto lastTime = std::chrono::system_clock::now();
    auto curTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
    double duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    while (!glvWindowShouldClose(window)) {

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        angle += 1.0f;
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        // model = glm::scale(model, glm::vec3(0.9f, 0.9f, 0.9f));
        model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));

        shader.use();

        shader.setMat4("model", model);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        glvWriteStream(window);

        curTime = std::chrono::system_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
        duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
        if (duration_s > 5) //5秒之后开始统计FPS
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
    testStandfordBunny();
    return 0;
}
