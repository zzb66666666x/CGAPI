add_subdirectory(glsl_math)
add_subdirectory(hello)

link_directories(${PROJECT_SOURCE_DIR}/lib)
include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(main main.cpp)
target_link_libraries(main tcc)
target_link_libraries(main glsl_math)

