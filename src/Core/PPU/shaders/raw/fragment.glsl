// BEGIN FragmentShaderSource

#version 430 core

in vec2 texCoord;

out vec4 FragColor;

//layout (std140, binding = ++PALUBO++) uniform PAL
//{
//    uint PALdata[++PALSize++ >> 2];
//};
//
//layout (std140, binding = ++OAMUBO++) uniform OAM
//{
//    uint OAMdata[++OAMSize++ >> 2];
//};
//
//layout (std140, binding = ++IOUBO++) uniform IO
//{
//    uint IOdata[++IOSize++ >> 2];
//};
//
//layout (std430, binding = ++VRAMSSBO++) readonly buffer VRAMSSBO
//{
//    uint VRAM[++VRAMSize++ >> 2];
//};

vec4 mode4(vec2);

void main() {
    FragColor = mode4(texCoord);
}

// END FragmentShaderSource