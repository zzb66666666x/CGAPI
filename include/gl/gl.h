#ifndef _GL_H
#define _GL_H

#include "common.h"

// helloworld
extern void glHelloWorld();

// Gen
extern void glGenBufffers(int num, int * ID);
extern void glGenVertexArrays(int num, int* ID);
extern void glGenTexture(int num, int* ID);
// Bind
extern void glBindBuffer(GLenum buf_type,  int ID);
extern void glBindVertexArray(int ID);
extern void glBindTexture(GLenum target,  int ID);
// Pass data
extern void glBufferData(GLenum buf_type, int nbytes, const void* data, GLenum usage);
extern void glTexImage2D(GLenum target, int level,  GLenum internalFormat, int width, int height, int border, GLenum format, GLenum type, void * data);
// Attrib
extern void glVertexAttribPointer(int index, int size, GLenum dtype, bool normalized, int stride, void * pointer);
// Enable
extern void glEnableVertexAttribArray(int ID);
extern void glEnable(GLenum cap);
// draw
extern void glDrawArrays(GLenum mode, int first, int count);
// IO
extern void glReadPixels(int x, int y, int width, int height, GLenum format, GLenum type, void* data);
// shader API
extern unsigned int glCreateShader(GLenum shaderType);
extern void glShaderSource(unsigned int shader, int count, char** string, int* length);
extern void glCompileShader(unsigned int shader);
extern unsigned int glCreateProgram();
extern void glAttachShader(unsigned int shaderProgram, unsigned int shader);
extern void glLinkProgram(unsigned int shaderProgram);
extern void glUseProgram(unsigned int shaderProgram);

#endif
