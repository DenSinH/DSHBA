// BEGIN BlitVertexShaderSource

#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 inTexCoord;

out vec2 texCoord;  // needed for fragment_helpers

void main() {
    texCoord = inTexCoord;

    gl_Position = vec4(
        position.x,
        position.y,
        0,
        1
    );
}

// END BlitVertexShaderSource