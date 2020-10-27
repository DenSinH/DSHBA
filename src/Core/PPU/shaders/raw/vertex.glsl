// BEGIN VertexShaderSource

#version 430 core

layout (location = 0) in vec2 position;

out vec2 texCoord;

void main() {
    gl_Position = vec4(position.x, 1.0 - 2 * position.y / ++VISIBLE_SCREEN_HEIGHT++, 0, 1);

    // flip vertically
    texCoord = vec2((1.0 + position.x) / 2.0, position.y / ++VISIBLE_SCREEN_HEIGHT++);
}

// END VertexShaderSource