// BEGIN VertexShaderSource

#version 330 core

layout (location = 0) in vec2 position;

out vec2 screenCoord;
out vec2 OnScreenPos;  // needed for fragment_helpers

void main() {
    // convert y coordinate from scanline to screen coordinate
    gl_Position = vec4(
        position.x,
        1 - (2.0 * position.y) / float(++VISIBLE_SCREEN_HEIGHT++), 0, 1
    );

    screenCoord = vec2(
        float(++VISIBLE_SCREEN_WIDTH++) * float((1.0 + position.x)) / 2.0,
        position.y
    );

    OnScreenPos = screenCoord;
}

// END VertexShaderSource