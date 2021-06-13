#include "globj.h"

// template<class T>
// int glManager::insertStorage(const glStorage<T>& obj){
//     // pass in object directly, we should use deep copy of data
//     // otherwise we are not sure if the the obj's memory is 
//     // on stack or on heap, which is dangerous
//     glObject * objptr = new glStorage<T>(obj.getSize(), obj.activated, obj.type, obj.bind);
//     if (objptr==nullptr)
//         return GL_FAILURE;
//     int id = idMgr.AllocateId();
//     if (obj.getDataPtr() != nullptr)
//         memcpy(objptr->getDataPtr(), obj.getDataPtr(), obj.byteCapacity());
//     return __insert(objptr, id);
// }

int glManager::insertStorage(GLenum dtype, glObject& obj){
    // if (obj.type==GLOBJ_PLACE_HOLDER){
    //     return GL_FAILURE;
    // }
    glObject * objptr = __storage(dtype, obj.getSize(), obj.activated, obj.type, obj.bind);
    if (objptr==nullptr)
        return GL_FAILURE;
    int id = idMgr.AllocateId();
    if (obj.getDataPtr() != nullptr)
        memcpy(objptr->getDataPtr(), obj.getDataPtr(), obj.byteCapacity());
    return __insert(objptr, id);            
}

int glManager::insertStorage(GLenum dtype, int size){
    glObject * objptr = __storage(dtype, size, false, GL_UNDEF, GL_UNDEF);
    if (objptr==nullptr)
        return GL_FAILURE;
    int id = idMgr.AllocateId();
    return __insert(objptr, id);
}

int glManager::insertStorage(GLenum dtype, int size, bool activated, GLenum type, GLenum bind){
    glObject * objptr = __storage(dtype, size, activated, type, bind);
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

glObject* glManager::__storage(GLenum dtype, int size, bool activated, GLenum type, GLenum bind){
    switch(dtype){
        case GL_FLOAT:
            return new glStorage<float>(size, activated, type, bind);
        case GL_INT:
            return new glStorage<int>(size, activated, type, bind);
        case GL_BYTE:
            return new glStorage<char>(size, activated, type, bind);
        case GL_VERTEX_ATTRIB_CONFIG:
            return new glStorage<vertex_attrib_t>(size, activated, type, bind);
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
}

glThreads::glThreads(){

}

glProgram::glProgram(){

}