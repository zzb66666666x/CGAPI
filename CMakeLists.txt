cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 11)
project(glite LANGUAGES CXX C)

# include_directories(
# 	include/gl
# 	include/glv
# )

set(GL_HEADERS 
	include/gl/gl.h
	include/gl/common.h
	src/gl/glcontext.h
	src/gl/globj.h
	src/gl/render.h
	src/gl/geometry.h
)

set(GLV_HEADERS 
	include/glv/glv.h
	src/glv/internal.h
)

set(GL_SOURCE
	src/gl/gl.cpp 
	src/gl/glcontext.cpp
	src/gl/render.cpp
)

set(GLV_SOURCE
	src/glv/glv.cpp
)

################################################
################# library make #################
################################################

# foo lib, hello world
add_library(glhello SHARED 	
	${GLV_SOURCE}
	${GL_SOURCE}
	${GLV_HEADERS}
	${GL_HEADERS}
)

################################################
################# utest make ###################
################################################
add_executable(
	test_globjs 
	src/utests/test_globjs.cpp
	src/utils/id.cpp
)

################################################
################## test make ###################
################################################
add_executable(basic_lib_usage tests/basic_lib_usage.cpp)
target_link_libraries(basic_lib_usage glhello)