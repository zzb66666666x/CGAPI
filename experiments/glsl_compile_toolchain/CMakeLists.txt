cmake_minimum_required(VERSION 3.0.0)
project(glsl_with_tcc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(src)