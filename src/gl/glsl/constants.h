#ifndef _CONSTANTS_H
#define _CONSTANTS_H

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

#endif