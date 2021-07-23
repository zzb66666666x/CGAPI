#include <iostream>
#include <vector>
#include "../include/gl/gl.h"
#include "../include/glv/glv.h"
#include "shared_data.h"
#include <chrono>
#define GLM_FORCE_AVX2
#define GLM_FORCE_INLINE 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

const char * vertex_shader_src = \
"layout (location = 0) in vec3 aPos; \n"     
"layout (location = 1) in vec3 aVertColor; \n"
"out vec3 vertColor; \n"
"uniform mat4 projection; \n"    
"uniform mat4 view; \n"   
"uniform mat4 model; \n"
"void main() \n"
"{ \n"
"    gl_Position =  projection * view * model * vec4(aPos,1.0); \n"
"    vertColor = aVertColor; \n"
"} \n"
;

const char* frag_shader_src = \
"in vec3 vertColor; \n"
"void main() \n"
"{ \n"
"    gl_FragColor = vec4(vertColor, 1.0f); \n"
"} \n"
;

const int WIDTH = 800, HEIGHT = 600;
const float NEAR = 0.1f, FAR = 500.0f;

int main(){
    int frame_count = 0;

    if (!glvInit())
    {
        std::cout << "glv Init failed\n";
        return -1;
    }
    GLVStream *window = glvCreateStream(WIDTH, HEIGHT, "cube", GLV_STREAM_WINDOW);
    glEnable(GL_DEPTH_TEST);

    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertex_shader_src, NULL);
    glCompileShader(vertexShader);
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &frag_shader_src, NULL);
    glCompileShader(fragmentShader);
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

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

    glUseProgram(shaderProgram);

    float angle = 0.0f;

    while (!glvWindowShouldClose(window))
    {
        glBindVertexArray(VAO);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // pass in uniform variables
        glm::mat4 model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 eyepos(0.0f,0.0f,5.0f);
        glm::vec3 front(0.0f, 0.0f, -1.0f);
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::mat4 view  = glm::lookAt(eyepos, eyepos+front, up);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)WIDTH, NEAR, FAR);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

        glDrawArrays(GL_TRIANGLES, 0, 36);

        glvWriteStream(window);

        angle += 2.0f;
    }
    glvTerminate();
    return 0;
}