#include <vector>
#include <stdio.h>
#include <string.h>

#include "internal.h"
#include "../../include/glv/glv.h"
#include "../gl/glcontext.h"
#include "../gl/globj.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

//////////////////////////////////////////
///////// window stream api //////////////
//////////////////////////////////////////
static _GLVStream *create_window_stream(int width, int height, const char *name)
{
    int title_len = strlen(name);
    if (title_len > MAX_NAME_LEN)
    {
        printf("The filename is too long!\n");
        return NULL;
    }
    _GLVStream *window = MALLOC(_GLVStream, 1);
    memcpy(window->name, name, title_len);

    window->height = height;
    window->width = width;
    window->type = GLV_STREAM_WINDOW;
    gl_context *ctx = _cg_create_context(width, height, true);
    _cg_make_current(ctx);
    _glvContext->ctx = ctx;
    _glvContext->curStream = window;
    return window;
}

static int write_window_stream(_GLVStream *_window)
{
    gl_context *ctx = _glvContext->ctx;
    
    color_t *framebuf_data = (color_t *)ctx->framebuf->getDataPtr();
    // int total_size = ctx->framebuf->getSize();

    int w = _window->width;
    int h = _window->height;

    cv::Mat frame(h, w, CV_32FC3, framebuf_data);
    frame.convertTo(frame, CV_8UC3, 1.0f);
    cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
    cv::imshow(_window->name, frame);
    cv::waitKey(10);
    if(ctx->use_double_buf){
        _cg_swap_framebuffer(ctx);
    }
    return GLV_TRUE;
}

//////////////////////////////////////////
///////// file stream api ////////////////
//////////////////////////////////////////
static _GLVStream *create_file_stream(int width, int height, const char *name)
{
    int filename_len = strlen(name);
    if (filename_len > MAX_NAME_LEN - 5)
    {
        printf("The filename is too long!\n");
        return NULL;
    }
    _GLVStream *file = MALLOC(_GLVStream, 1);
    memcpy(file->name, name, filename_len);

    file->name[filename_len] = '.';
    file->name[filename_len + 1] = 'b';
    file->name[filename_len + 2] = 'm';
    file->name[filename_len + 3] = 'p';
    file->name[filename_len + 4] = '\0';
    file->height = height;
    file->width = width;
    file->type = GLV_STREAM_FILE;
    gl_context *ctx = _cg_create_context(width, height, false);
    _cg_make_current(ctx);
    _glvContext->ctx = ctx;
    _glvContext->curStream = file;
    return file;
}

static int write_file_stream(_GLVStream *_file)
{
    gl_context *ctx = _glvContext->ctx;
    color_t *framebuf_data = (color_t *)ctx->framebuf->getDataPtr();
    // int total_size = ctx->framebuf->getSize();

    int w = _file->width;
    int h = _file->height;

    // to generate a bmp image.
    unsigned char *img = MALLOC(unsigned char, w * h * 3);
    int index = 0;
    for (int y = h - 1; y >= 0; --y)
    {
        for (int x = 0; x < w; ++x)
        {
            img[index * 3] = framebuf_data[y * w + x].B;
            img[index * 3 + 1] = framebuf_data[y * w + x].G;
            img[index * 3 + 2] = framebuf_data[y * w + x].R;
            ++index;
        }
    }
    int l = (w * 3 + 3) / 4 * 4;
    int bmi[] = {l * h + 54, 0, 54, 40, w, h, 1 | 3 * 8 << 16, 0, l * h, 0, 0, 100, 0};
    FILE *fp = fopen(_file->name, "wb");
    fprintf(fp, "BM");
    fwrite(&bmi, 52, 1, fp);
    fwrite(img, 1, l * h, fp);
    fclose(fp);
    FREE(img);
    return GLV_TRUE;
}


/////////////////////////////////////////////////////
/////////////////// glv api /////////////////////////
/////////////////////////////////////////////////////
int glvInit(void)
{
    _glvContext = MALLOC(_GLVContext, 1);
    memset(_glvContext, 0, sizeof(_GLVContext));
    return GLV_TRUE;
}

GLVStream *glvCreateStream(int width, int height, const char *name, int type)
{
    if (name == NULL || width <= 0 || height <= 0)
        return NULL;

    // judge the type of stream.
    _GLVStream *stream = NULL;
    if (type == GLV_STREAM_FILE){
        stream = create_file_stream(width, height, name);
    }
    else if (type == GLV_STREAM_WINDOW){
        stream = create_window_stream(width, height, name);
    }else if (type == GLV_STREAM_NETWORK){
        // TODO to be continue...
    }

    return (GLVStream *)(stream);
}

int glvWriteStream(GLVStream *stream)
{
    if (stream == NULL)
        return GLV_FALSE;
    _GLVStream *_stream = (_GLVStream*) stream;
    if(_stream->type == GLV_STREAM_FILE){
        return write_file_stream(_stream);
    }else if(_stream->type == GLV_STREAM_WINDOW){
        return write_window_stream(_stream);
    }
    
    return GLV_FALSE;
}

void glvTerminate()
{
    _cg_free_context_data(_glvContext->ctx);
    FREE(_glvContext->curStream);
    FREE(_glvContext);
}