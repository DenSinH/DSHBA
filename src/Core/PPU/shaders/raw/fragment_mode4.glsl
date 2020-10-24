// BEGIN FragmentShaderMode4Source

#version 430 core

layout (std140, binding = ++PALUBO++) uniform PAL
{
    uint PALdata[++PALSize++ >> 2];
};

layout (std140, binding = ++OAMUBO++) uniform OAM
{
    uint OAMdata[++OAMSize++ >> 2];
};

layout (std140, binding = ++IOUBO++) uniform IO
{
    uint IOdata[++IOSize++ >> 2];
};

layout (std430, binding = ++VRAMSSBO++) readonly buffer VRAMSSBO
{
    uint VRAM[++VRAMSize++ >> 2];
};

vec4 mode4(vec2 texCoord) {
    uint x = uint(round(texCoord.x * ++VISIBLE_SCREEN_WIDTH++)) - 1;
    uint y = uint(round(texCoord.y * ++VISIBLE_SCREEN_HEIGHT++)) - 1;

    // todo: offset
    uint VRAMAddr = (++VISIBLE_SCREEN_WIDTH++ * y + x);
    uint Alignment = VRAMAddr & 3u;
    uint PaletteIndex = VRAM[VRAMAddr >> 2];
    PaletteIndex = (PaletteIndex) >> (Alignment << 3u);
    PaletteIndex &= 0xffu;

    if ((PaletteIndex & 1u) != 0u) {
        return vec4(
            1.0,
            1.0,
            1.0,
            1.0
        );
    }
    else {
        return vec4(
            0.0,
            0.0,
            0.0,
            1.0
        );
    }

}

// END FragmentShaderMode4Source