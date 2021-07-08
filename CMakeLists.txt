cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_TOOLCHAIN_FILE ./vcpkg/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")
set(CMAKE_CXX_STANDARD 11)
project(glite LANGUAGES CXX C)
# set(CMAKE_CXX_FLAGS "-g -O0")
# set(CMAKE_CXX_FLAGS "-mavx")
# set(CMAKE_CXX_FLAGS "-save-temps")
# set(CMAKE_CXX_FLAGS "-O3")
# use openmp
set(CMAKE_CXX_FLAGS "-fopenmp -mavx -O3")
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

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_INCLUDE_DIR ./vcpkg/installed/x64-mingw-static/include)
include_directories(${Boost_INCLUDE_DIR})
find_package(Boost REQUIRED)

set(glm_DIR ./vcpkg/installed/x64-mingw-static/share/glm)
find_package(glm REQUIRED)

set(OpenCV_DIR E:/developer/lib/opencv-3.4.13/build/install)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(benchmark_DIR ./vcpkg/installed/x64-mingw-static/share/benchmark)
find_package(benchmark REQUIRED)
find_package(OpenMP REQUIRED)

################################################
################# library make #################
################################################
add_library(glite SHARED 	
	${GLV_SOURCES}
	${GL_SOURCES}
	${UTILS_SOURCES}
	${GLV_HEADERS}
	${GL_HEADERS}
	${UTILS_HEADERS}
)
target_link_libraries(glite pthread)
target_link_libraries(glite ${OpenCV_LIBRARIES})
target_link_libraries(glite OpenMP::OpenMP_CXX)
# target_link_libraries(glite ${Boost_LIBRARIES})
# target_link_libraries(glite glm::glm)

################################################
################# utest make ###################
################################################
add_executable(
	test_globjs 
	src/utests/test_globjs.cpp
	${GL_SOURCES}
	${UTILS_SOURCES}
)
target_link_libraries(test_globjs pthread)
target_link_libraries(test_globjs OpenMP::OpenMP_CXX)

################################################
################## test make ###################
################################################
add_executable(basics tests/test_basics.cpp tests/shared_data.cpp tests/shared_data.h)
target_link_libraries(basics glite)

add_executable(ebo tests/test_ebo.cpp)
target_link_libraries(ebo glite)

add_executable(cow tests/test_cow.cpp)
target_link_libraries(cow glite)

add_executable(texture tests/test_texture.cpp tests/shared_data.cpp tests/shared_data.h)
target_link_libraries(texture glite)

add_executable(bunny tests/test_bunny.cpp)
target_link_libraries(bunny glite)
