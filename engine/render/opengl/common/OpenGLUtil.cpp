/**
 * Note: operation of OpenGL
 * Date: 2025/12/15
 * Author: frank
 */

#include "OpenGLUtil.h"

#include "NextLog.h"

#define OPENGL_TAG "OpenGLUtil"

GLuint CreateProgram(const std::string &vertexShader, const std::string &fragmentShader) {
    if (vertexShader.empty()) {
        NEXT_LOGE(OPENGL_TAG, "vertex shader is empty!\n");
        return -1;
    }
    if (vertexShader.empty() || fragmentShader.empty()) {
        NEXT_LOGE(OPENGL_TAG, "fragment shader is empty!\n");
        return -1;
    }

    GLint compileStatus = 0;
    GLint linkStatus = GL_FALSE;
    const char *vertexShaderStr   = vertexShader.c_str();
    const char *fragmentShaderStr = fragmentShader.c_str();

    GLuint program    = glCreateProgram();
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!vertShader) {
        NEXT_LOGE(OPENGL_TAG, "create vertex shader error, code=%d\n", glGetError());
        return -1;
    }
    if (!fragShader) {
        NEXT_LOGE(OPENGL_TAG, "create fragment shader error, code=%d\n", glGetError());
        goto error;
    }

    glShaderSource(vertShader, 1, &vertexShaderStr, nullptr);
    glCompileShader(vertShader);
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus) {
        NEXT_LOGE(OPENGL_TAG, "compile vertex shader error, source=%s\n", vertexShaderStr);
        goto error;
    }

    glShaderSource(fragShader, 1, &fragmentShaderStr, nullptr);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus) {
        NEXT_LOGE(OPENGL_TAG, "compile fragment shader error, source=%s\n", fragmentShaderStr);
        goto error;
    }

    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        NEXT_LOGE(OPENGL_TAG, "link program error, code=%d\n", glGetError());
        goto error;
    }

error:
    if (vertShader) {
        glDeleteShader(vertShader);
    }
    if (fragShader) {
        glDeleteShader(fragShader);
    }

    return program;
}

GLuint CreateTexture() {
    auto *texture = new GLuint[1];
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    GLuint result = texture[0];
    delete[] texture;

    return result;
}

GLuint CreateFramebuffer(int width, int height, GLuint texture) {
    auto *frameBuffer = new GLuint[1];
    glGenFramebuffers(1, frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer[0]);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLuint result = frameBuffer[0];
    delete[] frameBuffer;

    return result;
}
