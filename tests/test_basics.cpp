#include <iostream>
#include <vector>
#include "../include/gl/gl.h"
#include "../include/glv/glv.h"
#include "shared_data.h"
#include <chrono>

using namespace std;

const int WIDTH = 800, HEIGHT = 600;

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


    if (!glvInit())
    {
        std::cout << "glv Init failed\n";
        return;
    }
    GLVStream *file = glvCreateStream(WIDTH, HEIGHT, "cube", GLV_STREAM_FILE);
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


int main()
{
    // testBasic();
    // testDrawNaiveImage();
    testDrawInWindow();
    return 0;
}