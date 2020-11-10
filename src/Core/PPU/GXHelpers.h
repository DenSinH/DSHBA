#ifndef DSHBA_GXHELPERS_H
#define DSHBA_GXHELPERS_H

#include "default.h"
#include "log.h"

#include <glad/glad.h>

static void CompileShader(unsigned int shader, const char* name) {
    glCompileShader(shader);
    int  success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        log_fatal("Shader compilation failed (%s): %s\n", name, infoLog);
    }
}

static void LinkProgram(unsigned int program) {
    glLinkProgram(program);

    int  success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        log_fatal("Shader program linking failed: %s\n", infoLog);
    }
}

static void CheckFramebufferInit(const char* name) {
    GLenum buffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (buffer_status != GL_FRAMEBUFFER_COMPLETE) {
        switch (buffer_status) {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                log_fatal("Error initializing %s framebuffer, incomplete attachment", name);
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                log_fatal("Error initializing %s framebuffer, missing attachment", name);
            case GL_FRAMEBUFFER_UNSUPPORTED:
                log_fatal("Error initializing %s framebuffer, unsupported", name);
            default:
                log_fatal("Error initializing %s framebuffer, unknown error %x", name, buffer_status);
        }
    }
}

#endif //DSHBA_GXHELPERS_H
