#ifndef _GL_H
#define _GL_H

#include "common.h"

extern void glGenBufffers(int num, int * ID);
extern void glGenVertexArrays(int num, int* ID);
extern void glGenTexture(int num, int* ID);

extern void glBindBuffer(GLenum buf_type,  int ID);
extern void glBindVertexArray(int ID);
extern void glBindTexture(GLenum target,  int ID);

extern void glBufferData(GLenum buf_type, int nbytes, const void* data, GLenum usage);
extern void glTexImage2D(GLenum target, int level,  GLenum internalFormat, int width, int height, int border, GLenum format, GLenum type, void * data);

extern void glVertexAttribPointer(int index, int size, GLenum dtype, bool normalized, int stride, void * pointer);

extern void glEnableVertexAttribArray(int ID);

extern void glDrawArrays(GLenum mode, int first, int count);

extern void glReadPixels(int x, int y, int width, int height, GLenum format, GLenum type, void* data);

#endif
