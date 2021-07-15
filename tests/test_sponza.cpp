#include "../include/gl/gl.h"
#include "../include/glv/glv.h"
#include "OBJ_Loader.h"
#include "header/model.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <benchmark/benchmark.h>
#include <iostream>
#include <vector>

const int WIDTH = 800, HEIGHT = 600;

static void testSponza(benchmark::State& state)
{
    if (!glvInit()) {
        std::cout << "glv Init failed\n";
        return;
    }

    GLVStream* window = glvCreateStream(WIDTH, HEIGHT, "sponza atrium", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);

    Model model("../resources/sponza/models/sponza.obj");

    // Perform setup here
    for (auto _ : state) {

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        model.draw();
        // glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        // glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        glvWriteStream(window);
    }

    glvTerminate();
}

// Register the function as a benchmark
// Benchmark               Time                 CPU            Iterations
BENCHMARK(testSponza)->Iterations(1000);
// Run the benchmark
BENCHMARK_MAIN();