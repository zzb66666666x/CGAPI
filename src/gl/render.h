#ifndef _RENDER_H
#define _RENDER_H

#include "configs.h"

typedef void (*render_fp)();

// interface
void process_geometry();
void rasterize();
void process_pixel();

// interface multi-thread version
void terminate_all_threads();
void process_geometry_threadmain();
void rasterize_threadmain();

#endif