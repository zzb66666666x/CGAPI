cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 11)
project(glite LANGUAGES CXX C)

include_directories(
	src/gl
	src/glView
	src/utils
	src/utests
)

# library make

# unit tests
add_executable(
	test_globjs 
	src/utests/test_globjs.cpp
	src/utils/id.cpp
)

