#include "../include/gl/gl.h"
#include "../include/glv/glv.h"
#include "header_assimp/model.h"
#include "header_assimp/shader.h"
#include <benchmark/benchmark.h>
#define GLM_FORCE_AVX2
#define GLM_FORCE_INLINE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <chrono>

const int WIDTH = 800, HEIGHT = 600;
const float NEAR = 0.1f, FAR = 500.0f;

glm::vec3 lightPos(0.0f, 0.0f, -80.0f);
// static void testProgSponza(benchmark::State& state)
// {
//     if (!glvInit()) {
//         std::cout << "glv Init failed\n";
//         return;
//     }

//     GLVStream* window = glvCreateStream(WIDTH, HEIGHT, "sponza atrium", GLV_STREAM_WINDOW);
//     glEnable(GL_DEPTH_TEST);

//     glm::mat4 modelMatrix(1.0f);
//     glm::mat4 viewMatrix(1.0f);
//     glm::mat4 projectionMatrix(1.0f);
//     modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -30.0f, -80.0f));
//     // modelMatrix = glm::translate(modelMatrix, glm::vec3(20.0f, -30.0f, -80.0f));
//     modelMatrix = glm::rotate(modelMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
//     modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 0.1f, 0.1f));
//     glm::vec3 eyepos(0.0f, 0.0f, 5.0f);
//     glm::vec3 front(0.0f, 0.0f, -1.0f);
//     glm::vec3 up(0.0f, 1.0f, 0.0f);
//     viewMatrix = glm::lookAt(eyepos, eyepos + front, up);
//     projectionMatrix = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, NEAR, FAR);

//     Model model("../resources/sponza/sponza.obj");
//     Model light("../resources/basic/cube.obj");

//     Shader shader("../shader/sponza_vert.glsl", "../shader/sponza_frag.glsl");
//     shader.use();
//     shader.setMat4("model", modelMatrix);
//     shader.setMat4("view", viewMatrix);
//     shader.setMat4("projection", projectionMatrix);
//     shader.setMat4("inv_model", glm::transpose(glm::inverse(modelMatrix)));
//     shader.setVec3("lightPos", lightPos);
//     shader.setVec3("viewPos", eyepos);
//     shader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
//     // shader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));

//     // Perform setup here
//     for (auto _ : state) {
//         if (glvWindowShouldClose(window)) {
//             break;
//         }

//         glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//         shader.use();
//         model.Draw(shader);

//         glvWriteStream(window);
//     }

//     glvTerminate();
// }

// // Register the function as a benchmark
// // Benchmark               Time                 CPU            Iterations
// BENCHMARK(testProgSponza)->Iterations(100);
// // Run the benchmark
// BENCHMARK_MAIN();

int main(void){
    if (!glvInit()) {
        std::cout << "glv Init failed\n";
        return 0;
    }

    GLVStream* window = glvCreateStream(WIDTH, HEIGHT, "sponza atrium", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);

    glm::mat4 modelMatrix(1.0f);
    glm::mat4 viewMatrix(1.0f);
    glm::mat4 projectionMatrix(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -30.0f, -80.0f));
    // modelMatrix = glm::translate(modelMatrix, glm::vec3(20.0f, -30.0f, -80.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 0.1f, 0.1f));
    glm::vec3 eyepos(0.0f, 0.0f, 5.0f);
    glm::vec3 front(0.0f, 0.0f, -1.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    viewMatrix = glm::lookAt(eyepos, eyepos + front, up);
    projectionMatrix = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, NEAR, FAR);

    Model model("../resources/sponza/sponza.obj");
    Model light("../resources/basic/cube.obj");

    Shader shader("../shader/sponza_vert.glsl", "../shader/sponza_frag.glsl");
    shader.use();
    shader.setMat4("model", modelMatrix);
    shader.setMat4("view", viewMatrix);
    shader.setMat4("projection", projectionMatrix);
    shader.setMat4("inv_model", glm::transpose(glm::inverse(modelMatrix)));
    shader.setVec3("lightPos", lightPos);
    shader.setVec3("viewPos", eyepos);
    shader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    // shader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));

    double fps = 0.0;
    int frameCount = 0;
    auto lastTime = std::chrono::system_clock::now();
    auto curTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(curTime - lastTime);
    double duration_s = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;

    // Perform setup here
    for (;;) {
        if (glvWindowShouldClose(window)) {
            break;
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        model.Draw(shader);

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

    return 0;
}