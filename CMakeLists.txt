cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
project(glite LANGUAGES CXX C)

# vcpkg dir settings
set(CMAKE_TOOLCHAIN_FILE ./vcpkg/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")
# set(CMAKE_TOOLCHAIN_FILE D:/CodingLibs/vcpkg/scripts/buildsystems/vcpkg.cmake
#   CACHE STRING "Vcpkg toolchain file")

# flags
# set(CMAKE_CXX_FLAGS "-g -O0")
# set(CMAKE_CXX_FLAGS "-mavx")
# set(CMAKE_CXX_FLAGS "-save-temps")
set(CMAKE_CXX_FLAGS "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS "-fopenmp")

# dir settings
include_directories(
	include/gl
	include/glv
)

set(GL_HEADERS 
	include/gl/gl.h
	include/gl/common.h
	src/gl/glcontext.h
	src/gl/globj.h
	src/gl/render.h
	src/gl/geometry.h
	src/gl/formats.h
	src/gl/configs.h
	src/gl/binning.h
	src/gl/glsl/texture.h
)

set(GLV_HEADERS 
	include/glv/glv.h
	src/glv/internal.h
)

set(GL_SOURCES
	src/gl/glsl/texture.cpp
	src/gl/gl.cpp 
	src/gl/globj.cpp
	src/gl/glcontext.cpp
	src/gl/render.cpp
	src/gl/geometry.cpp
	src/gl/binning.cpp
)

set(GLV_SOURCES
	src/glv/glv.cpp
)

set(UTILS_HEADERS
	src/utils/id.h
)

set(UTILS_SOURCES
	src/utils/id.cpp
)

set(GL_PARSER
	src/gl/glsl/lex.yy.c
	src/gl/glsl/y.tab.h
	src/gl/glsl/y.tab.c
	src/gl/glsl/symbols.h
	src/gl/glsl/symbols.cpp
	src/gl/glsl/translate.h
	src/gl/glsl/translate.cpp
	src/gl/glsl/parse.h
	src/gl/glsl/parse.c
	src/gl/glsl/shader.hpp
	src/gl/glsl/inner_variable.h
	src/gl/glsl/inner_support.h
	src/gl/glsl/vec_math.h
)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_INCLUDE_DIR ./vcpkg/installed/x64-mingw-static/include)
include_directories(${Boost_INCLUDE_DIR})
find_package(Boost REQUIRED)

set(glm_DIR ./vcpkg/installed/x64-mingw-static/share/glm)
find_package(glm REQUIRED)

set(OpenCV_DIR E:/developer/lib/opencv-3.4.13/build/install)
# set(OpenCV_DIR D:/CodingLibs/opencv/mingw_build_release)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(benchmark_DIR ./vcpkg/installed/x64-mingw-static/share/benchmark)
# set(benchmark_DIR D:/CodingLibs/vcpkg/installed/x64-mingw-static/share/benchmark)
find_package(benchmark REQUIRED)

find_package(assimp CONFIG REQUIRED)
if(assimp_FOUND)
link_directories(${ASSIMP_LIBRARY_DIRS})
message(${ASSIMP_LIBRARY_DIRS})
include_directories(${ASSIMP_INCLUDE_DIRS})
message(${ASSIMP_INCLUDE_DIRS})
endif(assimp_FOUND)

################################################
################# library make #################
################################################
add_library(glite SHARED
	${GLV_SOURCES}
	${GL_SOURCES}
	${GL_PARSER}
	${UTILS_SOURCES}
	${GLV_HEADERS}
	${GL_HEADERS}
	${UTILS_HEADERS}
)
target_link_libraries(glite pthread)
target_link_libraries(glite ${OpenCV_LIBRARIES})
target_link_libraries(glite benchmark::benchmark benchmark::benchmark_main)
target_link_libraries(glite ${Boost_LIBRARIES})
target_link_libraries(glite glm::glm)

################################################
################# utest make ###################
################################################
add_executable(test_globjs src/utests/test_globjs.cpp ${GL_SOURCES} ${UTILS_SOURCES})

add_executable(test_pipeline src/utests/test_pipeline.cpp)
target_link_libraries(test_pipeline glite)


add_executable(test_glsl ${GL_PARSER} src/utests/test_glsl.cpp)

add_executable(test_sampler2D_type ${GL_PARSER} src/utests/test_sampler2D_type.cpp)
target_link_libraries(test_sampler2D_type ${OpenCV_LIBRARIES})

add_executable(benchmark_prog_pipeline src/utests/benchmark_prog_pipeline.cpp ${UTESTS_HEADER_ASSIMP})
target_link_libraries(benchmark_prog_pipeline glite)
target_link_libraries(benchmark_prog_pipeline assimp)


################################################
################## test make ###################
################################################
set(TESTS_HEADER_ASSIMP
	tests/header_assimp/configs.h
	tests/header_assimp/mesh.h
	tests/header_assimp/model.h
	tests/header_assimp/shader.h
	tests/header_assimp/camera.h
)

set(TESTS_HEADER
	tests/header/mesh.h
	tests/header/model.h
	tests/header/shader.h
)

set(UTESTS_HEADER_ASSIMP
	src/utests/header_assimp/configs.h
	src/utests/header_assimp/mesh.h
	src/utests/header_assimp/model.h
	src/utests/header_assimp/shader.h
)

add_executable(basics tests/test_basics.cpp tests/shared_data.cpp tests/shared_data.h)
target_link_libraries(basics glite)

add_executable(ebo tests/test_ebo.cpp)
target_link_libraries(ebo glite)

add_executable(texture tests/test_texture.cpp tests/shared_data.cpp tests/shared_data.h)
target_link_libraries(texture glite)

add_executable(bunny tests/test_bunny.cpp)
target_link_libraries(bunny glite)

add_executable(test_benchmark tests/test_benchmark.cpp)
target_link_libraries(test_benchmark glite)

add_executable(test_sponza tests/test_sponza.cpp tests/header/mesh.h tests/header/model.h)
target_link_libraries(test_sponza glite)

# programmable pipeline version of testing files
add_executable(test_prog_sponza tests/test_prog_sponza.cpp tests/header/mesh.h tests/header/model.h tests/header/shader.h)
target_link_libraries(test_prog_sponza glite)

add_executable(test_prog_cube tests/test_prog_cube.cpp tests/header/mesh.h tests/header/model.h tests/header/shader.h)
target_link_libraries(test_prog_cube glite)

add_executable(test_prog_basics tests/test_prog_basics.cpp tests/shared_data.cpp)
target_link_libraries(test_prog_basics glite)

# add_executable(test_prog_cow tests/test_prog_cow.cpp)
# target_link_libraries(test_prog_cow glite)

# add_executable(test_prog_sponza_texture tests/test_prog_sponza_texture.cpp ${TESTS_HEADER_ASSIMP})
# target_link_libraries(test_prog_sponza_texture glite)
# target_link_libraries(test_prog_sponza_texture assimp)