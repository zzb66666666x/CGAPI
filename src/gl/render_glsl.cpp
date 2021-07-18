#include "render_glsl.h"
#include "../../include/gl/common.h"
#include "glsl/shader.hpp"
#include "glcontext.h"
#include <assert.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <omp.h>
#include <stdio.h>

#define GET_PIPELINE(P) glPipeline* P = &(glapi_ctx->pipeline)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define GET_INDEX(x, y, width, height) ((height - 1 - y) * width + x)

void programmable_process_geometry_openmp(){

}

void programmable_rasterize_with_shading_openmp(){

}