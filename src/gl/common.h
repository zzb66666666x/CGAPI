#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>

#define GL_UNDEF                -1
#define GL_FAILURE              -1
#define GL_SUCCESS               0

typedef enum{
// objects
GLOBJ_VERTEX_BUFFER,
GLOBJ_VERTEX_ATTRIB,
GLOBJ_FRAMEBUF,
 GLOBJ_ZBUF ,
//  mode
GL_ARRAY_BUFFER ,
GL_TRIANGLE ,
//target
GL_TEXTURE_2D, 

//fotmat
GL_RGB,

// usage
GL_STATIC_DRAW,

//types
GL_FLOAT,
GL_INT,
GL_UNSIGNED_BYTE,

//shader
GL_VERTEX_SHADER,
GL_FRAGMENT_SHADER,

// capbilities
GL_DEPTH_TEST

}GLenum;

typedef int8_t byte;

#endif 