#include <iostream>
#include <vector>
#include "../include/gl/gl.h"
#include "../include/glv/glv.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "OBJ_Loader.h"
#include "header_assimp/shader.h"
#include "header_assimp/camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

const int WIDTH = 800, HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

glm::vec3 lightPos(20.0f, 20.0f, 20.0f);

void mouse_callback(GLVStream* stream, float xpos, float ypos);
void scroll_callback(GLVStream* stream, float xoffset, float yoffset);
void processInput(GLVStream* stream);

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

int load_vertices(std::vector<float> & vertices){
    objl::Loader Loader;
    int ret =0;
    int num_triangles = 0;
    bool loadout = Loader.LoadFile("../resources/spot/spot_triangulated_good.obj");
    for(auto mesh:Loader.LoadedMeshes)
    {
        for(int i=0;i<mesh.Vertices.size();i+=3)
        {
            for(int j=0;j<3;j++)
            {
                vertices.push_back(mesh.Vertices[i+j].Position.X);
                vertices.push_back(mesh.Vertices[i+j].Position.Y);
                vertices.push_back(mesh.Vertices[i+j].Position.Z);
                vertices.push_back(mesh.Vertices[i+j].TextureCoordinate.X);
                vertices.push_back(mesh.Vertices[i + j].TextureCoordinate.Y);
                vertices.push_back(mesh.Vertices[i + j].Normal.X);
                vertices.push_back(mesh.Vertices[i + j].Normal.Y);
                vertices.push_back(mesh.Vertices[i + j].Normal.Z);
                ret+=3;
            }
            num_triangles ++;
        }
    }
    std::cout<<"num triangles: "<<num_triangles<<std::endl;
    return ret;
}


static void testDrawCowWindow(){

    if (!glvInit())
    {
        std::cout << "glv Init failed\n";
        return;
    }
    GLVStream *window = glvCreateStream(WIDTH, HEIGHT, "cow", GLV_STREAM_WINDOW);

    glvSetCursorPosCallback(window, mouse_callback);
    glvSetScrollCallback(window, scroll_callback);

    glEnable(GL_DEPTH_TEST);

    // load model
    std::vector<float> vertices_data;
    int vertex_num;
    vertex_num = load_vertices(vertices_data);

    // create shader
    Shader myshader("../shader/cow_vert.glsl", "../shader/cow_frag.glsl");

    // Gen
    unsigned int VBO, VAO, texture1;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenTextures(1, &texture1);

    // load texture data
    glBindTexture(GL_TEXTURE_2D, texture1);
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load("../resources/spot/spot_texture.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    // Bind
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices_data.size()*sizeof(float), &vertices_data[0], GL_STATIC_DRAW);

    // VAO config
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

    // activate VAO attribs
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // activate shader
    myshader.use();
    myshader.setInt("texture_diffuse1", 0);
    myshader.setVec3("lightPos", lightPos);
    myshader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    
    float angle = 20.0f;
    int frame_cnt = 0;
    float time_interval = 0.0f;
    std::vector<float> fps_data;
    while (!glvWindowShouldClose(window))
    {
        float currentFrame = glvGetTime();
        deltaTime = currentFrame - lastFrame;
        // std::cout<<deltaTime<<std::endl;
        lastFrame = currentFrame;

        processInput(window);

        glBindVertexArray(VAO);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

        myshader.use();
        glm::mat4 model(1.0f);
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        myshader.setMat4("projection", projection);
        glm::mat4 view = camera.GetViewMatrix();
        myshader.setMat4("view", view);
        // model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        // model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.2f));
        myshader.setMat4("model", model);
        myshader.setMat4("inv_model", glm::transpose(glm::inverse(model)));
        myshader.setVec3("viewPos", camera.Position);

        angle += 2.0f;

        glDrawArrays(GL_TRIANGLES, 0, vertex_num);
        glvWriteStream(window);

        frame_cnt ++;
        time_interval += deltaTime;
        if (frame_cnt % 100 == 0){
            fps_data.push_back((100.0f)/time_interval);
            time_interval = 0.0f;
            frame_cnt = 0;
        }
    }

    glvTerminate(); 
    for (int i=0; i<fps_data.size(); i++){
        printf("fps: %f \n", fps_data[i]);
    }
}

void processInput(GLVStream* stream)
{
    if (glvGetKey(stream, GLV_KEY_ESCAPE) == GLV_PRESS){
        glvSetWindowShouldClose(stream, true);
    }
    if (glvGetKey(stream, GLV_KEY_W) == GLV_PRESS){
        // std::cout<<"forward"<<std::endl;
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glvGetKey(stream, GLV_KEY_S) == GLV_PRESS){
        // std::cout<<"back"<<std::endl;
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glvGetKey(stream, GLV_KEY_A) == GLV_PRESS){
        // std::cout<<"left"<<std::endl;
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glvGetKey(stream, GLV_KEY_D) == GLV_PRESS){
        // std::cout<<"right"<<std::endl;
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}


void mouse_callback(GLVStream* stream, float xpos, float ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLVStream* stream, float xoffset, float yoffset)
{
    std::cout<<yoffset<<std::endl;
    camera.ProcessMouseScroll(yoffset);
}

int main(){
    testDrawCowWindow();
    return 0;
}