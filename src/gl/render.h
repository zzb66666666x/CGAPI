#ifndef _RENDER_H
#define _RENDER_H

#include "configs.h"

typedef void (*render_fp)();

#define PROCESS_VERTEX_THREAD_COUNT 6
#define DOING_VERTEX_PROCESSING 1
#define DOING_RASTERIZATION 2

// single thread interface
void process_geometry();
void process_geometry_ebo();
void rasterize();
void rasterize_ebo();
void process_pixel();
void rasterize_with_shading();

// improved pixel processing APIs
void binning();

// interface multi-thread version
void terminate_all_threads();
void process_geometry_threadmain();
void rasterize_threadmain();
void process_pixel_threadmain();

#endif