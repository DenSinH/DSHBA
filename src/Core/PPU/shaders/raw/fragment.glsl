// BEGIN FragmentShaderSource

#version 430 core


in vec2 texCoord;

out vec4 FragColor;

vec4 mode4(vec2);

void main() {
    FragColor = mode4(texCoord);
}

// END FragmentShaderSource