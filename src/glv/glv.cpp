#include <vector>
#include <stdio.h>
#include <string.h>
#include "internal.h"
#include "../../include/glv/glv.h"
#include "../../include/gl/common.h"
#include "../gl/glcontext.h"
#include "../gl/globj.h"

int glvInit(void)
{
    for (int i=0; i<MAX_GLVFILE_NUM; i++){
        glv_file_usage[i] = false;
        glv_files[i] = nullptr;
    }
    _glv_file_current = nullptr;
    return GLV_TRUE;
}

// create file to demo the rendering process
// automatic bind frame buffer to it
// then if we use glvWritefile, an image will be written to file
GLVFile *glvCreateFile(int width, int height, const char *filename)
{
    if (filename == nullptr || width <= 0 || height <= 0)
        return nullptr;
    int filename_len = strlen(filename);
    if (filename_len > MAX_FILENAME_LEN - 5)
    {
        printf("The filename is too long!\n");
        return nullptr;
    }
    int slot = _glv_files_free_slot();
    if (slot<0)
        return nullptr;
    _GLVFile *file = MALLOC(_GLVFile, 1);
    memcpy(file->outFile.filename, filename, filename_len);
    file->outFile.filename[filename_len] = '.';
    file->outFile.filename[filename_len + 1] = 'b';
    file->outFile.filename[filename_len + 2] = 'm';
    file->outFile.filename[filename_len + 3] = 'p';
    file->outFile.filename[filename_len + 4] = '\0';
    file->outFile.height = height;
    file->outFile.width = width;
    file->outFile.type = FILE_TYPE_BMP;
    gl_context *ctx = _cg_create_context(width, height, false);
    file->ctx = ctx;
    glv_file_usage[slot] = true;
    glv_files[slot] = file;
    return (GLVFile *)(file);
}

int glvMakeFileCurrent(GLVFile* file){
    if (file == nullptr)
        return GLV_FALSE;
    _GLVFile *_file = (_GLVFile *)file;
    _cg_make_current(_file->ctx);
    _glv_file_current = _file;
    return GLV_TRUE;
}

int glvWriteFile(GLVFile *file)
{
    if (file == nullptr)
        return GLV_FALSE;
    _GLVFile *_file = (_GLVFile *)file;
    GET_CURRENT_CONTEXT(C);
    if (C==nullptr)
        throw std::runtime_error("YOU DO NOT HAVE CURRENT CONTEXT\n");
    if (_file->ctx != C)
        throw std::runtime_error("you cannot make current context target this file as output\n");
    color_t *framebuf_data = (color_t *)C->framebuf->getDataPtr();
    int total_size = C->framebuf->getSize();

    int w = _file->outFile.width;
    int h = _file->outFile.height;

    // to generate a bmp image.
    unsigned char *img = MALLOC(unsigned char, w *h * 3);
    for (int i = 0; i < total_size; ++i)
    {
        img[i * 3 + 2] = framebuf_data[i].R;
        img[i * 3 + 1] = framebuf_data[i].G;
        img[i * 3] = framebuf_data[i].B;
    }
    int l = (w * 3 + 3) / 4 * 4;
    int bmi[] = {l * h + 54, 0, 54, 40, w, h, 1 | 3 * 8 << 16, 0, l * h, 0, 0, 100, 0};
    FILE *fp = fopen(_file->outFile.filename, "wb");
    fprintf(fp, "BM");
    fwrite(&bmi, 52, 1, fp);
    fwrite(img, 1, l * h, fp);
    fclose(fp);
    FREE(img);
    return GLV_TRUE;
}

void glvTerminate()
{
    for (int i=0; i<MAX_GLVFILE_NUM; i++){
        if (glv_files[i] != nullptr){
            _cg_free_context_data(glv_files[i]->ctx);
            delete glv_files[i];
            glv_files[i] = nullptr;
        }
        glv_file_usage[i] = false;
    }
    _glv_file_current = nullptr;
    _cg_reset_current_context();
}
