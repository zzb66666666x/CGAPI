#ifndef _GLOBJ_H
#define _GLOBJ_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <map>
#include <queue>
#include <list>
#include <pthread.h>
#include "geometry.h"
#include "render.h"
#include "../utils/id.h"
#include "../../include/gl/common.h"

class glObject{
    public: 
    glObject():activated(false), type(GL_UNDEF), bind(GL_UNDEF){}
    glObject(bool act, GLenum t, GLenum b):activated(act), type(t), bind(b){}
    virtual ~glObject(){}
    virtual void* getDataPtr() const = 0;
    virtual int allocEltSpace(int nelts) = 0;
    virtual int allocByteSpace(int nbytes) = 0;
    virtual int loadElts(void* src, int nelts) = 0;
    virtual int loadBytes(void* src, int nbytes) = 0;
    virtual int getSize() const = 0;
    virtual int byteCapacity() const = 0;

    bool activated;
    GLenum type;
    GLenum bind;
};

template<class T>
class glStorage: public glObject{
    public:
        glStorage(): glObject(), size(0), data(nullptr){}
        glStorage(int s):glObject(), size(s){
            data = new T[s];
            if (data==nullptr)
                throw std::runtime_error("memory resource exhausted");
        }
        glStorage(int s, bool act, GLenum t, GLenum b):glObject(act, t, b), size(s){
            data = new T[s];
            if (data==nullptr)
                throw std::runtime_error("memory resource exhausted");
        }
        glStorage(const glStorage& other){
            // time consuming !
            printf("LARGE SCALE COPY HAPPENS IN GL_STORAGE\n");
            activated = other.activated;
            type = other.type;
            bind = other.bind;
            size = other.size;
            // deep copy
            if (other.size!=0 && other.data != nullptr){
                data = new T[size];
                if (data!=nullptr)
                    memcpy(data, other.data, sizeof(T)*size);
                else
                    throw std::runtime_error("memory resource exhausted");
            }
        }

        template<class T2>
        glStorage(const glStorage<T2>& other){
            throw std::runtime_error("don't initialize glStorge<T> with another type");
        }

        virtual ~glStorage(){
            // std::cout<<"free space!!!"<<std::endl;
            delete[] data;
        }

        virtual void* getDataPtr() const{
            return (void*)data;
        }

        virtual int allocEltSpace(int nelts){
            T* temp = new T[nelts];
            if (temp==nullptr)
                return GL_FAILURE;
            // first free old data
            delete[] data;
            data = temp;
            size = nelts;
            return GL_SUCCESS;
        }

        virtual int allocByteSpace(int nbytes){
            int nelts = (int)(nbytes/sizeof(T));
            T* temp = new T[nelts];
            if (temp==nullptr)
                return GL_FAILURE;
            // first free old data
            delete[] data;
            data = temp;
            size = nelts;
            return GL_SUCCESS;
        }

        virtual int loadElts(void* src, int nelts){
            if (nelts>size || src==nullptr || nelts<=0)
                return GL_FAILURE;
            memcpy((void*)data, src, nelts*sizeof(T));
            return GL_SUCCESS;
        }

        virtual int loadBytes(void* src, int nbytes){
            if (nbytes > sizeof(T)*size || src==nullptr || nbytes<=0)
                return GL_FAILURE;
            memcpy((void*)data, src, nbytes);
            return GL_SUCCESS;
        }

        virtual int getSize() const{
            return size;
        }

        virtual int byteCapacity() const{
            return sizeof(T)*size;
        }

    private:
        T* data = nullptr;
        // in the unit of sizeof(T), not bytes
        int size = 0;
};

class glManager{
    public:
        glManager(){}
        virtual ~glManager(){
            for(const auto& it : hash){
                delete it.second;
            }
        }

        // register storage space in map
        template<class T>
        int insertStorage(const glStorage<T>& obj){
            // pass in object directly, we should use deep copy of data
            // otherwise we are not sure if the the obj's memory is 
            // on stack or on heap, which is dangerous
            glObject * objptr = new glStorage<T>(obj.getSize(), obj.activated, obj.type, obj.bind);
            if (objptr==nullptr)
                return GL_FAILURE;
            int id = idMgr.AllocateId();
            if (obj.getDataPtr() != nullptr)
                memcpy(objptr->getDataPtr(), obj.getDataPtr(), obj.byteCapacity());
            return __insert(objptr, id);
        }

        int insertStorage(GLenum dtype, glObject& obj){
            glObject * objptr = __storage(dtype, obj.getSize(), obj.activated, obj.type, obj.bind);
            if (objptr==nullptr)
                return GL_FAILURE;
            int id = idMgr.AllocateId();
            if (obj.getDataPtr() != nullptr)
                memcpy(objptr->getDataPtr(), obj.getDataPtr(), obj.byteCapacity());
            return __insert(objptr, id);            
        }

        int insertStorage(GLenum dtype, int size){
            glObject * objptr = __storage(dtype, size, false, GL_UNDEF, GL_UNDEF);
            if (objptr==nullptr)
                return GL_FAILURE;
            int id = idMgr.AllocateId();
            return __insert(objptr, id);
        }

        int insertStorage(GLenum dtype, int size, bool activated, GLenum type, GLenum bind){
            glObject * objptr = __storage(dtype, size, activated, type, bind);
            if (objptr==nullptr)
                return GL_FAILURE;
            int id = idMgr.AllocateId();
            return __insert(objptr, id);
        }

        int searchStorage(glObject** ptr, int id){
            auto it = hash.find(id);
            if ((it->first)==id){
                *ptr = it->second;
                return GL_SUCCESS;
            }
            else{
                return GL_FAILURE;
            }
        }

    private:
        std::map<int, glObject*> hash;
        IdManager idMgr;

        int __insert(glObject* objptr, int id){
            auto ret = hash.emplace(id, objptr);
            if (!ret.second){
                idMgr.FreeId(id);
                delete objptr;
                return GL_FAILURE;
            }
            return id;
        }

        glObject* __storage(GLenum dtype, int size, bool activated, GLenum type, GLenum bind){
            switch(dtype){
                case GL_FLOAT:
                    return new glStorage<float>(size, activated, type, bind);
                case GL_INT:
                    return new glStorage<int>(size, activated, type, bind);
                case GL_BYTE:
                    return new glStorage<char>(size, activated, type, bind);
                default:
                    break;
            }
            return nullptr;
        }
};

class glShareData{
    public:
    glManager buffers;
    glManager vertex_attribs;
    glManager textures;
};

class glThreads{
    public:
    pthread_t thr_arr[THREAD_NUM];
};

typedef struct{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t padding;
}color_t; //32 bits

class glProgram{
    public: 

};

class glRenderPayload{
    public: 

};

template<class T>
class output_stream{
    public:
    output_stream(){
        mtx = PTHREAD_MUTEX_INITIALIZER;
    }
    std::queue<T> stream;
    pthread_mutex_t mtx;
};

class glPipeline{
    public:
        glPipeline(){}
        // data needed for render functions
        output_stream<Triangle> triStream;
        std::list<render_fp> exec;
};

#endif
