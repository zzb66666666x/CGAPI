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
GL_PLACE_HOLDER,
// mode
GL_ARRAY_BUFFER,
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

//shader
GL_VERTEX_SHADER,
GL_FRAGMENT_SHADER,

// capbilities
GL_DEPTH_TEST

}GLenum;

#define THREAD_NUM  7

#endif 