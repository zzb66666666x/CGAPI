#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "globj.h"
#include "render.h"
#include "glcontext.h"
#include "glsl/texture.h"

// alloc space for static variables in shader
glm::mat4 glProgram::model = glm::mat4(1.0f); 
glm::mat4 glProgram::view = glm::mat4(1.0f);
glm::mat4 glProgram::projection = glm::mat4(1.0f);

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
        default:
            break;
    }
    return nullptr;
}


glRenderPayload::glRenderPayload(){
    renderMap.emplace(GL_ARRAY_BUFFER, -1);
    renderMap.emplace(GL_BIND_VAO, -1);
    renderMap.emplace(GL_ELEMENT_ARRAY_BUFFER, -1);
    renderMap.emplace(GL_FRAMEBUFFER, -1);
    renderMap.emplace(GL_TEXTURE_2D, -1);
    // for(int i = 0;i < 16;++i){
    //     tex_units.emplace(GL_TEXTURE0 + i, TEXTURE_UNIT_CLOSE);
    // }
    tex_units.emplace(GL_TEXTURE0, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE1, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE2, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE3, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE4, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE5, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE6, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE7, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE8, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE9, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE10, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE11, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE12, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE13, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE14, TEXTURE_UNIT_CLOSE);
    tex_units.emplace(GL_TEXTURE15, TEXTURE_UNIT_CLOSE);
}

glThreads::glThreads(){

}

glProgram::glProgram(){
    layouts[0] = 0;
    layouts[1] = 1;
    layouts[2] = 2;
    layout_cnt = 3;
}

void glProgram::default_vertex_shader(){
    frag_Pos = glm::vec3(model * glm::vec4(input_Pos.x, input_Pos.y, input_Pos.z, 1.0f));
    // glm::vec4 test = view*glm::vec4(frag_Pos.x, frag_Pos.y, frag_Pos.z, 1.0f);
    gl_Position = projection * view * glm::vec4(frag_Pos.x, frag_Pos.y, frag_Pos.z, 1.0f);
    gl_VertexColor = vert_Color;
}

PixelShaderResult glProgram::default_fragment_shader(PixelShaderParam &params){
    // frag_Color = diffuse_Color;
    // glm::vec4 color = texture2D(diffuse_texture, texcoord);
    // frag_Color.x = color.x;
    // frag_Color.y = color.y;
    // frag_Color.z = color.z;
    PixelShaderResult result;
    glm::vec4 color = texture2D(diffuse_texture, params.texcoord);
    result.fragColor = color;
    return result;
}

void glProgram::set_transform_matrices(int width, int height, float znear, float zfar, float angle){
    GET_CURRENT_CONTEXT(ctx);
    
    model = glm::mat4(1.0f); 
    view = glm::mat4(1.0f);
    projection = glm::mat4(1.0f);

    int textureId = ctx->payload.renderMap[GL_TEXTURE_2D];
    glObject *tex_ptr;
    ctx->share.textures.searchStorage(&tex_ptr, textureId);
    diffuse_texture.width = 512;
    diffuse_texture.height = 512;
    diffuse_texture.channel = 3;
    diffuse_texture.data = (unsigned char*)tex_ptr->getDataPtr();
    diffuse_texture.filter = filter_type::NEAREST;
    
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.6f, 1.0f, 0.8f));
    model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
    glm::vec3 eyepos(0.0f,0.0f,5.0f);
    glm::vec3 front(0.0f, 0.0f, -1.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    view  = glm::lookAt(eyepos, eyepos+front, up);
    projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, znear, zfar);
}


glPipeline::glPipeline(){
    vertex_num = 0;
    first_vertex = 0;
    triangle_stream_mtx = PTHREAD_MUTEX_INITIALIZER;
    exec.emplace_back(process_geometry);
    exec.emplace_back(rasterize);
    exec.emplace_back(process_pixel);
    vao_ptr = nullptr;
    vbo_ptr = nullptr;
    ebo_ptr = nullptr;
    for (int i=0; i<GL_MAX_TEXTURE_UNITS; i++){
        textures[i] = nullptr;
    }
}
