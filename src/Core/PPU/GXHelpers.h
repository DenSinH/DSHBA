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

#endif //DSHBA_GXHELPERS_H
