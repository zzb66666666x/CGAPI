#ifndef _GLOBJ_H
#define _GLOBJ_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <map>
#include "common.h"
#include "../utils/id.h"

template<class T>
class glStorage{
    public:
        glStorage():activated(false), type(GL_UNDEF), bind(GL_UNDEF), size(0), data(nullptr){}
        glStorage(int s):activated(false), type(GL_UNDEF), bind(GL_UNDEF), size(s){
            data = new T[s];
            if (data==nullptr)
                throw std::runtime_error("memory resource exhausted");
        }
        glStorage(int s, bool act, int t, int b):activated(act), type(t), bind(b), size(s){
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

        ~glStorage(){delete[] data;}

        inline T* getDataPtr() const{
            return data;
        }

        int allocEltSpace(int nelts){
            T* temp = new T[nelts];
            if (temp==nullptr)
                return GL_FAILURE;
            data = temp;
            size = nelts;
            return GL_SUCCESS;
        }

        int loadElts(T* src, int nelts){
            if (nelts>size || src==nullptr || nelts<=0)
                return GL_FAILURE;
            memcpy(data, src, nelts*sizeof(T));
            return GL_SUCCESS;
        }

        template<class T2>
        int loadElts(T* src, int nelts){
            throw std::runtime_error("don't load data to glStorage with different type");
        }

        int getSize() const{
            return size;
        }

        bool activated;
        int type;
        int bind;
    private:
        T* data = nullptr;
        // in the unit of sizeof(T), not bytes
        int size = 0;
};

template<class T>
class glManager{
    public:
        // register storage space in map
        int insertStorage(const glStorage<T>& obj){
            // pass in object directly, we should use deep copy of data
            // otherwise we are not sure if the the obj's memory is 
            // on stack or on heap, which is dangerous
            glStorage<T> * objptr = new glStorage<T>(obj.getSize(), obj.activated, obj.type, obj.bind);
            if (objptr==nullptr)
                return GL_FAILURE;
            int id = idMgr.AllocateId();
            if (obj.getDataPtr() != nullptr)
                memcpy(objptr->getDataPtr(), obj.getDataPtr(), obj.getSize()*sizeof(T));
            return __insert(objptr, id);
        }

        template<class T2>
        int insertStorage(const glStorage<T2>& obj){
            throw std::runtime_error("don't initialize glStorge<T> with another type");
        }

        int insertStorage(int size){
            glStorage<T> * objptr = new glStorage<T>(size);
            if (objptr==nullptr)
                return GL_FAILURE;
            int id = idMgr.AllocateId();
            return __insert(objptr, id);
        }

        int insertStorage(int size, bool activated, int type, int bind){
            glStorage<T> * objptr = new glStorage<T>(size, activated, type, bind);
            if (objptr==nullptr)
                return GL_FAILURE;
            int id = idMgr.AllocateId();
            return __insert(objptr, id);
        }

        int searchStorage(glStorage<T>** ptr, int id){
            auto it = hash.find(id);
            if ((it->first)==id){
                *ptr = it->second;
                return GL_SUCCESS;
            }
            else{
                return GL_FAILURE;
            }
        }

        template<class T2>
        int searchStorage(glStorage<T2>** ptr, int id){
            throw std::runtime_error("please don't search in glManager<T> but expect output storage with another type");
        }

    private:
        std::map<int, glStorage<T>*> hash;
        IdManager idMgr;

        int __insert(glStorage<T>* objptr, int id){
            auto ret = hash.emplace(id, objptr);
            if (!ret.second){
                idMgr.FreeId(id);
                return GL_FAILURE;
            }
            return id;
        }
};

typedef glStorage<int> glSi;
typedef glStorage<float> glSf;
typedef glStorage<int> * glSi_p;
typedef glStorage<float> * glSf_p;

typedef glManager<int> glMi;
typedef glManager<float> glMf;
typedef glManager<int> * glMi_p;
typedef glManager<float> * glMf_p;

#endif
