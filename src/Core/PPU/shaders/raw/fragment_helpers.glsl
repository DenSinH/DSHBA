// BEGIN FragmentHelperSource

#version 330 core

/* GENERAL */
in vec2 OnScreenPos;

uniform sampler2D PAL;
uniform usampler2D VRAM;
uniform usampler2D IO;
uniform isampler1D OAM;
uniform usampler2D Window;

// algorithm from https://byuu.net/video/color-emulation/
const float lcdGamma = 4.0;
const float outGamma = 2.2;
const mat3x3 CorrectionMatrix = mat3x3(
        255.0,  10.0,  50.0,
         50.0, 230.0,  10.0,
          0.0,  30.0, 220.0
) / 255.0;

vec4 ColorCorrect(vec4 color) {
    vec3 lrgb = pow(color.rgb, vec3(lcdGamma));
    vec3 rgb = pow(CorrectionMatrix * lrgb, vec3(1.0 / outGamma)) * (255.0 / 280.0);
    return vec4(rgb, color.a);
}

uint readVRAM8(uint address) {
    return texelFetch(
        VRAM, ivec2(address & 0x7fu, address >> 7u), 0
    ).x;
}

uint readVRAM16(uint address) {
    address &= ~1u;
    uint lsb = readVRAM8(address);
    return lsb | (readVRAM8(address + 1u) << 8u);
}

uint readVRAM32(uint address) {
    address &= ~3u;
    uint lsh = readVRAM16(address);
    return lsh | (readVRAM16(address + 2u) << 16u);
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
        Window, ivec2(x, ++VISIBLE_SCREEN_HEIGHT++ - y), 0
    ).r;
}

// END FragmentHelperSource