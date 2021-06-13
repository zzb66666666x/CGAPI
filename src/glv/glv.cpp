#include <vector>
#include <stdio.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include "internal.h"
#include "../../include/glv/glv.h"
#include "../../include/gl/common.h"
#include "../gl/glcontext.h"
#include "../gl/globj.h"

// hello world of using GLV
_GLVFile foo = {666,6666,"hello there!",66666};

// container of files
#define MAX_FILENAME_LEN 100
_GLVFile curfile;
char libfilename[MAX_FILENAME_LEN];

int glvInit(void){
    return GLV_TRUE;
}

// create file to demo the rendering process
// automatic bind frame buffer to it
// then if we use glvWritefile, an image will be written to file
GLVFile* glvCreateFile(int width, int height, const char *filename){
    if (filename == nullptr || width<=0 || height <= 0)
        return nullptr;
    if (strlen(filename)>MAX_FILENAME_LEN){
        printf("The filename is too long!\n");
        return nullptr;
    }
    strcpy(libfilename, filename);
    curfile.filename = libfilename;
    curfile.height = height;
    curfile.width = width;
    curfile.type = FILE_TYPE_UNDEF;
    gl_context* ctx = _cg_create_context(width, height, false);
    _cg_make_current(ctx);
    return (GLVFile*)(&curfile);
}

int glvWriteFile(GLVFile* file){
    if (file == nullptr)
        return GLV_FALSE;
    _GLVFile* _file = (_GLVFile*) file;
    GET_CURRENT_CONTEXT(C);
    color_t * framebuf_data = (color_t *)C->framebuf->getDataPtr();
    int total_size = C->framebuf->getSize();
    for (int i=total_size-1; i<total_size; i++){
        printf("%d: %f, %f, %f\n", i, framebuf_data[i].R, framebuf_data[i].G, framebuf_data[i].B);
    }
    // opencv 
    int w = _file->width;
    int h = _file->height;
    printf("%d, %d\n", w, h);
    cv::Mat image(w, h, CV_32FC3, framebuf_data);
    image.convertTo(image, CV_8UC3, 1.0f);
    cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
    cv::imwrite(_file->filename, image);
    return GLV_TRUE;
}

void glvTerminate(){
    _cg_free_context_data();
    curfile.filename = nullptr;
    curfile.type = FILE_TYPE_UNDEF;
    curfile.height = 0;
    curfile.width = 0;
}




