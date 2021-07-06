#ifndef _COMMON_H
#define _COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define GL_FAILURE              -1
#define GL_SUCCESS               0
#define GL_FALSE                 0
#define GL_TRUE                  1

#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_COLOR_BUFFER_BIT 0x00004000

// data type
#define GL_UNSIGNED_INT 0x1405

// texture wrapping parameters
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_REPEAT 0x2901
#define GL_MIRRORED_REPEAT 0x8370
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_CLAMP_TO_BORDER 0x812D

// texture filtering parameters
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801

typedef enum{
GL_UNDEF = -1,

// input mode
GL_ARRAY_BUFFER,
GL_BIND_VAO,
GL_ELEMENT_ARRAY_BUFFER,
// output draw mode
GL_TRIANGLES,
// target
GL_TEXTURE_2D, 

//fotmat
GL_RGB,
GL_RGBA,

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
GL_DEPTH_TEST,
GL_CULL_FACE,

// texture units
GL_TEXTURE0,
GL_TEXTURE1,
GL_TEXTURE2,
GL_TEXTURE3,
GL_TEXTURE4,
GL_TEXTURE5,
GL_TEXTURE6,
GL_TEXTURE7,
GL_TEXTURE8,
GL_TEXTURE9,
GL_TEXTURE10,
GL_TEXTURE11,
GL_TEXTURE12,
GL_TEXTURE13,
GL_TEXTURE14,
GL_TEXTURE15

}GLenum;

#define THREAD_NUM  16
#define GL_MAX_VERTEX_ATTRIB_NUM    16
#define GL_MAX_TEXTURE_UNITS        16

#ifdef __cplusplus
}
#endif 

#endif 