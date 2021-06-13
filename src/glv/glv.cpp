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
    return GLV_TRUE;
}

// create file to demo the rendering process
// automatic bind frame buffer to it
// then if we use glvWritefile, an image will be written to file
GLVFile *glvCreateFile(int width, int height, const char *filename)
{
    if (filename == nullptr || width <= 0 || height <= 0)
        return nullptr;
    int file_len = strlen(filename);
    if (file_len > MAX_FILENAME_LEN - 4)
    {
        printf("The filename is too long!\n");
        return nullptr;
    }
    _GLVFile *curfile = MALLOC(_GLVFile, 1);
    memcpy(curfile->filename ,filename ,file_len);
    
    curfile->filename[file_len] = '.';
    curfile->filename[file_len + 1] = 'b';
    curfile->filename[file_len + 2] = 'm';
    curfile->filename[file_len + 3] = 'p';
    curfile->height = height;
    curfile->width = width;
    curfile->type = FILE_TYPE_BMP;
    gl_context *ctx = _cg_create_context(width, height, false);
    _cg_make_current(ctx);
    return (GLVFile *)(curfile);
}

int glvWriteFile(GLVFile *file)
{
    if (file == nullptr)
        return GLV_FALSE;
    _GLVFile *_file = (_GLVFile *)file;
    GET_CURRENT_CONTEXT(C);
    color_t *framebuf_data = (color_t *)C->framebuf->getDataPtr();
    int total_size = C->framebuf->getSize();

    int w = _file->width;
    int h = _file->height;
    unsigned char *img = MALLOC(unsigned char, w * h * 3);
    for (int i = 0; i < total_size; ++i)
    {
        img[i * 3 + 2] = framebuf_data[i].R;
        img[i * 3 + 1] = framebuf_data[i].G;
        img[i * 3] = framebuf_data[i].B;
    }
    int l = (w * 3 + 3) / 4 * 4;
    int bmi[] = {l * h + 54, 0, 54, 40, w, h, 1 | 3 * 8 << 16, 0, l * h, 0, 0, 100, 0};
    FILE *fp = fopen(_file->filename, "wb");
    fprintf(fp, "BM");
    fwrite(&bmi, 52, 1, fp);
    fwrite(img, 1, l * h, fp);
    fclose(fp);
    free(img);
    return GLV_TRUE;
}

void glvTerminate()
{
    _cg_free_context_data();
}
