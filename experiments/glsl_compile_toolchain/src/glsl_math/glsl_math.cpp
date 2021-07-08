#include <stdio.h>
#include "../../include/glsl_math.h"

// supportive math lib
#define GLM_FORCE_AVX2
#define GLM_FORCE_INLINE 
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

void mat4_mulv(mat4 m, vec4 v, vec4 dest){
    glm::mat4 glm_m4 = glm::make_mat4((float *)m);
    glm::vec4 glm_v4 = glm::make_vec4((float *)v);
    float * ans = (float *)glm::value_ptr(glm_m4 * glm_v4);
    // printf("%f, %f, %f, %f\n", ans[0], ans[1], ans[2], ans[3]);
    memcpy(dest, ans, sizeof(float)*4);
}

// g++ glsl_math.cpp -o test
// int main(){
//     // transposed matrix since glm uses col major
//     mat4 m = {1,5,9,13,
//               2,6,10,14,
//               3,7,11,15,
//               4,8,12,16};
//     vec4 v = {1,2,3,4};
//     vec4 dest;
//     mat4_mulv(m, v, dest);
//     printf("%f, %f, %f, %f\n", dest[0], dest[1], dest[2], dest[3]);
//     return 0;
// }