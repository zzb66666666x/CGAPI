#ifndef _GLSL_MATH_H
#define _GLSL_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

// basic types
typedef float   vec2[2];
typedef float   vec3[3];
typedef int     ivec3[3];
typedef float   vec4[4];
typedef vec3    mat3[3];
typedef vec2    mat2[2];
typedef vec4    mat4[4];

// struct defs
typedef union{
    struct{
        float x;
        float y;
    };
    vec2 raw;
}vec2s;

typedef union{
    struct{
        float x;
        float y;
        float z;
    };
    vec3 raw;
}vec3s;

typedef union{
    struct{
        float x;
        float y;
        float z;
        float w;
    };
    vec4 raw;
}vec4s;

typedef union{
    mat2 raw;
    vec2s col[2];
    struct {
        float m00, m01;
        float m10, m11;
    };
}mat2s;

typedef union{
    mat3 raw;
    vec3s col[3];
    struct {
        float m00, m01, m02;
        float m10, m11, m12;
        float m20, m21, m22;
    };
}mat3s;

typedef union{
  mat4  raw;
  vec4s col[4];
  struct {
    float m00, m01, m02, m03;
    float m10, m11, m12, m13;
    float m20, m21, m22, m23;
    float m30, m31, m32, m33;
  };
} mat4s;

extern void mat4_mulv(mat4 m, vec4 v, vec4 dest);

#ifdef __cplusplus
}
#endif

#endif