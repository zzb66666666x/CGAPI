#include "internal.h"
#include "../../include/glv/glv.h"

// hello world of using GLV
_GLVFile foo = {666,6666,"hello there!",66666};
GLVFile* glvCreateFile(int width, int height, char *filename){
    return (GLVFile*)(&foo);
}