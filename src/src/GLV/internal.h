
#ifndef _GLV_internal_h_
#define _GLV_internal_h_


#include "../GL/internal.h"
#include "../../include/GLV/glv.h"

typedef struct{
    gl_context *ctx;
    
}_GLVContext;

typedef struct{

    int width;
    int height;
    char *filename;
    int type;

}_GLVFile;



#endif  /* _internal_h_ */