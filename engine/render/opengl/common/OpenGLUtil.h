
#ifndef OPENGL_UTIL_H
#define OPENGL_UTIL_H

#include <string>

#if defined(__ANDROID__)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#elif defined(__APPLE__)
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#endif

GLuint CreateProgram(const std::string &vertexShader, const std::string &fragmentShader);

GLuint CreateTexture();

GLuint CreateFramebuffer(int width, int height, GLuint texture);

#endif //OPENGL_UTIL_H
