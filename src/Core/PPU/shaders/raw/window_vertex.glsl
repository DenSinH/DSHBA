// BEGIN WindowVertexShaderSource

#version 430 core

layout (location = 0) in vec2 position;

uniform uint BG;

out vec2 screenCoord;

void main() {
    if (true) {
        // background
        // convert y coordinate from scanline to screen coordinate
        gl_Position = vec4(
            position.x,
            (2.0 * position.y) / float(++VISIBLE_SCREEN_HEIGHT++) - 1.0, 0, 1
        );

        screenCoord = vec2(
            float(++VISIBLE_SCREEN_WIDTH++) * float((1.0 + position.x)) / 2.0,
            position.y
        );
    }
    else {
        // object
    }
}

// END WindowVertexShaderSource