#include "configs.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "globj.h"
#include "render.h"
#include "glcontext.h"
#include "glsl/texture.h"
#include <omp.h>
#include <thread>

// alloc space for static variables in shader
// glm::mat4 glProgram::model = glm::mat4(1.0f); 
// glm::mat4 glProgram::view = glm::mat4(1.0f);
// glm::mat4 glProgram::projection = glm::mat4(1.0f);
int glProgram::layouts[GL_MAX_VERTEX_ATTRIB_NUM];
int glProgram::layout_cnt;

int glManager::insertStorage(GLenum dtype, glObject& obj){
    // if (obj.type==GLOBJ_PLACE_HOLDER){
    //     return GL_FAILURE;
    // }
    glObject * objptr = __storage(dtype, obj.getSize(), obj.bind);
    if (objptr==nullptr)
        return GL_FAILURE;
    int id = idMgr.AllocateId();
    if (obj.getDataPtr() != nullptr)
        memcpy(objptr->getDataPtr(), obj.getDataPtr(), obj.byteCapacity());
    return __insert(objptr, id);            
}

int glManager::insertStorage(GLenum dtype, int size){
    glObject * objptr = __storage(dtype, size, GL_UNDEF);
    if (objptr==nullptr)
        return GL_FAILURE;
    int id = idMgr.AllocateId();
    return __insert(objptr, id);
}

int glManager::insertStorage(GLenum dtype, int size, GLenum bind){
    glObject * objptr = __storage(dtype, size, bind);
    if (objptr==nullptr)
        return GL_FAILURE;
    int id = idMgr.AllocateId();
    return __insert(objptr, id);
}

int glManager::searchStorage(glObject** ptr, int id){
    auto it = table.find(id);
    if ((it->first)==id){
        *ptr = it->second;
        return GL_SUCCESS;
    }
    else{
        *ptr = nullptr;
        return GL_FAILURE;
    }
}

int glManager::__insert(glObject* objptr, int id){
    auto ret = table.emplace(id, objptr);
    if (!ret.second){
        idMgr.FreeId(id);
        delete objptr;
        return GL_FAILURE;
    }
    return id;
}

glObject* glManager::__storage(GLenum dtype, int size, GLenum bind){
    switch(dtype){
        case GL_FLOAT:
            return new glStorage<float>(size, bind);
        case GL_INT:
            return new glStorage<int>(size, bind);
        case GL_BYTE:
            return new glStorage<char>(size, bind);
        case GL_VERTEX_ATTRIB_CONFIG:
            return new glStorage<vertex_attrib_t>(size, bind);
        case GL_VERTEX_ARRAY_OBJECT:
            return new glStorage<vertex_array_object_t>(size, bind);
        case GL_FRAMEBUFFER_ATTACHMENT_CONFIG:
            return new glStorage<framebuf_attachment_t>(size, bind);
        default:
            break;
    }
    return nullptr;
}

void framebuf_attachment_t::init_sync_unit(int npixels){
    sync_unit.resize(npixels);
    framebuf_lock.resize(npixels);
    for (int i=0; i<npixels; i++){
        omp_init_lock(&(sync_unit[i]));
        framebuf_lock.push_back(new std::atomic_bool(false));
    }
}

glRenderPayload::glRenderPayload(){
    // -1: undef    0: default
    renderMap.emplace(GL_ARRAY_BUFFER, -1);
    renderMap.emplace(GL_BIND_VAO, -1);
    renderMap.emplace(GL_ELEMENT_ARRAY_BUFFER, -1);
    renderMap.emplace(GL_TEXTURE_2D, -1);
    renderMap.emplace(GL_BIND_PROGRAM, -1);
    renderMap.emplace(GL_FRAMEBUFFER, 0);

    // GL_TEXTURE0 is default open to be set (activated)
    tex_units.emplace(GL_TEXTURE0, TEXTURE_UNIT_TBD);
    for(int i = 1;i < GL_MAX_TEXTURE_UNITS;++i){
        tex_units.emplace((GLenum)(GL_TEXTURE0 + i), TEXTURE_UNIT_CLOSE);
    }
}

glThreads::glThreads(){
    memset(usage, 0, THREAD_NUM * sizeof(int));
}

int glThreads::get(int* arg, int thread_num)
{
    if (thread_num <= 0 || arg == nullptr)
        return GL_FAILURE;
    int cnt = 0;
    for (int i = 0; i < THREAD_NUM; i++) {
        if (!usage[i]) {
            arg[cnt] = i;
            cnt++;
        }
        if (cnt == thread_num) {
            for (int j = 0; j < thread_num; j++) {
                usage[arg[j]] = 1;
            }
            return GL_SUCCESS;
        }
    }
    return GL_FAILURE;
}

void glThreads::reset()
{
    for (int i = 0; i < THREAD_NUM; i++) {
        if (usage[i]) {
            pthread_cancel(thr_arr[i]);
        }
    }
    memset(usage, 0, THREAD_NUM * sizeof(int));
}

glProgram::glProgram()
{
    layout_cnt = GL_MAX_VERTEX_ATTRIB_NUM;
}

void glProgram::initialize_layouts()
{
    for (int i = 0; i < GL_MAX_VERTEX_ATTRIB_NUM; i++) {
        layouts[i] = LAYOUT_INVALID;
    }
    layouts[0] = LAYOUT_POSITION;
    layouts[1] = LAYOUT_COLOR;
    layouts[2] = LAYOUT_TEXCOORD;
    layouts[3] = LAYOUT_NORMAL;
}

void glProgram::default_vertex_shader(){
    // std::cout<<input_Pos.x<<" "<<input_Pos.y<<" "<<input_Pos.z<<std::endl;
    frag_Pos = glm::vec3(model * glm::vec4(input_Pos.x, input_Pos.y, input_Pos.z, 1.0f));
    gl_Position = projection * view * glm::vec4(frag_Pos.x, frag_Pos.y, frag_Pos.z, 1.0f);
    // gl_VertexColor = vert_Color;

    gl_Normal = glm::vec3(model_inv_trans * glm::vec4(vert_Normal, 0.0f));
    // gl_Normal = vert_Normal;
}

void glProgram::default_fragment_shader(){
    // without texture
    // frag_Color = diffuse_Color;
    frag_Color = glm::normalize(normal) * 0.5f + glm::vec3(0.5f, 0.5f, 0.5f);

    // with texture
    // frag_Color = glm::vec3(texture2D(diffuse_texture, texcoord));
}

void glProgram::set_transform_matrices(int width, int height, float znear, float zfar, float angle){
    model = glm::mat4(1.0f);
    view = glm::mat4(1.0f);
    projection = glm::mat4(1.0f);

    // for cow
    // model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    // model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    // model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.2f));

    // for bunny
    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.9f, 0.9f, 0.9f));
    // model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.2f));
    // model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));

    // for wheel
    // model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    // model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    // model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));

    // for sponza
    // model = glm::translate(model, glm::vec3(0.0f, -30.0f, -80.0f));
    // model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
    // model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));

    model_inv_trans = glm::transpose(glm::inverse(model));
    glm::vec3 eyepos(0.0f,0.0f,5.0f);
    glm::vec3 front(0.0f, 0.0f, -1.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    view  = glm::lookAt(eyepos, eyepos+front, up);
    projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, znear, zfar);
}

void glProgram::set_diffuse_texture(GLenum unit){
    GET_CURRENT_CONTEXT(C);
    int textureId = C->payload.tex_units[unit];
    glObject *tex_ptr;
    C->share.textures.searchStorage(&tex_ptr, textureId);
    diffuse_texture.config = C->share.tex_config_map[textureId];
    diffuse_texture.data = (unsigned char*)tex_ptr->getDataPtr();
    diffuse_texture.filter = filter_type::NEAREST;
}

void glProgrammableShader::link_programs(){
    // init the call chain
    // default call chain: vertex shader, then fragment shader
    // ignore geometry shader
    auto it1 = shaders.find(GL_VERTEX_SHADER);
    if (it1 == shaders.end()){
        throw std::runtime_error("link error: missing vertex shader\n");
    }
    auto it2 = shaders.find(GL_FRAGMENT_SHADER);
    if (it2 == shaders.end()){
        throw std::runtime_error("link error: missing fragment shader");
    }
    // merge the uniform variables
    int uniform_id = 0;
    for (auto it = shaders.begin(); it != shaders.end(); ++it){
        for (auto it2 = it->second->uniform_map.begin(); it2 != it->second->uniform_map.end(); ++it2) {
            if (merged_uniform_maps.find(it2->first) != merged_uniform_maps.end()){
                // already has item inside
                merged_uniform_maps[it2->first].uniform_ftable_idx.emplace(it->first, it2->second);
            }else{
                // insert item
                uniform_varaible_t uvar;
                uvar.uniform_id = uniform_id;
                uvar.uniform_ftable_idx.emplace(it->first, it2->second);
                merged_uniform_maps.emplace(it2->first, uvar);
                uniform_id_to_name.emplace(uniform_id, it2->first);
                ++uniform_id;
            }
        }
    }
}

Shader* glProgrammableShader::get_shader(GLenum shader_type){
    auto it = shaders.find(shader_type);
    if (it != shaders.end())
        return (it->second);
    return nullptr;
}

glProgramManager::~glProgramManager(){
    for(auto it = shader_cache_map.begin(); it != shader_cache_map.end(); it++){
        delete it->second;
    }
}

int glProgramManager::create_program(){
    int id = idmgr.AllocateId();
    shader_program_map.emplace(id, glProgrammableShader());
    return id;
}

int glProgramManager::create_shader(GLenum shader_type, int cpu_num){
    int id = cache_idmgr.AllocateId();
    Shader* shader_ptr = new Shader(shader_type, cpu_num);
    shader_cache_map.emplace(id, shader_ptr);
    return id;
}

int glProgramManager::attach(int prog, int shader_cache_id){
    // sanity check
    auto it_prog = shader_program_map.find(prog);
    auto it_shader = shader_cache_map.find(shader_cache_id);
    if (it_prog == shader_program_map.end() || it_shader == shader_cache_map.end()){
        return GL_FAILURE;
    }
    glProgrammableShader& shader_prog = it_prog->second;
    Shader* shader_ptr = it_shader->second;
    if (shader_prog.shaders.find(shader_ptr->shader_type) != shader_prog.shaders.end()){
        // replace the old shader
        shader_prog.shaders[shader_ptr->shader_type] = shader_ptr;
    }else{
        shader_prog.shaders.emplace(shader_ptr->shader_type, shader_ptr);
    }
    return GL_SUCCESS;
}

glPipeline::glPipeline()
{
    cpu_num = std::thread::hardware_concurrency();
    pixel_block.resize(cpu_num);
    vfc_list.resize(cpu_num);
    bin_data.resize(cpu_num);
    for(int i = 0;i<cpu_num;++i){
        pixel_block[i].resize(4);
    }
    // cpu_num = 10;
    omp_set_num_threads(cpu_num);
    omp_init_lock(&tri_culling_lock);

    vertex_num = 0;
    first_vertex = 0;
    prog_triangle_size = 0;
    triangle_stream_mtx = PTHREAD_MUTEX_INITIALIZER;
    // exec.emplace_back(process_geometry_threadmain);
    // exec.emplace_back(rasterize_threadmain);
    // exec.emplace_back(process_pixel);

    // exec.emplace_back(process_geometry_ebo_openmp);
    // exec.emplace_back(rasterize_with_shading_openmp);
    // exec.emplace_back(process_pixel_openmp);

#ifndef GL_SCANLINE
    // exec.emplace_back(programmable_process_geometry_openmp);
    // exec.emplace_back(programmable_rasterize_with_shading_openmp);

    exec.emplace_back(programmable_process_geometry_with_rasterization);
    exec.emplace_back(programmable_process_pixel);
#else
    exec.emplace_back(programmable_process_geometry_openmp);
    exec.emplace_back(programmable_rasterize_with_scanline);
#endif

    vao_ptr = nullptr;
    vbo_ptr = nullptr;
    ebo_config.ebo_ptr = nullptr;
    for (int i=0; i<GL_MAX_TEXTURE_UNITS; i++){
        textures[i] = nullptr;
    }
}

glPipeline::~glPipeline()
{
    std::vector<Pixel>::iterator it;
    for (it = pixel_tasks.begin(); it != pixel_tasks.end(); it++) {
        omp_destroy_lock(&(it->lock));
    }
    for (int i = 0, len = prog_triangle_list.size(); i < len;++i){
        delete prog_triangle_list[i];
    }
    for (int i = 0; i < cpu_num;++i){
        for (int j = 0, len = vfc_list[i].size();j<len;++j){
            delete vfc_list[i][j];
        }
    }
    // delete bins;
}

void glPipeline::init_pixel_locks()
{
    std::vector<Pixel>::iterator it;
    for (it = pixel_tasks.begin(); it != pixel_tasks.end(); it++) {
        omp_init_lock(&(it->lock));
    }
}
