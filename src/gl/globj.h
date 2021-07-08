#ifndef _GLOBJ_H
#define _GLOBJ_H

#include "configs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <map>
#include <queue>
#include <list>
#include <vector>
#include <pthread.h>
#include <glm/glm.hpp>
#include "geometry.h"
#include "render.h"
#include "formats.h"
#include "binning.h"
#include "../utils/id.h"
#include "../../include/gl/common.h"
#include "glsl/texture.h"
#include <unordered_map>
#include <omp.h>

#define TEXTURE_UNIT_CLOSE     -1
#define TEXTURE_UNIT_TBD        0
#define LAYOUT_INVALID         -1
#define LAYOUT_POSITION         0
#define LAYOUT_COLOR            1
#define LAYOUT_TEXCOORD         2
#define LAYOUT_NORMAL           3
#define UNIFORM_ID_BASE         0x10000000 
#define SHADER_INTERNAL_BASE    0x20000000          
#define UNIFORM(i)              UNIFORM_ID_BASE + i      
#define SHADER_INTERNAL(i)      SHADER_INTERNAL_BASE + i
// processed layout
#define VSHADER_OUT_POSITION    SHADER_INTERNAL(LAYOUT_POSITION)
#define VSHADER_OUT_COLOR       SHADER_INTERNAL(LAYOUT_COLOR)
#define VSHADER_OUT_TEXCOORD    SHADER_INTERNAL(LAYOUT_TEXCOORD)
#define VSHADER_OUT_NORMAL      SHADER_INTERNAL(LAYOUT_NORMAL)
// newly added shader variable, starting from base+16
#define VSHADER_OUT_FRAGPOS     SHADER_INTERNAL(GL_MAX_VERTEX_ATTRIB_NUM)

typedef struct{
    int     index;      // No. of vertex attrib
    int     size;       // size of attrib (in the unit of float/int/etc.)
    GLenum  type;       // dtype 
    bool    normalized; // should we normalize the attrib
    bool    activated;  // should this vertex attrib be used 
    int     stride;     // stride to get info.
    void*   pointer;    // offset
}vertex_attrib_t;

// typedef struct{
//     uint8_t R;
//     uint8_t G;
//     uint8_t B;
// }color_t;

typedef COLOR_32FC3 color_t;

class glObject{
    public: 
    glObject():bind(GL_UNDEF){}
    glObject(GLenum b): bind(b){}
    virtual ~glObject(){}
    virtual void* getDataPtr() const = 0;
    virtual int allocEltSpace(int nelts) = 0;
    virtual int allocByteSpace(int nbytes) = 0;
    virtual int loadElts(const void* src, int nelts) = 0;
    virtual int loadBytes(const void* src, int nbytes) = 0;
    virtual int getSize() const = 0;
    virtual int byteCapacity() const = 0;

    GLenum bind;
    // GL_STATIC_DRAW
    GLenum usage;
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
        glStorage(int s, GLenum b):glObject(b), size(s){
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
        int insertStorage(GLenum dtype, int size, GLenum bind);
        int searchStorage(glObject** ptr, int id);

    private:
        std::map<int, glObject*> table;
        IdManager idMgr;

        int __insert(glObject* objptr, int id);
        glObject* __storage(GLenum dtype, int size, GLenum bind);
};

class glShareData{
    public:
    glShareData(){}
    glManager buffers;
    glManager vertex_attribs;
    glManager textures;
    std::map<int, sampler_config> tex_config_map;
    glManager framebufs;
};

class glThreads{
    public:
    glThreads();
    pthread_t thr_arr[THREAD_NUM];
    int usage[THREAD_NUM];
    int get(int* arg, int thread_num);
    void reset();
};

class glProgram{
    public: 
    glProgram();
    // layouts
    static int layouts[GL_MAX_VERTEX_ATTRIB_NUM];
    static int layout_cnt;
    // input to vertex shader
    glm::vec3 input_Pos;
    glm::vec3 vert_Color;
    glm::vec2 iTexcoord;
    glm::vec3 vert_Normal;
    // vertex shader output variable
    glm::vec4 gl_Position;
    glm::vec3 gl_VertexColor;
    glm::vec3 gl_Normal;
    // fragment shader
    glm::vec3 diffuse_Color;
    glm::vec3 frag_Pos;
    glm::vec3 frag_Color;
    glm::vec2 texcoord;
    static glm::mat4 model; 
    static glm::mat4 view;
    static glm::mat4 projection;
    sampler2D diffuse_texture;
    // methods
    void default_vertex_shader();
    void default_fragment_shader();
    void set_transform_matrices(int width, int height, float znear, float zfar, float angle);
    void set_diffuse_texture(GLenum unit);
};

class glRenderPayload{
    public: 
    glRenderPayload();
    // GL_XXX : ID
    std::map<GLenum, int> renderMap;
    // texture paths
    std::map<GLenum, int> tex_units;
};

struct Pixel{
    Pixel():write(false){}
    bool write;
    
    // programmable input
    glm::vec3 vertexColor;
    glm::vec2 texcoord;
    // sync
    omp_lock_t lock;
};

class PrimitiveCache{
public:
    int removeCacheData(unsigned int VAO){
        int ret = indexCache.erase(VAO);
        return ret == 0 ? GL_FALSE : GL_TRUE;
    }

    int getCacheData(unsigned int VAO, std::vector<int>& indices)
    {
        auto it = indexCache.find(VAO);
        if (it == indexCache.end()) {
            return GL_FALSE;
        }
        indices = it->second;
        return GL_TRUE;
    }

    int addCacheData(unsigned int VAO, std::vector<int> &indices){
        auto ret = indexCache.insert(std::make_pair(VAO, indices));
        return ret.second ? GL_TRUE : GL_FALSE;
    }

private:
    std::unordered_map<unsigned int, std::vector<int>> indexCache;
};

class glPipeline{
    public:
        glPipeline();
        ~glPipeline();
        void init_pixel_locks();
        // the number of cpu core
        int cpu_num;
        // data needed for render functions
        std::queue<Triangle*> triangle_stream;
        pthread_mutex_t triangle_stream_mtx;
        // multi-thread pixel processing version of code (manually calling pthread API)
        // list is better for parallel computing
        std::vector<Triangle*> triangle_list;   
        std::list<render_fp> exec;
        // pixel processing task list
        std::vector<Pixel> pixel_tasks;
        // glManager search cache
        int first_vertex;
        int vertex_num;
        glObject* vao_ptr;
        glObject* vbo_ptr;
        glObject* textures[GL_MAX_TEXTURE_UNITS];
        // data structures supporting drawing by indices (EBO)
        bool use_indices;
        struct {
            const void* first_indices;
            unsigned int indices_type;
            glObject* ebo_ptr;
        } ebo_config;
        PrimitiveCache indexCache;
        // binning data
        ScreenBins* bins;
};

#endif
