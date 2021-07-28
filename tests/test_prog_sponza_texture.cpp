#include "../include/gl/gl.h"
#include "../include/glv/glv.h"
#include "OBJ_Loader.h"
#include "header_assimp/model.h"
#include "header_assimp/shader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <benchmark/benchmark.h>
#define GLM_FORCE_AVX2
#define GLM_FORCE_INLINE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

const int WIDTH = 800, HEIGHT = 600;
const float NEAR = 0.1f, FAR = 500.0f;

glm::vec3 lightPos(0.0f, 0.0f, -80.0f);
static void testProgSponza(benchmark::State& state)
{
    if (!glvInit()) {
        std::cout << "glv Init failed\n";
        return;
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
    shader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));

    // Shader lightShader("../shader/light_vert.glsl", "../shader/light_frag.glsl");
    // lightShader.use();
    // glm::mat4 lightModel = glm::mat4(1.0f);
    // lightModel = glm::translate(lightModel, lightPos);
    // lightShader.setMat4("model", lightModel);
    // lightShader.setMat4("view", viewMatrix);
    // lightShader.setMat4("projection", projectionMatrix);

    // Perform setup here
    for (auto _ : state) {
        if (glvWindowShouldClose(window)) {
            break;
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        model.Draw(shader);

        // lightShader.use();
        // light.draw();

        glvWriteStream(window);
    }

    glvTerminate();
}

// Register the function as a benchmark
// Benchmark               Time                 CPU            Iterations
BENCHMARK(testProgSponza)->Iterations(100);
// Run the benchmark
BENCHMARK_MAIN();