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

typedef struct{
    int index;      // No. of vertex attrib
    int size;       // size of attrib (in the unit of float/int/etc.)
    int type;       // dtype 
    bool normalized;// should we normalize the attrib
    int stride;     // stride to get info.
    void* pointer;  // offset
}vertex_attrib_t;

// typedef struct{
//     uint8_t R;
//     uint8_t G;
//     uint8_t B;
// }color_t;

typedef struct{
    float R;
    float G;
    float B;
}color_t;

class glObject{
    public: 
    glObject():activated(false), type(GL_UNDEF), bind(GL_UNDEF){}
    glObject(bool act, GLenum t, GLenum b):activated(act), type(t), bind(b){}
    virtual ~glObject(){}
    virtual void* getDataPtr() const = 0;
    virtual int allocEltSpace(int nelts) = 0;
    virtual int allocByteSpace(int nbytes) = 0;
    virtual int loadElts(const void* src, int nelts) = 0;
    virtual int loadBytes(const void* src, int nbytes) = 0;
    virtual int getSize() const = 0;
    virtual int byteCapacity() const = 0;

    bool activated; // reserved for VAO
    GLenum type;
    GLenum bind;
};

// class glPlaceHolder: public glObject{
//     public:
//         glPlaceHolder(): glObject(false, GLOBJ_PLACE_HOLDER, GL_UNDEF){}
//         glPlaceHolder(GLenum type): glObject(false, type, GL_UNDEF){} 
//         virtual ~glPlaceHolder(){}
//         virtual void* getDataPtr() const{return nullptr;}
//         virtual int allocEltSpace(int nelts){return GL_FAILURE;};
//         virtual int allocByteSpace(int nbytes){return GL_FAILURE;};
//         virtual int loadElts(const void* src, int nelts){return GL_FAILURE;};
//         virtual int loadBytes(const void* src, int nbytes){return GL_FAILURE;};
//         virtual int getSize() const{return GL_FAILURE;};
//         virtual int byteCapacity() const{return GL_FAILURE;}
// };

template<class T>
class glStorage: public glObject{
    public:
        glStorage(): glObject(), size(0), data(nullptr){}
        glStorage(int s):glObject(), size(s){
            if (s>0){
                data = new T[s];
                if (data==nullptr)
                    throw std::runtime_error("memory resource exhausted");
            }else{
                data = nullptr;
                size = 0;
            }
        }
        glStorage(int s, bool act, GLenum t, GLenum b):glObject(act, t, b), size(s){
            if(s>0){
                // std::cout<<"internal size: "<<s<<std::endl;
                data = new T[s];
                std::cout<<"internal data: "<<data<<std::endl;
                if (data==nullptr)
                    throw std::runtime_error("memory resource exhausted");
            }else{
                data = nullptr;
                size = 0;
            }
        }
        glStorage(const glStorage<T>& other){
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

        virtual int loadElts(const void* src, int nelts){
            if (nelts>size || src==nullptr || nelts<=0)
                return GL_FAILURE;
            memcpy((void*)data, src, nelts*sizeof(T));
            return GL_SUCCESS;
        }

        virtual int loadBytes(const void* src, int nbytes){
            if (nbytes > (int)sizeof(T)*size || src==nullptr || nbytes<=0)
                return GL_FAILURE;
            memcpy((void*)data, src, nbytes);
            return GL_SUCCESS;
        }

        virtual int getSize() const{
            return size;
        }

        virtual int byteCapacity() const{
            return (int)sizeof(T)*size;
        }

    private:
        // in the unit of sizeof(T), not bytes
        int size;
        T* data;
};

class glManager{
    public:
        glManager(){}
        virtual ~glManager(){
            for(const auto& it : table){
                delete it.second;
            }
        }

        // template<class T>
        // int insertStorage(const glStorage<T>& obj);
        int insertStorage(GLenum dtype, glObject& obj);
        int insertStorage(GLenum dtype, int size);
        int insertStorage(GLenum dtype, int size, bool activated, GLenum type, GLenum bind);
        int searchStorage(glObject** ptr, int id);

    private:
        std::map<int, glObject*> table;
        IdManager idMgr;

        int __insert(glObject* objptr, int id);
        glObject* __storage(GLenum dtype, int size, bool activated, GLenum type, GLenum bind);
};

class glShareData{
    public:
    glShareData(){}
    glManager buffers;
    glManager vertex_attribs;
    glManager textures;
    glManager framebufs;
};

class glThreads{
    public:
    glThreads();
    pthread_t thr_arr[THREAD_NUM];
};

class glProgram{
    public: 
    glProgram();
};

class glRenderPayload{
    public: 
    glRenderPayload();
    // GL_XXX : ID
    std::map<GLenum, int> renderMap;
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
