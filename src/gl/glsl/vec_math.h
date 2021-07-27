#ifndef _VEC_MATH_H
#define _VEC_MATH_H

#define GLM_FORCE_AVX2
#define GLM_FORCE_INLINE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

union data_t{
    glm::vec2 vec2_var;
    glm::vec3 vec3_var;
    glm::vec4 vec4_var;
    glm::mat2 mat2_var;
    glm::mat3 mat3_var;
    glm::mat4 mat4_var;
    // if you want to pass int, still use the field sampler2D_var, they can share
    union{
        int sampler2D_var;
        int int_var;
    };
};

#endif