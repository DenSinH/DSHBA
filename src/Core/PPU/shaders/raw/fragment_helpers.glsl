// BEGIN FragmentHelperSource

#version 430 core

/* GENERAL */
in vec2 OnScreenPos;

uniform sampler2D PAL;
uniform usampler2D IO;
uniform isampler1D OAM;
uniform usampler2D Window;

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

uint readIOreg(uint address) {
    return texelFetch(
        IO, ivec2(address >> 1u, uint(OnScreenPos.y)), 0
    ).x;
}

ivec4 readOAMentry(uint index) {
    return texelFetch(
        OAM, int(index), 0
    );
}

vec4 readPALentry(uint index) {
    // Conveniently, since PAL stores the converted colors already, getting a color from an index is as simple as this:
    return texelFetch(
        PAL, ivec2(index, uint(OnScreenPos.y)), 0
    );
}

uint getWindow(uint x, uint y) {
    return texelFetch(
        Window, ivec2(x, y), 0
    ).r;
}

// END FragmentHelperSource