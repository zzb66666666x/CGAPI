#include <vector>
#include <stdio.h>
#include <string.h>
#include "internal.h"
#include "../../include/glv/glv.h"
#include "../gl/glcontext.h"
#include "../gl/globj.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <chrono>

#define A_to_a  32

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

//////////////////////////////////////////
///////// internal variable //////////////
//////////////////////////////////////////
_GLVContext *_glvContext = nullptr;

//////////////////////////////////////////
///////// window stream api //////////////
//////////////////////////////////////////
static _GLVStream *create_window_stream(int width, int height, const char *name)
{
    _GLVStream *window = new _GLVStream;
    window->name = name;

    window->height = height;
    window->width = width;
    window->type = GLV_STREAM_WINDOW;
    window->mouse_move = nullptr;
    window->mouse_scroll = nullptr;
    window->should_exit_flag = 0;
    gl_context *ctx = _cg_create_context(width, height, false);
    _cg_make_current(ctx);
    _glvContext->ctx = ctx;
    _glvContext->curStream = window;
    cv::namedWindow(window->name);
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
    int key = cv::waitKey(1);
    _window->keyinput = key;
    if (key == GLV_KEY_ESCAPE || cv::getWindowProperty(_glvContext->curStream->name, cv::WND_PROP_VISIBLE) == 0){
        _window->should_exit_flag = 1;
        return GLV_FALSE;
    }
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
    _GLVStream *file = new _GLVStream;
    file->name = std::string(name) + std::string(".bmp");

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
    FILE *fp = fopen(_file->name.c_str(), "wb");
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
    cv::destroyWindow(_glvContext->curStream->name);
    _cg_free_context_data(_glvContext->ctx);
    FREE(_glvContext->curStream);
    FREE(_glvContext);
}

int glvWindowShouldClose(GLVStream* stream){
    if (stream == NULL)
        return GLV_FALSE;
    _GLVStream *_stream = (_GLVStream*) stream;
    if (_stream->type != GLV_STREAM_WINDOW){
        return GLV_FALSE;
    }
    if (_stream->should_exit_flag == 1){
        return GLV_TRUE;
    }
    return GLV_FALSE;
}

inline bool should_process_mouse(){
    if (_glvContext == nullptr)
        return false;
    if (_glvContext->curStream == nullptr || _glvContext->ctx == nullptr)   
        return false;
    if (_glvContext->curStream->type != GLV_STREAM_WINDOW)
        return false;
    return true;
}

void on_mouse(int event, int x, int y, int flags, void*)
{
    if (!should_process_mouse())
        return;
    _GLVStream* window = _glvContext->curStream;
    double value;
    float step= 1.0f;
    switch (event) {
    case cv::EVENT_MOUSEWHEEL:
        value = cv::getMouseWheelDelta(flags);
        if (value >= 0){
            window->mouse_scroll((GLVStream*)window, 0, step);
        }else{
            window->mouse_scroll((GLVStream*)window, 0, -step);
        }
        break;

    case cv::EVENT_MOUSEMOVE:
        window->mouse_move((GLVStream*)window, (float)x, (float)y);
        break;
    default:
        break;
    }
}

GLVcursorposfun glvSetCursorPosCallback(GLVStream* stream, GLVcursorposfun callback){
    if (stream == NULL)
        return nullptr;
    _GLVStream *_stream = (_GLVStream*) stream;
    if (_stream->type == GLV_STREAM_WINDOW){
        GLVcursorposfun ret = _stream->mouse_move;
        _stream->mouse_move = callback;
        cv::setMouseCallback(_stream->name, on_mouse, NULL);
        return ret;
    }else{
        return nullptr;
    }
}

GLVscrollfun glvSetScrollCallback(GLVStream* stream, GLVscrollfun callback){
    if (stream == NULL)
        return nullptr;
    _GLVStream *_stream = (_GLVStream*) stream;
    if (_stream->type == GLV_STREAM_WINDOW){
        GLVscrollfun ret = _stream->mouse_scroll;
        _stream->mouse_scroll = callback;
        cv::setMouseCallback(_stream->name, on_mouse, NULL);
        return ret;
    }else{
        return nullptr;
    }
}

int glvGetKey(GLVStream* stream, int key){
    if (stream == NULL)
        return 0;
    _GLVStream *_stream = (_GLVStream*) stream;
    if (_stream->type != GLV_STREAM_WINDOW)
        return 0;
    // std::cout<<_stream->keyinput<<"    "<<key + A_to_a<<std::endl;
    if (key >= (int)'A' && key <= (int)'Z')
        return (int)(_stream->keyinput == key || _stream->keyinput == key + A_to_a);
    else
        return (int)(_stream->keyinput == key);
}

void glvSetWindowShouldClose(GLVStream* stream, bool flag){
    if (stream == NULL)
        return;
    _GLVStream *_stream = (_GLVStream*) stream;
    _stream->should_exit_flag = flag;
}

float glvGetTime(){
    static int first_entry = 1;
    static int64_t begin_time;
    int64_t millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    if (first_entry){
        begin_time = millisec_since_epoch;
        first_entry = 0;
    }
    float ret = (float)(millisec_since_epoch-begin_time)/1000.0f;
    return ret;
}
