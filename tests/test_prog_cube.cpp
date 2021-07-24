#include "../include/gl/gl.h"
#include "../include/glv/glv.h"
#include "OBJ_Loader.h"
#include "header/model.h"
#include "header/shader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <benchmark/benchmark.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

const int WIDTH = 800, HEIGHT = 600;
const float NEAR = 0.1f, FAR = 50.0f;

static void testProgCube(benchmark::State& state)
{
    if (!glvInit()) {
        std::cout << "glv Init failed\n";
        return;
    }

    GLVStream* window = glvCreateStream(WIDTH, HEIGHT, "cube test", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);

    glm::mat4 modelMatrix(1.0f);
    glm::mat4 viewMatrix(1.0f);
    glm::mat4 projectionMatrix(1.0f);
    glm::vec3 eyepos(0.0f, 0.0f, 5.0f);
    glm::vec3 front(0.0f, 0.0f, -1.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(0.0f), glm::vec3(1.0f, 1.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));

    viewMatrix = glm::lookAt(eyepos, eyepos + front, up);
    projectionMatrix = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, NEAR, FAR);

    Model model("../resources/basic/cube.obj");

    Shader shader("../shader/cube_vert.glsl", "../shader/cube_frag.glsl");
    shader.use();
    shader.setMat4("model", modelMatrix);
    shader.setMat4("view", viewMatrix);
    shader.setMat4("projection", projectionMatrix);
    float angle = 0.0f;
    // Perform setup here
    for (auto _ : state) {

        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(angle++), glm::vec3(1.0f, 1.0f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        shader.setMat4("model", modelMatrix);
        shader.setMat4("view", viewMatrix);
        shader.setMat4("projection", projectionMatrix);
        model.draw();
        // glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        // glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        glvWriteStream(window);
    }

    glvTerminate();
}

// Register the function as a benchmark
// Benchmark               Time                 CPU            Iterations
BENCHMARK(testProgCube)->Iterations(1000);
// Run the benchmark
BENCHMARK_MAIN();