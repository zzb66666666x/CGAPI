#include <pthread.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "render.h"
#include "glcontext.h"
#include "../../include/gl/common.h"

#define GET_PIPELINE(P) glPipeline *P = &(glapi_ctx->pipeline)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define GET_INDEX(x, y, width, height) ((height - 1 - y) * width + x)

// static helpers
static glm::vec3 interpolate(float alpha, float beta, float gamma, glm::vec3 &vert1, glm::vec3 &vert2, glm::vec3 &vert3, float weight);
static void default_vertex_shader();
static void default_vertex_shader();
static void set_transform_matrices();
// gl inner variables
glm::vec4 gl_Position;
glm::vec3 gl_VertexColor;
// vertex shader input
int layouts[] = {0, 1};
int layout_cnt = 2;
glm::vec3 input_Pos;
glm::vec3 vert_Color;
// fragment shader input
glm::vec3 diffuse_Color;
glm::vec3 frag_Pos;
// fragment shader output, pixel value
glm::vec3 frag_Color;
// MVP matrices
glm::mat4 model; 
glm::mat4 view;
glm::mat4 projection;

// for test
float angle = 0.0f;

static void default_vertex_shader(){
    frag_Pos = glm::vec3(model * glm::vec4(input_Pos.x, input_Pos.y, input_Pos.z, 1.0f));
    // glm::vec4 test = view*glm::vec4(frag_Pos.x, frag_Pos.y, frag_Pos.z, 1.0f);
    gl_Position = projection * view * glm::vec4(frag_Pos.x, frag_Pos.y, frag_Pos.z, 1.0f);
    gl_VertexColor = vert_Color;
}

static void default_fragment_shader(){
    frag_Color = diffuse_Color;
}

void set_transform_matrices(){
    GET_CURRENT_CONTEXT(C);
    model         = glm::mat4(1.0f); 
    view          = glm::mat4(1.0f);
    projection    = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.6f, 1.0f, 0.8f));
    glm::vec3 eyepos(0.0f,0.0f,5.0f);
    glm::vec3 front(0.0f, 0.0f, -1.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    view  = glm::lookAt(eyepos, eyepos+front, up);
    projection = glm::perspective(glm::radians(45.0f), (float)(C->width) / (float)(C->height), C->znear, C->zfar);
    // print
    // for (int i=0; i<4; i++){
    //     for (int j=0; j<4; j++){
    //         std::cout<<view[j][i]<<"     ";
    //     }
    //     std::cout<<"\n";
    // }
}

// geometry processing
void process_geometry()
{
    // 1. parse vbo and do vertex shading
    GET_PIPELINE(P);
    GET_CURRENT_CONTEXT(C);
    vertex_attrib_t* vattrib_data = (vertex_attrib_t*) P->vao_ptr->getDataPtr();
    char* vbuf_data = (char *) P->vbo_ptr->getDataPtr();
    int vbuf_size = P->vbo_ptr->getSize();
    int vertex_num = P->vertex_num;
    // 2. check if the config is activated
    for (int i=0; i<layout_cnt; i++){
        if (!vattrib_data[layouts[i]].activated)
            throw std::runtime_error("using inactive layout\n");
    }
    // 3. parse vertex data
    vbuf_data += (P->first_vertex)*(vattrib_data[0].stride); // use the first stride to determine start of vertex buffer
    // float * temp = (float*) vbuf_data;
    // for (int i=0; i<18; i++){
    //     std::cout<<temp[i]<<std::endl;
    // }
    
    int cnt = 0;
    char* buf;
    int indices[] = {0,0};
    glm::vec3* vec3_ptr;
    int flag = 1;
    Triangle* tri = new Triangle();
    angle = angle + 10.0f;
    while (cnt<vertex_num){
        if (!flag){
            delete tri;
            break;
        }
        for (int i=0; i<layout_cnt; i++){
            vec3_ptr = nullptr;
            switch(layouts[i]){
                case 0:
                    vec3_ptr = &input_Pos;
                    break;
                case 1:
                    vec3_ptr = &vert_Color;
                    break;
                default:
                    throw std::runtime_error("invalid layout\n");
            }
            vertex_attrib_t& config = vattrib_data[layouts[i]];
            buf = vbuf_data + (indices[i] + (int)((long long)config.pointer));
            switch(config.type){
                case GL_FLOAT:
                    switch(config.size){
                        case 3:
                            (*vec3_ptr).x = *(float*)(buf+0);
                            (*vec3_ptr).y = *(float*)(buf+sizeof(float)*1);
                            (*vec3_ptr).z = *(float*)(buf+sizeof(float)*2);
                            // std::cout<<"extracted float: "<<(*vec3_ptr).x<<" "<< (*vec3_ptr).y<<" "<< (*vec3_ptr).z<<std::endl;
                            break;
                        default: 
                            throw std::runtime_error("not supported size\n");
                    }
                    break;
                default: 
                    throw std::runtime_error("not supported type\n");
            }
            indices[i] += config.stride;
            if (indices[i] >= vbuf_size){
                flag = 0;
            }
        }
        // 4. vertex shading
        set_transform_matrices();
        default_vertex_shader();

        gl_Position.x /= gl_Position.w;
        gl_Position.y /= gl_Position.w;
        gl_Position.z /= gl_Position.w;
        // 5. view port transformation
        gl_Position.x = 0.5 * C->width * (gl_Position.x + 1.0);
        gl_Position.y = 0.5 * C->height * (gl_Position.y + 1.0);  
        // [-1,1] to [0,1]
        gl_Position.z = gl_Position.z * 0.5 + 0.5;
        // 6. assemble triangle
        tri->screen_pos[cnt%3] = gl_Position;
        tri->color[cnt%3] = gl_VertexColor;
        tri->frag_shading_pos[cnt%3] = frag_Pos;
        cnt += 1;
        if (cnt>0 && cnt % 3 == 0){
            P->triangle_stream.push(tri);
            tri = nullptr;
            if (cnt+3<=vertex_num)
                tri = new Triangle();
        }
    }
}

inline static glm::vec3 interpolate(float alpha, float beta, float gamma, glm::vec3 &vert1, glm::vec3 &vert2, glm::vec3 &vert3, float weight)
{
    return (alpha * vert1 + beta * vert2 + gamma * vert3) / weight;
}

void rasterize()
{
    GET_CURRENT_CONTEXT(C);
    std::queue<Triangle *> &triangle_stream = C->pipeline.triangle_stream;
    int width = C->width, height = C->height;
    std::vector<Pixel> &pixel_tasks = C->pipeline.pixel_tasks;
    
    while (!triangle_stream.empty())
    {
        Triangle *t = triangle_stream.front();
        triangle_stream.pop();
        glm::vec4 *screen_pos = t->screen_pos;
        int minx, maxx, miny, maxy, x, y;
        minx = MIN(screen_pos[0].x, MIN(screen_pos[1].x, screen_pos[2].x));
        miny = MIN(screen_pos[0].y, MIN(screen_pos[1].y, screen_pos[2].y));
        maxx = MAX(screen_pos[0].x, MAX(screen_pos[1].x, screen_pos[2].x));
        maxy = MAX(screen_pos[0].y, MAX(screen_pos[1].y, screen_pos[2].y));

        float *zbuf = (float *)C->zbuf->getDataPtr();
        // AABB algorithm
        for (y = miny; y <= maxy; ++y)
        {
            for (x = minx; x <= maxx; ++x)
            {
                int index = GET_INDEX(x, y, width, height);
                if (!t->inside(x + 0.5f, y + 0.5f))
                    continue;

                // alpha beta gamma
                glm::vec3 coef = t->computeBarycentric2D(x + 0.5f, y + 0.5f);
                // perspective correction
                float Z_viewspace = 1.0/(coef[0]/screen_pos[0].w + coef[1]/screen_pos[1].w + coef[2]/screen_pos[2].w);
                float alpha = coef[0]*Z_viewspace/screen_pos[0].w;
                float beta = coef[1]*Z_viewspace/screen_pos[1].w;
                float gamma = coef[2]*Z_viewspace/screen_pos[2].w;
               
                if (!C->use_z_test)
                {
                    pixel_tasks[index].write = true;
                    pixel_tasks[index].vertexColor = interpolate(alpha, beta, gamma, t->color[0], t->color[1], t->color[2], 1);
                }
                else{
                    // zp: z value after interpolation
                    float zp = alpha*screen_pos[0].z + beta*screen_pos[1].z + gamma*screen_pos[2].z;
                    if (zp < zbuf[index]){
                        zbuf[index] = zp;
                        pixel_tasks[index].write = true;
                        pixel_tasks[index].vertexColor = interpolate(alpha, beta, gamma, t->color[0], t->color[1], t->color[2], 1);
                    }
                }
            }
        }

        delete t;
    }
}

void process_pixel()
{
    GET_CURRENT_CONTEXT(C);
    std::vector<Pixel> &pixel_tasks = C->pipeline.pixel_tasks;
    color_t* frame_buf = (color_t*)C->framebuf->getDataPtr();
    for(int i = 0,len = pixel_tasks.size();i < len;++i)
    {
        if(pixel_tasks[i].write){
            diffuse_Color = pixel_tasks[i].vertexColor;
            default_fragment_shader();
            frame_buf[i].R = frag_Color.x * 255.0f;
            frame_buf[i].G = frag_Color.y * 255.0f;
            frame_buf[i].B = frag_Color.z * 255.0f;
            pixel_tasks[i].write = false;
        }
    }
}

void *_thr_process_vertex(void *thread_id)
{
    GET_CURRENT_CONTEXT(C);
    std::vector<Pixel> &pixel_tasks = C->pipeline.pixel_tasks;
    color_t* frame_buf = (color_t*)C->framebuf->getDataPtr();
    for(int i = 0,len = pixel_tasks.size();i < len;++i)
    {
        if(pixel_tasks[i].write){
            diffuse_Color = pixel_tasks[i].vertexColor;
            default_fragment_shader();

            frame_buf[i].R = frag_Color.x * 255.0f;
            frame_buf[i].G = frag_Color.y * 255.0f;
            frame_buf[i].B = frag_Color.z * 255.0f;
        }
    }
}

void *_thr_rasterize(void *thread_id)
{

    return nullptr;
}
