// BEGIN FragmentShaderMode4Source

#version 430 core

layout (std430, binding = ++PALUBO++) readonly buffer PAL
{
    uint PALdata[++PALSize++ >> 2];
};

layout (std430, binding = ++OAMUBO++) readonly buffer OAM
{
    uint OAMdata[++OAMSize++ >> 2];
};

uniform uint IOdata[++IOSize++ >> 2];

layout (std430, binding = ++VRAMSSBO++) readonly buffer VRAMSSBO
{
    uint VRAM[++VRAMSize++ >> 2];
};

vec4 mode4(vec2 texCoord) {
    uint x = uint(round(texCoord.x * ++VISIBLE_SCREEN_WIDTH++)) - 1;
    uint y = uint(round(texCoord.y * ++VISIBLE_SCREEN_HEIGHT++)) - 1;

    uint DISPCNT = IOdata[0];
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
    uint Alignment = VRAMAddr & 3u;
    uint PaletteIndex = VRAM[VRAMAddr >> 2];
    PaletteIndex = (PaletteIndex) >> (Alignment << 3u);
    PaletteIndex &= 0xffu;

    // PaletteIndex should actually be multiplied by 2, but since we store bytes in uints, we divide by 4 right after
    uint BitColor = PALdata[PaletteIndex >> 1];
    if ((PaletteIndex & 1u) != 0) {
        // misaligned
        BitColor >>= 16;
    }
    // 16 bit RGB555 format, top bit unused
    BitColor &= 0x7fffu;

    vec3 Color;
    // GBA uses BGR colors
    Color.b = float(BitColor & 0x1fu);
    BitColor >>= 5;
    Color.g = float(BitColor & 0x1fu);
    BitColor >>= 5;
    Color.r = float(BitColor);  // top bit was unused

    return vec4(Color / 32.0, 1.0);
}

// END FragmentShaderMode4Source