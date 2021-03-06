#ifndef _GLOBJ_H
#define _GLOBJ_H

#include "configs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <map>
#include <unordered_map>
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
#include "glsl/shader.hpp"
#include "glsl/inner_support.h"
#include <atomic>

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

typedef struct{
    int vertex_buffer_object_id;                        // vbo id
    int element_buffer_object_id;                       // ebo id
    vertex_attrib_t attribs[GL_MAX_VERTEX_ATTRIB_NUM];  // attribs
} vertex_array_object_t;

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

class framebuf_attachment_t{
    public:
    framebuf_attachment_t():attachment_type(GL_UNDEF),
                            attached_obj_for_depth(-1),
                            attached_obj_for_color(-1),
                            buffer_npixels(0),
                            color_buf(nullptr),
                            depth_buf(nullptr),
                            draw_color_buf(true),
                            flip_image(false)
                            {}
    void init_sync_unit(int npixels);
    GLenum attachment_type;
    int attached_obj_for_depth;
    int attached_obj_for_color;
    int buffer_npixels;
    color_t * color_buf;
    float * depth_buf;
    bool draw_color_buf;
    bool flip_image;
    std::vector<omp_lock_t> sync_unit;
    std::vector<std::atomic_bool*> framebuf_lock;
};

class glShareData{
    public:
    glShareData(){}
    glManager buffers;
    glManager vertex_attribs;
    // new vao fields
    glManager vertex_array_objects;
    glManager textures;
    std::map<int, sampler_config> tex_config_map;
    std::map<int, MipmapStorage> tex_mipmaps_map;
    glManager framebuf_attachments;
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
    glm::vec3 normal;
    glm::vec2 texcoord;

    glm::mat4 model;
    glm::mat4 model_inv_trans;
    glm::mat4 view;
    glm::mat4 projection;
    sampler2D diffuse_texture;
    // methods
    void default_vertex_shader();
    void default_fragment_shader();
    void set_transform_matrices(int width, int height, float znear, float zfar, float angle);
    void set_diffuse_texture(GLenum unit);
    void initialize_layouts();
};

typedef struct{
    data_t data;
    int uniform_id;     // unique id for uniform variables in one shder program
    std::map<GLenum, int> uniform_ftable_idx;
}uniform_varaible_t;

class glProgrammableShader{
    public:
    std::map<GLenum, Shader*> shaders;
    std::map<std::string, uniform_varaible_t> merged_uniform_maps;
    std::map<int, std::string> uniform_id_to_name;
    void link_programs();
    Shader* get_shader(GLenum shader_type);
};

class glProgramManager {
    public:
    ~glProgramManager();
    std::map<int, glProgrammableShader> shader_program_map;
    std::map<int, Shader*> shader_cache_map;
    IdManager idmgr;
    IdManager cache_idmgr;
    int create_program();
    int create_shader(GLenum shader_type, int cpu_num);
    int attach(int prog, int shader_cache_id);
};

class glRenderPayload{
    public: 
    glRenderPayload();
    // GL_XXX : ID
    std::map<GLenum, int> renderMap;
    // texture paths
    std::map<GLenum, int> tex_units;
    int shader_program_in_use;
    glProgrammableShader* cur_shader_program_ptr;
};

struct Pixel{
    Pixel():write(false){}
    bool write;
    
    // programmable input
    glm::vec3 vertexColor;
    glm::vec2 texcoord;
    glm::vec3 normal;
    // sync
    omp_lock_t lock;
};


class PrimitiveCache{
public:
    int removeCacheData(unsigned int VAO){
        int ret = indexCache.erase(VAO);
        return ret == 0 ? GL_FALSE : GL_TRUE;
    }

    int getCacheData(unsigned int VAO, std::vector<int> **indices)
    {
        auto it = indexCache.find(VAO);
        if (it == indexCache.end()) {
            return GL_FALSE;
        }
        *indices = &it->second;
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
        // list of programmable triangles
        std::vector<ProgrammableTriangle*> prog_triangle_list;
        int prog_triangle_size;
        // list of programmable triangles
        std::vector<ProgrammableTriangle*> prog_tri_culling_list;
        // the triangles are from view frustum culling
        std::vector<Triangle*> tri_culling_list;
        omp_lock_t tri_culling_lock;

        std::list<render_fp> exec;
        // pixel processing task list
        std::vector<Pixel> pixel_tasks;

        // for calculating bounding box of a mesh
        std::vector<bin_data_t> bin_data;
        // programmable pixel processing task list;
        std::vector<ProgrammablePixel> prog_pixel_tasks;

        // get information that 2x2 pixels near the current pixel;
        std::vector<std::vector<ProgrammablePixel*>> pixel_block;

        // view frustum culling triangles
        std::vector<std::vector<ProgrammableTriangle*>> vfc_list;
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
