#ifndef _VEC_MATH_H
#define _VEC_MATH_H

#define GLM_FORCE_AVX2
#define GLM_FORCE_INLINE
#include <glm/glm.hpp>

typedef union{
    glm::vec2 vec2_var;
    glm::vec3 vec3_var;
    glm::vec4 vec4_var;
    glm::mat2 mat2_var;
    glm::mat3 mat3_var;
    glm::mat4 mat4_var;
}data_t;

typedef void (*set_uniform)(data_t);
typedef data_t (*get_uniform)();

#endif