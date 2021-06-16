///

#ifndef _glv_h_
#define _glv_h_

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************
 * GLV API tokens
 *************************************************************************/

#define GLV_FALSE       0
#define GLV_TRUE        1

#define GLV_STREAM_FILE    0
#define GLV_STREAM_WINDOW  1
#define GLV_STREAM_NETWORK 2

/*************************************************************************
 * GLV API types
 *************************************************************************/

typedef struct GLVStream GLVStream;

/*************************************************************************
 * GLV API functions
 *************************************************************************/

/**
 * initialize glv environment
 * @return GLV_TRUE or GLV_FALSE
 */
int glvInit(void);

/**
 * The function creates io stream for storing framebuffer result.
 * 
 * @param width image width
 * @param height image height
 * @param name stream name
 * @param type stream type,maybe it is file stream, window stream or network stream
 * 
 * @return io stream
 */
GLVStream* glvCreateStream(int width, int height, const char *name, int type);

/**
 * the function writes framebuffer result in file.
 * 
 * @param stream stream
 * @return GLV_TRUE or GLV_FALSE
 */
int glvWriteStream(GLVStream* stream);

/**
 * release glv memory.
 */
void glvTerminate();

#ifdef __cplusplus
}
#endif 

#endif  /* _glv_h_ */