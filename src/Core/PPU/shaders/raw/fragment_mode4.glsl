// BEGIN FragmentShaderMode4Source

#version 430 core

uniform sampler2D PAL;
uniform usampler2D OAM;
uniform usampler2D IO;

layout (std430, binding = ++VRAMSSBO++) readonly buffer VRAMSSBO
{
    uint VRAM[++VRAMSize++ >> 2];
};

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

//    {
//        vec4 Color = texelFetch(
//        PAL, ivec2(y, x), 0
//        );
//        if (Color == vec4(0, 0, 0, 0)) {
//            return vec4(1.0, 0, 0, 1.0);
//        }
//    }

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
    vec4 Color = texelFetch(
        PAL, ivec2(PaletteIndex, y), 0
    );

    // We already converted to BGR
    return vec4(Color.rgb, 1.0);
}

// END FragmentShaderMode4Source