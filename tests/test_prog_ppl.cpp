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
const float NEAR = 0.1f, FAR = 500.0f;

static void testProgPipeline(benchmark::State& state)
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
    modelMatrix = glm::rotate(modelMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 0.1f, 0.1f));
    glm::vec3 eyepos(0.0f, 0.0f, 5.0f);
    glm::vec3 front(0.0f, 0.0f, -1.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    viewMatrix = glm::lookAt(eyepos, eyepos + front, up);
    projectionMatrix = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, NEAR, FAR);

    Model model("../resources/sponza/models/sponza.obj");

    Shader shader("../shader/sponza_vert.glsl", "../shader/sponza_frag.glsl");
    shader.setMat4("modelView", modelMatrix * modelMatrix);
    shader.setMat4("projection", projectionMatrix);
    shader.setMat4("inv_model", glm::transpose(glm::inverse(modelMatrix)));
    // Perform setup here
    for (auto _ : state) {

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        model.draw();
        // glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        // glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        glvWriteStream(window);
    }

    glvTerminate();
}

// Register the function as a benchmark
// Benchmark               Time                 CPU            Iterations
BENCHMARK(testProgPipeline)->Iterations(100);
// Run the benchmark
BENCHMARK_MAIN();