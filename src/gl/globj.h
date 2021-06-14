#ifndef _GLOBJ_H
#define _GLOBJ_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <map>
#include <queue>
#include <list>
#include <vector>
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

template<class T>
class glStorage: public glObject{
    public:
        glStorage(): glObject(), size(0){data.resize(0);}
        glStorage(int s):glObject(), size(s){
            if (s>0){
                data.resize(0);
            }else{
                size = 0;
                data.resize(0);
            }
        }
        glStorage(int s, bool act, GLenum t, GLenum b):glObject(act, t, b), size(s){
            if(s>0){
                data.resize(s);
            }else{
                data.resize(0);
                size = 0;
            }
        }
        glStorage(glStorage<T>& other){
            // time consuming !
            printf("LARGE SCALE COPY HAPPENS IN GL_STORAGE\n");
            activated = other.activated;
            type = other.type;
            bind = other.bind;
            size = other.size;
            // deep copy
            if (other.size>0){
                data.assign(other.getVectorRef().begin(), other.getVectorRef().end());
            }
        }

        template<class T2>
        glStorage(const glStorage<T2>& other){
            throw std::runtime_error("don't initialize glStorge<T> with another type");
        }

        virtual ~glStorage(){
            data.resize(0);
        }

        virtual void* getDataPtr() const{
            // std::cout<<"data ptr: "<<&data[0]<<std::endl;
            return (void*)&data[0];
        }

        // non virtual function
        std::vector<T>& getVectorRef(){
            return data;
        }

        virtual int allocEltSpace(int nelts){
            size = nelts;
            data.clear();
            data.resize(nelts);
            return GL_SUCCESS;
        }

        virtual int allocByteSpace(int nbytes){
            int nelts = (int)(nbytes/sizeof(T));
            size = nelts;
            data.clear();
            data.resize(nelts);
            return GL_SUCCESS;
        }

        virtual int loadElts(const void* src, int nelts){
            if (nelts>size || src==nullptr || nelts<=0)
                return GL_FAILURE;
            memcpy((void*)&data[0], src, nelts*sizeof(T));
            return GL_SUCCESS;
        }

        virtual int loadBytes(const void* src, int nbytes){
            if (nbytes > (int)sizeof(T)*size || src==nullptr || nbytes<=0)
                return GL_FAILURE;
            memcpy((void*)&data[0], src, nbytes);
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
        std::vector<T> data;
};

class glManager{
    public:
        glManager(){}
        virtual ~glManager(){
            for(const auto& it : table){
                delete it.second;
            }
        }

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
        glPipeline();
        // data needed for render functions
        output_stream<Triangle> triStream;
        std::list<render_fp> exec;
};

#endif
