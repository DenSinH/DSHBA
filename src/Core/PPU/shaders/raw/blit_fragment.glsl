// BEGIN BlitFragmentShaderSource

#version 330 core

in vec2 texCoord;

uniform sampler2D TopLayer;
uniform sampler2D BottomLayer;

out vec4 FragColor;

void main() {
    vec4 top = texture(TopLayer, texCoord);
    vec4 bottom = texture(BottomLayer, texCoord);

    // default: pick top
    FragColor = vec4(
        top.rgb, 1
    );
    if ((bottom.a != -1) && (bottom.a <= 0)) {
        // there was a bottom layer in the bottom framebuffer
        if (top.a >= 0) {
            FragColor = vec4(
                // correct for how we store bottom alpha
                top.rgb * top.a - 2 * bottom.rgb * (bottom.a + 0.25), 1
            );
        }
    }
}

// END BlitFragmentShaderSource