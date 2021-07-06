#include "../include/gl/gl.h"
#include "../include/glv/glv.h"
#include "OBJ_Loader.h"
#include "stb_image.h"
#include <benchmark/benchmark.h>
#include <iostream>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION

const int WIDTH = 800, HEIGHT = 600;


///////////////////////////////////// test for drawing cow ////////////////////////////////////////

static void loadCowModel(std::vector<unsigned int> &indices, std::vector<float> &vertices){
    // Initialize Loader
    objl::Loader Loader;

    bool loadout = Loader.LoadFile("../resources/spot/spot_triangulated_good.obj");

    if (!loadout) {
        printf("spot failed to load");
        return;
    }

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

            vertices.push_back(curMesh.Vertices[j].TextureCoordinate.X);
            vertices.push_back(curMesh.Vertices[j].TextureCoordinate.Y);

            // vertices.push_back(curMesh.Vertices[j].Normal.X);
            // vertices.push_back(curMesh.Vertices[j].Normal.Y);
            // vertices.push_back(curMesh.Vertices[j].Normal.Z);
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

    // printf("vertices size: %u\n", vertices.size() / 9);
    // printf("indices size: %u\n", indices.size());
    // printf("triangle size: %u\n", indices.size() / 3);
}

static void testCow(benchmark::State& state)
{
    if (!glvInit()) {
        std::cout << "glv Init failed\n";
        return;
    }

    GLVStream* window = glvCreateStream(WIDTH, HEIGHT, "cow", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);

    std::vector<unsigned int> indices;
    std::vector<float> vertices;

    loadCowModel(indices, vertices);

    // Gen
    unsigned int VBO, VAO, EBO, texture1;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
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
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // VAO config
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    // activate VAO attribs
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Perform setup here
    for (auto _ : state) {
        glBindVertexArray(VAO);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        // glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        glvWriteStream(window);
    }


    glvTerminate();

}


// Register the function as a benchmark
// Benchmark               Time                 CPU            Iterations
BENCHMARK(testCow)->Iterations(1000);
BENCHMARK(testCow)->Iterations(5000);
// Run the benchmark
BENCHMARK_MAIN();