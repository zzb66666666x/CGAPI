#include <pthread.h>
#include "render.h"
#include "glcontext.h"
#include "../../include/gl/common.h"

#define GET_PIPELINE(P) glPipeline *P = &(glapi_ctx->pipeline)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define GET_INDEX(x, y, width, height) ((height - 1 - y) * width + x)

// geometry processing
void process_geometry()
{
    // 1. vertex shading
    GET_PIPELINE(P);
    glObject *vbo_ptr = P->vbo_ptr;
    // glObject* vao_ptr = P->vao_ptr;
    // vertex_attrib_t* va_data = (vertex_attrib_t*) vao_ptr->getDataPtr();
    vbo_ptr->getDataPtr();
    // 2. clipping

    // 3. screen mapping
    GET_CURRENT_CONTEXT(C);

    // naive impelment
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f};
    Triangle *triangle = new Triangle();
    for (int i = 0; i < 3; ++i)
    {
        triangle->screen_pos[i].x = 0.5 * C->width * (vertices[i * 6] + 1);
        triangle->screen_pos[i].y = 0.5 * C->height * (vertices[i * 6 + 1] + 1);
        triangle->screen_pos[i].z = vertices[i * 6 + 2];
        
        triangle->color[i].x = vertices[i * 6 + 3] * 255;
        triangle->color[i].y = vertices[i * 6 + 4] * 255;
        triangle->color[i].z = vertices[i * 6 + 5] * 255;
    }
    P->triangle_stream.push(triangle);
}

static glm::vec3 interpolate(float alpha, float beta, float gamma, glm::vec3 &vert1, glm::vec3 &vert2, glm::vec3 &vert3, float weight)
{
    return (alpha * vert1 + beta * vert2 + gamma * vert3) / weight;
}

void rasterize()
{
    GET_CURRENT_CONTEXT(C);
    std::queue<Triangle *> &triangle_stream = C->pipeline.triangle_stream;
    int width = C->width, height = C->height;

    while (!triangle_stream.empty())
    {
        Triangle *t = triangle_stream.front();
        triangle_stream.pop();
        glm::vec3 *screen_pos = t->screen_pos;
        int minx, maxx, miny, maxy, x, y;
        minx = MIN(screen_pos[0].x, MIN(screen_pos[1].x, screen_pos[2].x));
        miny = MIN(screen_pos[0].y, MIN(screen_pos[1].y, screen_pos[2].y));
        maxx = MAX(screen_pos[0].x, MAX(screen_pos[1].x, screen_pos[2].x));
        maxy = MAX(screen_pos[0].y, MAX(screen_pos[1].y, screen_pos[2].y));

        float *zbuf = (float *)C->zbuf->getDataPtr();
        color_t* frame_buf = (color_t*)C->framebuf->getDataPtr();
        // AABB algorithm
        for (y = miny; y <= maxy; ++y)
        {
            for (x = minx; x <= maxx; ++x)
            {
                int index = GET_INDEX(x, y, width, height);
                if (!t->inside(x + 0.5f, y + 0.5f))
                    continue;

                // alpha beta gamma
                std::array<float, 3> coef = t->computeBarycentric2D(x + 0.5f, y + 0.5f);
                float zp = coef[0] * screen_pos[0].z + coef[1] * screen_pos[1].z + coef[2] * screen_pos[2].z;
                float Z = 1.0 / (coef[0] + coef[1] + coef[2]);
                zp *= Z;

                if (zp < zbuf[index])
                {
                    zbuf[index] = zp;
                    glm::vec3 color = interpolate(coef[0], coef[1], coef[2], t->color[0], t->color[1], t->color[2], 1);
                    frame_buf[index].R = color.x;
                    frame_buf[index].G = color.y;
                    frame_buf[index].B = color.z;
                }
            }
        }

        delete t;
    }
}

void process_pixel()
{
}

void *_thr_process_vertex(void *thread_id)
{

    return nullptr;
}

void *_thr_rasterize(void *thread_id)
{

    return nullptr;
}
