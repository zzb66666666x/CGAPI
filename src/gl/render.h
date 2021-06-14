#ifndef _RENDER_H
#define _RENDER_H

typedef void (*render_fp)();

// interface
void process_vertex();
void assemble_triangle();
void rasterize();
void calculate_color();

// thread task functions
void* _thr_process_vertex(void* thread_id);
void* _thr_rasterize(void* thread_id);

#endif