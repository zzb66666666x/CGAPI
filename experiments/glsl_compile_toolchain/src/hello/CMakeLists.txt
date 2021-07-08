set(CMAKE_C_STANDARD 99)
add_library(hello STATIC hello.c ${PROJECT_SOURCE_DIR}/include/hello.h)