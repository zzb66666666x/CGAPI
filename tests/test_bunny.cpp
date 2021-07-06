#include "../include/gl/gl.h"
#include "../include/glv/glv.h"
#include <iostream>
#include <vector>
#include <chrono>
#include "OBJ_Loader.h"

using namespace std;

const int WIDTH = 800, HEIGHT = 600;

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

        // for (int j = 0; j < curMesh.Vertices.size(); j++) {
        //     vertices.push_back(curMesh.Vertices[j].Position.X);
        //     vertices.push_back(curMesh.Vertices[j].Position.Y);
        //     vertices.push_back(curMesh.Vertices[j].Position.Z);
        //     vertices.push_back(0.3f);
        //     vertices.push_back(0.4f);
        //     vertices.push_back(0.8f);
        //     vertices.push_back(curMesh.Vertices[j].Normal.X);
        //     vertices.push_back(curMesh.Vertices[j].Normal.Y);
        //     vertices.push_back(curMesh.Vertices[j].Normal.Z);
        // }

        // Go through every 3rd index and print the
        //	triangle that these indices represent
        for (int j = 0; j < curMesh.Indices.size(); j += 3) {
            // indices.push_back(curMesh.Indices[j]);
            // indices.push_back(curMesh.Indices[j + 1]);
            // indices.push_back(curMesh.Indices[j + 2]);
            unsigned int tri_indices [] = {
                curMesh.Indices[j],
                curMesh.Indices[j+1],
                curMesh.Indices[j+2]
            };
            for (int i = 0; i<3; i++){
                int index = tri_indices[i];
                vertices.push_back(curMesh.Vertices[index].Position.X);
                vertices.push_back(curMesh.Vertices[index].Position.Y);
                vertices.push_back(curMesh.Vertices[index].Position.Z);
                vertices.push_back(0.3f);
                vertices.push_back(0.4f);
                vertices.push_back(0.8f);
            }
        }
    }
    // printf("indices size: %u\n", indices.size());
    // printf("triangle size: %u\n", indices.size() / 3);
    printf("triangles: %d \n", vertices.size()/6/3);
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    // glGenBuffers(1, &EBO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
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

        // glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());

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

int main(){
    testStandfordBunny();
    return 0;
}
