#include <benchmark/benchmark.h>
#include "../gl/glcontext.h"
#include "../../include/gl/gl.h"
#include "../gl/render.h"
#include "../../include/glv/glv.h"
#include "OBJ_Loader.h"

const int WIDTH = 800, HEIGHT = 600;
unsigned int VBO, VAO, EBO;
unsigned indices_size;

///////////////////// prepareElement without any drawcall /////////////////////

static void check_set_layouts()
{
    GET_CURRENT_CONTEXT(C);
    vertex_attrib_t* vattrib_data = (vertex_attrib_t*)C->pipeline.vao_ptr->getDataPtr();
    // printf("vao_ptr.size = %d\n",ppl->vao_ptr->getSize());
    // check if the config is activated
    for (int i = 0; i < C->shader.layout_cnt; ++i) {
        if (C->shader.layouts[i] != LAYOUT_INVALID
            && C->shader.layouts[i] >= C->pipeline.vao_ptr->getSize()) {
            C->shader.layouts[i] = LAYOUT_INVALID;
        } else if (!vattrib_data[C->shader.layouts[i]].activated) {
            C->shader.layouts[i] = LAYOUT_INVALID;
        }
    }
}

static void prepareElement(GLenum mode, int count, unsigned int type, const void* indices)
{
    GET_CURRENT_CONTEXT(ctx);
    if (ctx == nullptr) {
        return;
    }
    glManager& bufs = ctx->share.buffers;
    glManager& vaos = ctx->share.vertex_attribs;
    glManager& texs = ctx->share.textures;
    glObject* vao_ptr;
    glObject* vbo_ptr;
    glObject* ebo_ptr;
    glObject* tex_ptr;
    int ret, vao_id, vbo_id, ebo_id;
    // sanity checks for VAO
    vao_id = ctx->payload.renderMap[GL_BIND_VAO];
    vbo_id = ctx->payload.renderMap[GL_ARRAY_BUFFER];
    ebo_id = ctx->payload.renderMap[GL_ELEMENT_ARRAY_BUFFER];
    if (vao_id < 0 || vbo_id < 0 || ebo_id < 0) {
        return;
    }
    ret = vaos.searchStorage(&vao_ptr, vao_id);
    if (ret == GL_FAILURE
        || vao_ptr->bind != GL_BIND_VAO
        || vao_ptr->getDataPtr() == nullptr
        || vao_ptr->getSize() <= 0) {
        return;
    }
    ret = bufs.searchStorage(&vbo_ptr, vbo_id);
    if (ret == GL_FAILURE
        || vbo_ptr->bind != GL_ARRAY_BUFFER
        || vbo_ptr->getDataPtr() == nullptr
        || vbo_ptr->getSize() <= 0) {
        return;
    }
    ret = bufs.searchStorage(&ebo_ptr, ebo_id);
    if (ret == GL_FAILURE
        || ebo_ptr->bind != GL_ELEMENT_ARRAY_BUFFER
        || ebo_ptr->getDataPtr() == nullptr
        || ebo_ptr->getSize() <= 0) {
        return;
    }
    // sanity check for texture resources
    // check active textures, tell pipeline what are the useful textures
    int cnt = 0;
    for (auto& it : ctx->payload.tex_units) {
        if (it.second > 0) {
            // check if the texture ID is valid
            ret = texs.searchStorage(&tex_ptr, it.second);
            if (ret == GL_FAILURE || tex_ptr->getSize() <= 0 || tex_ptr->getDataPtr() == nullptr)
                return;
            ctx->pipeline.textures[cnt] = tex_ptr;
            ctx->shader.set_diffuse_texture((GLenum)((int)GL_TEXTURE0 + cnt));
        } else {
            ctx->pipeline.textures[cnt] = nullptr;
        }
        ++cnt;
    }
    // prepare pipeline environment
    ctx->pipeline.vao_ptr = vao_ptr;
    ctx->pipeline.vbo_ptr = vbo_ptr;
    ctx->pipeline.vertex_num = count;
    ctx->pipeline.ebo_config.ebo_ptr = ebo_ptr;
    ctx->pipeline.ebo_config.first_indices = indices;
    ctx->pipeline.ebo_config.indices_type = type;
    ctx->pipeline.use_indices = true;

    check_set_layouts();

    // // draw
    // std::list<render_fp>& exec_list = ctx->pipeline.exec;
    // auto iter = exec_list.begin();
    // switch (mode) {
    // case GL_TRIANGLES:
    //     while (iter != exec_list.end()) {
    //         (*iter)();
    //         ++iter;
    //     }
    //     break;
    // default:
    //     break;
    // }
}

//////////////////// load Model /////////////////////
static GLVStream* setUp()
{
    if (!glvInit()) {
        std::cout << "glv Init failed\n";
        return nullptr;
    }

    GLVStream* window = glvCreateStream(WIDTH, HEIGHT, "bunny_test", GLV_STREAM_WINDOW);

    glEnable(GL_DEPTH_TEST);

    // Initialize Loader
    objl::Loader Loader;

    bool loadout = Loader.LoadFile("../resources/bunny/bunny.obj");

    if (!loadout) {
        printf("bunny failed to load");
        return nullptr;
    }

    std::vector<unsigned int> indices;
    std::vector<float> vertices;
    // Go through each loaded mesh and out its contents
    for (int i = 0; i < Loader.LoadedMeshes.size(); i++) {
        // Copy one of the loaded meshes to be our current mesh
        objl::Mesh curMesh = Loader.LoadedMeshes[i];

        for (int j = 0; j < curMesh.Vertices.size(); j++) {
            vertices.push_back(curMesh.Vertices[j].Position.X);
            vertices.push_back(curMesh.Vertices[j].Position.Y);
            vertices.push_back(curMesh.Vertices[j].Position.Z);

            // vertices.push_back(0.3f);
            // vertices.push_back(0.4f);
            // vertices.push_back(0.8f);

            vertices.push_back(curMesh.Vertices[j].Normal.X);
            vertices.push_back(curMesh.Vertices[j].Normal.Y);
            vertices.push_back(curMesh.Vertices[j].Normal.Z);
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
    // printf("vertices size: %u\n", vertices.size() / 6);
    // printf("indices size: %u\n", indices.size());
    // printf("triangle size: %u\n", indices.size() / 3);

    indices_size = indices.size();

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // 将vertices数据复制到缓冲种
    // GL_STATIC_DRAW / GL_DYNAMIC_DRAW、GL_STREAM_DRAW
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // 解析缓冲数据
    // 顶点位置，顶点大小(vec3)，顶点类型，是否normalize，步长，偏移量
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    // 激活顶点属性0
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return window;

    // while (!glvWindowShouldClose(window)) {
    //     glBindVertexArray(VAO);

    //     glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    //     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //     glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    //     glvWriteStream(window);

    // }

    // glvTerminate();
}
//////////////////// test pipeline //////////////////

static void test_process_geometry_ebo_openmp(benchmark::State& state)
{

    if(setUp() == nullptr){
        return;
    }
    for (auto _ : state) {
        state.PauseTiming();
        glBindVertexArray(VAO);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        prepareElement(GL_TRIANGLES, indices_size, GL_UNSIGNED_INT, 0);
        state.ResumeTiming();

        process_geometry_ebo_openmp();
    }
}

static void test_rasterize_with_shading_openmp(benchmark::State& state)
{
    if (setUp() == nullptr) {
        return;
    }
    for (auto _ : state) {
        
        state.PauseTiming();
        glBindVertexArray(VAO);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        prepareElement(GL_TRIANGLES, indices_size, GL_UNSIGNED_INT, 0);
        process_geometry_ebo_openmp();
        state.ResumeTiming();

        rasterize_with_shading_openmp();
    }
}

static void test_write_framebuffer(benchmark::State& state)
{
    GLVStream* window = nullptr;
    if ((window = setUp()) == nullptr) {
        return;
    }

    for (auto _ : state) {

        state.PauseTiming();
        glBindVertexArray(VAO);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        prepareElement(GL_TRIANGLES, indices_size, GL_UNSIGNED_INT, 0);
        process_geometry_ebo_openmp();
        rasterize_with_shading_openmp();
        state.ResumeTiming();

        glvWriteStream(window);
    }

    glvTerminate();
}

// Register the function as a benchmark
BENCHMARK(test_process_geometry_ebo_openmp)->Iterations(100);
BENCHMARK(test_rasterize_with_shading_openmp)->Iterations(100);
BENCHMARK(test_write_framebuffer)->Iterations(100);
// Run the benchmark
BENCHMARK_MAIN();