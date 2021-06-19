#ifndef _RENDER_H
#define _RENDER_H

typedef void (*render_fp)();

// interface
void process_geometry();
void rasterize();
void process_pixel();

// interface multi-thread version
void terminate_all_threads();
void process_geometry_threadmain();

// thread task functions
void* _thr_process_vertex(void* thread_id);
void* _thr_rasterize(void* thread_id);

#endif