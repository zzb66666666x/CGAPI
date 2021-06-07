#ifndef _GLCONTEXT_H
#define _GLCONTEXT_H

typedef struct{


} gl_context;

extern gl_context* ctx;

#define GET_CURRENT_CONTEXT(C) struct gl_context *C = ctx

extern int _cg_create_context();
extern int _cg_make_current();
extern int _cg_init_context();
extern int _cg_free_context_data();

#endif