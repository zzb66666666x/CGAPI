#ifndef _RENDER_H
#define _RENDER_H

#include "configs.h"

typedef void (*render_fp)();

#define PROCESS_VERTEX_THREAD_COUNT 1
#define DOING_VERTEX_PROCESSING 1
#define DOING_RASTERIZATION 2

// interface
void process_geometry();
void rasterize();
void process_pixel();

// new interface for parallel
void geometry_processing();
void rasterization();

// interface multi-thread version
void terminate_all_threads();
void process_geometry_threadmain();
void rasterize_threadmain();

#endif