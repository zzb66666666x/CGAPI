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

set(GL_SOURCES
	src/gl/gl.cpp 
	src/gl/globj.cpp
	src/gl/glcontext.cpp
	src/gl/render.cpp
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

################################################
################# library make #################
################################################

# foo lib, hello world
add_library(glhello SHARED 	
	${GLV_SOURCES}
	${GL_SOURCES}
	${UTILS_SOURCES}
	${GLV_HEADERS}
	${GL_HEADERS}
	${UTILS_HEADERS}
)

################################################
################# utest make ###################
################################################
add_executable(
	test_globjs 
	src/gl/globj.cpp
	src/utests/test_globjs.cpp
	src/utils/id.cpp
)

################################################
################## test make ###################
################################################
add_executable(basic_lib_usage tests/basic_lib_usage.cpp)
target_link_libraries(basic_lib_usage glhello)