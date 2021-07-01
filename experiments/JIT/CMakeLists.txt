cmake_minimum_required(VERSION 3.0.0)
project(main)

include_directories(./include)
link_directories(./lib)
add_executable(main main.cpp support.h)
target_link_libraries(main tcc)