// BEGIN ObjectFragmentShaderSource

#version 430 core

#define attr0 x
#define attr1 y
#define attr2 z
#define attr3 w

in vec2 InObjPos;
flat in uint ObjWidth;
flat in uint ObjHeight;

uniform sampler2D PAL;
uniform usampler2D OAM;
uniform usampler2D IO;

out vec4 FragColor;

layout (std430, binding = ++VRAMSSBO++) readonly buffer VRAMSSBO
{
    uint VRAM[++VRAMSize++ >> 2];
};

void main() {
    FragColor = vec4(InObjPos.x / float(ObjWidth), InObjPos.y / float(ObjHeight), 1, 1);
}

// END ObjectFragmentShaderSource