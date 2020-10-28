// BEGIN FragmentShaderSource

#version 430 core


in vec2 screenCoord;

out vec4 FragColor;

uniform sampler2D PAL;
uniform usampler2D OAM;
uniform usampler2D IO;

layout (std430, binding = ++VRAMSSBO++) readonly buffer VRAMSSBO
{
    uint VRAM[++VRAMSize++ >> 2];
};

uint readVRAM8(uint address) {
    uint alignment = address & 3u;
    uint value = VRAM[address >> 2];
    value = (value) >> (alignment << 3u);
    value &= 0xffu;
    return value;
}

uint readVRAM16(uint address) {
    uint alignment = address & 2u;
    uint value = VRAM[address >> 2];
    value = (value) >> (alignment << 3u);
    value &= 0xffffu;
    return value;
}

uint readVRAM32(uint address) {
    return VRAM[address >> 2];
}

uint readIOreg(uint address, uint scanline) {
    return texelFetch(
        IO, ivec2(address >> 1u, scanline), 0
    ).r;
}

vec4 readPALentry(uint index, uint scanline) {
    // Conveniently, since PAL stores the converted colors already, getting a color from an index is as simple as this:
    return texelFetch(
        PAL, ivec2(index, scanline), 0
    );
}

uvec4 readOAMentry(uint index, uint scanline) {
    return texelFetch(
        OAM, ivec2(index, scanline), 0
    );
}

vec4 mode3(uint, uint);
vec4 mode4(uint, uint);

void main() {
    uint x = uint(screenCoord.x);
    uint y = uint(screenCoord.y);

    uint DISPCNT = readIOreg(0, y);

    vec4 outColor;
    switch(DISPCNT & 7u) {
        case 3u:
            outColor = mode3(x, y);
            break;
        case 4u:
            outColor = mode4(x, y);
            break;
        default:
            outColor = vec4(1, 0, 0, 1); // mode3(x, y);
            break;
    }

    FragColor = outColor;
}

// END FragmentShaderSource