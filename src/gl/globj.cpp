#include "globj.h"
#include "render.h"

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

}

glPipeline::glPipeline(){
    vertex_num = 0;
    first_vertex = 0;
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
