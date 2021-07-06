#ifndef _RENDER_H
#define _RENDER_H

#include "configs.h"

typedef void (*render_fp)();

#define PROCESS_VERTEX_THREAD_COUNT 6
#define RASTERIZE_THREAD_COUNT      8
#define BINNING_THREAD_COUNT        10
#define DOING_VERTEX_PROCESSING     1
#define DOING_BINNING               2
#define DOING_RASTERIZATION         3
#define DOING_PIXEL_SHADING         4
#define PIPELINE_FINISH             5

// single thread interface
void process_geometry();
void process_geometry_ebo();
void rasterize();
void rasterize_ebo();
void process_pixel();
void rasterize_with_shading();

// improved pixel processing APIs
void binning_threadmain();

// interface multi-thread version
void terminate_all_threads();
void process_geometry_threadmain();
void rasterize_threadmain();
void process_pixel_threadmain();

#endif