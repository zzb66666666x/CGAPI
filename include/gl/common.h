#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>

#define GL_FAILURE              -1
#define GL_SUCCESS               0

typedef enum{
GL_UNDEF = -1,
// object types (type)
GLOBJ_VERTEX_BUFFER,
GLOBJ_VERTEX_ATTRIB,
GLOBJ_FRAMEBUF,
GLOBJ_ZBUF,
// GLOBJ_PLACE_HOLDER,
// input mode
GL_ARRAY_BUFFER,
GL_BIND_VAO,
GL_ELEMENT_ARRAY_BUFFER,
// output draw mode
GL_TRIANGLE,
// target
GL_TEXTURE_2D, 

//fotmat
GL_RGB,

// usage
GL_STATIC_DRAW,

//data types (dtype)
GL_FLOAT,
GL_INT,
GL_BYTE,
GL_UNSIGNED_BYTE,
GL_VERTEX_ATTRIB_CONFIG,

//shader
GL_VERTEX_SHADER,
GL_FRAGMENT_SHADER,

// framebuf
GL_FRAMEBUFFER,
GL_FRAMEBUFFER_ATTACH_ZBUF,
// capbilities
GL_DEPTH_TEST

}GLenum;

#define THREAD_NUM  7
#define DEFAULT_VERTEX_ATTRIB_NUM   16

#endif 