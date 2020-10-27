// BEGIN FragmentShaderMode4Source

#version 430 core

uniform sampler2D PAL;
uniform usampler2D OAM;
uniform usampler2D IO;

layout (std430, binding = ++VRAMSSBO++) readonly buffer VRAMSSBO
{
    uint VRAM[++VRAMSize++ >> 2];
};

uint readVRAM8(uint address);
uint readVRAM16(uint address);
uint readVRAM32(uint address);

vec4 mode4(vec2 texCoord) {
    uint x = uint(round(texCoord.x * ++VISIBLE_SCREEN_WIDTH++)) - 1;
    uint y = uint(round(texCoord.y * ++VISIBLE_SCREEN_HEIGHT++)) - 1;

    uint DISPCNT = texelFetch(
        IO, ivec2(0, y), 0
    ).r;

    if ((DISPCNT & ++DisplayBG2++) == 0) {
        // background 2 is disabled
        discard;
    }

    // offset is specified in DISPCNT
    uint Offset = 0;
    if ((DISPCNT & ++DPFrameSelect++) != 0) {
        // offset
        Offset = 0xa000u;
    }

    uint VRAMAddr = (++VISIBLE_SCREEN_WIDTH++ * y + x);
    VRAMAddr += Offset;
    uint PaletteIndex = readVRAM8(VRAMAddr);

    // Conveniently, since PAL stores the converted colors already, getting a color from an index is as simple as this:
    vec4 Color = texelFetch(
        PAL, ivec2(PaletteIndex, y), 0
    );

    // We already converted to BGR when buffering data
    return vec4(Color.rgb, 1.0);
}

// END FragmentShaderMode4Source