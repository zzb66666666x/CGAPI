#ifndef _GL_H
#define _GL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

// helloworld
extern void glHelloWorld();

// Gen
extern void glGenBuffers(int num, unsigned int * ID);
extern void glGenVertexArrays(int num, unsigned int* ID);
extern void glGenTextures(int num, unsigned int* ID);
extern void glGenFramebuffers(int num, unsigned int* ID);
// Bind & Activate
extern void glBindBuffer(GLenum buf_type, unsigned int ID);
extern void glBindVertexArray(unsigned int ID);
extern void glBindTexture(GLenum target, unsigned int ID);
extern void glBindFramebuffer(GLenum target, unsigned int ID);
extern void glActiveTexture(GLenum unit);
extern void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, unsigned int texture, int level);
// Pass data
extern void glBufferData(GLenum buf_type, int nbytes, const void* data, GLenum usage);
extern void glTexImage2D(GLenum target, int level,  GLenum internalFormat, int width, int height, int border, GLenum format, GLenum type, void * data);
// Attrib
extern void glVertexAttribPointer(int index, int size, GLenum dtype, bool normalized, int stride, void * pointer);
// Tex Params
extern void glTexParameterf(GLenum target,GLenum pname,float param);
extern void glTexParameteri(GLenum target,GLenum pname,int param);
extern void glTexParameterfv(GLenum target,GLenum pname,const float * params);
extern void glTexParameteriv(GLenum target,GLenum pname,const int * params);
// Enable
extern void glEnableVertexAttribArray(unsigned int idx);
extern void glEnable(GLenum cap);
// draw
extern void glDrawBuffer(GLenum mode);
extern void glReadBuffer(GLenum mode);
extern void glViewport(int x, int y, int width, int height);
extern void glDrawArrays(GLenum mode, int first, int count);
extern void glDrawElements(GLenum mode, int count, unsigned int type, const void* indices);
extern void glClearColor(float R, float G, float B, float A);
extern void glClear(unsigned int bitfields);
// cull face
extern void glCullFace(unsigned int mode);
extern void glFrontFace(unsigned int mode);
// IO
extern void glReadPixels(int x, int y, int width, int height, GLenum format, GLenum type, void* data);
// shader API
extern unsigned int glCreateShader(GLenum shaderType);
extern void glShaderSource(unsigned int shader, int count, const char* const* string, const int* length);
extern void glCompileShader(unsigned int shader);
extern unsigned int glCreateProgram();
extern void glAttachShader(unsigned int shaderProgram, unsigned int shader);
extern void glLinkProgram(unsigned int shaderProgram);
extern void glUseProgram(unsigned int shaderProgram);
extern void glUniform1i(int location, int val);
extern void glUniformMatrix4fv(int location, int count, bool transpose, const float * value);
extern void glUniform3fv(int location, int count, const float* value);
extern int glGetUniformLocation(unsigned int program, const char* name);
// debug API
extern void glDebugflag(bool debug_flag);

#ifdef __cplusplus
}
#endif 

#endif
