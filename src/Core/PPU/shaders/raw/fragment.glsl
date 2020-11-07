// BEGIN FragmentShaderSource

#version 430 core

#define attr0 x
#define attr1 y
#define attr2 z
#define attr3 w

in vec2 screenCoord;

out float gl_FragDepth;
out vec4 FragColor;

uniform sampler2D PAL;
uniform usampler2D IO;

uniform uint ReferenceLine2[++VISIBLE_SCREEN_HEIGHT++];
uniform uint ReferenceLine3[++VISIBLE_SCREEN_HEIGHT++];

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

uint VRAMIndex(uint Tx, uint Ty, uint Size) {
    uint temp = ((Ty & 0x1fu) << 6);
    temp |= temp | ((Tx & 0x1fu) << 1);
    switch (Size) {
        case 0:  // 32x32
            break;
        case 1:  // 64x32
            if ((Tx & 0x20u) != 0) {
                temp |= 0x800u;
            }
            break;
        case 2:  // 32x64
            if ((Ty & 0x20u) != 0) {
                temp |= 0x800u;
            }
            break;
        case 3:  // 64x64
            if ((Ty & 0x20u) != 0) {
                temp |= 0x1000u;
            }
            if ((Tx & 0x20u) != 0) {
                temp |= 0x800u;
            }
            break;
        default:
            // invalid, should not happen
            return 0;
    }
    return temp;
}

vec4 regularScreenEntryPixel(uint dx, uint dy, uint scanline, uint ScreenEntry, uint CBB, bool ColorMode) {
    uint PaletteBank = ScreenEntry >> 12;  // 16 bits, we need top 4
    if ((ScreenEntry & 0x0800u) != 0) {
        // VFlip
        dy = 7 - dy;
    }

    if ((ScreenEntry & 0x0400u) != 0) {
        // HFlip
        dx = 7 - dx;
    }

    uint TID     = ScreenEntry & 0x3ffu;
    uint Address = CBB << 14;

    if (!ColorMode) {
        // 4bpp
        Address += TID << 5; // beginning of tile
        Address += dy << 2;  // beginning of sliver

        Address += dx >> 1;  // offset into sliver
        uint VRAMEntry = readVRAM8(Address);
        if ((dx & 1u) != 0) {
            VRAMEntry >>= 4;     // odd x, upper nibble
        }
        else {
            VRAMEntry &= 0xfu;  // even x, lower nibble
        }

        if (VRAMEntry != 0) {
            return vec4(readPALentry((PaletteBank << 4) + VRAMEntry, scanline).rgb, 1);
        }
    }
    else {
        // 8bpp
        Address += TID << 6; // beginning of tile
        Address += dy << 3;  // beginning of sliver

        Address += dx;       // offset into sliver
        uint VRAMEntry = readVRAM8(Address);

        if (VRAMEntry != 0) {
            return vec4(readPALentry(VRAMEntry, scanline).rgb, 1);
        }
    }

    // transparent
    return vec4(0, 0, 0, 0);
}

vec4 regularBGPixel(uint BGCNT, uint BG, uint x, uint y) {
    uint HOFS, VOFS;
    uint CBB, SBB, Size;
    bool ColorMode;

    HOFS  = readIOreg(++BG0HOFS++ + (BG << 2), y) & 0x1ffu;
    VOFS  = readIOreg(++BG0VOFS++ + (BG << 2), y) & 0x1ffu;

    CBB       = (BGCNT >> 2) & 3u;
    ColorMode = (BGCNT & 0x80u) != 0;  // 0: 4bpp, 1: 8bpp
    SBB       = (BGCNT >> 8) & 0x1fu;
    Size      = (BGCNT >> 14) & 3u;

    uint x_eff = (x + HOFS) & 0xffffu;
    uint y_eff = (y + VOFS) & 0xffffu;

    uint ScreenEntryIndex = VRAMIndex(x_eff >> 3u, y_eff >> 3u, Size);
    ScreenEntryIndex += (SBB << 11u);
    uint ScreenEntry = readVRAM16(ScreenEntryIndex);  // always halfword aligned

    return regularScreenEntryPixel(x_eff & 7u, y_eff & 7u, y, ScreenEntry, CBB, ColorMode);
}

const uint AffineBGSizeTable[4] = {
    128, 256, 512, 1024
};

vec4 affineBGPixel(uint BGCNT, uint BG, vec2 screen_pos) {
    uint x = uint(screen_pos.x);
    uint y = uint(screen_pos.y);

    uint ReferenceLine;
    uint BGX_raw, BGY_raw;
    int PA, PB, PC, PD;
    if (BG == 2) {
        ReferenceLine = ReferenceLine2[y];

        BGX_raw  = readIOreg(++BG2X_L++, y);
        BGX_raw |= readIOreg(++BG2X_H++, y) << 16;
        BGY_raw  = readIOreg(++BG2Y_L++, y);
        BGY_raw |= readIOreg(++BG2Y_H++, y) << 16;
        PA = int(readIOreg(++BG2PA++, y)) << 16;
        PB = int(readIOreg(++BG2PB++, y)) << 16;
        PC = int(readIOreg(++BG2PC++, y)) << 16;
        PD = int(readIOreg(++BG2PD++, y)) << 16;
    }
    else {
        ReferenceLine = ReferenceLine3[y];

        BGX_raw  = readIOreg(++BG3X_L++, y);
        BGX_raw |= readIOreg(++BG3X_H++, y) << 16;
        BGY_raw  = readIOreg(++BG3Y_L++, y);
        BGY_raw |= readIOreg(++BG3Y_H++, y) << 16;
        PA = int(readIOreg(++BG3PA++, y)) << 16;
        PB = int(readIOreg(++BG3PB++, y)) << 16;
        PC = int(readIOreg(++BG3PC++, y)) << 16;
        PD = int(readIOreg(++BG3PD++, y)) << 16;
    }

    // convert to signed
    int BGX = int(BGX_raw) << 4;
    int BGY = int(BGY_raw) << 4;
    BGX >>= 4;
    BGY >>= 4;

    // was already shifted left
    PA >>= 16;
    PB >>= 16;
    PC >>= 16;
    PD >>= 16;

    uint CBB, SBB, Size;
    bool ColorMode;

    CBB       = (BGCNT >> 2) & 3u;
    ColorMode = (BGCNT & 0x80u) != 0;  // 0: 4bpp, 1: 8bpp
    SBB       = (BGCNT >> 8) & 0x1fu;
    Size      = AffineBGSizeTable[(BGCNT >> 14) & 3u];

    mat2x2 RotationScaling = mat2x2(
        float(PA), float(PC),  // first column
        float(PB), float(PD)   // second column
    );

    vec2 pos  = screen_pos - vec2(0, ReferenceLine);
    int x_eff = int(BGX + dot(vec2(PA, PB), pos));
    int y_eff = int(BGY + dot(vec2(PC, PD), pos));

    // correct for fixed point math
    x_eff >>= 8;
    y_eff >>= 8;

    if ((x_eff < 0) || (x_eff > Size) || (y_eff < 0) || (y_eff > Size)) {
        if ((BGCNT & 0x2000u) == 0) {
            // no display area overflow
            return vec4(0, 0, 0, 0);
        }

        // wrapping
        x_eff &= int(Size - 1);
        y_eff &= int(Size - 1);
    }

    uint TIDAddress = (SBB << 11u);  // base
    TIDAddress += ((uint(y_eff) >> 3) * (Size >> 3)) | (uint(x_eff) >> 3);  // tile
    uint TID = readVRAM8(TIDAddress);

    uint PixelAddress = (CBB << 14) | (TID << 6) | ((uint(y_eff) & 7u) << 3) | (uint(x_eff) & 7u);
    uint VRAMEntry = readVRAM8(PixelAddress);

    // transparent
    if (VRAMEntry == 0) {
        return vec4(0, 0, 0, 0);
    }

    return vec4(readPALentry(VRAMEntry, y).rgb, 1);
}

vec4 mode0(uint, uint);
vec4 mode1(uint, uint, vec2);
vec4 mode3(uint, uint);
vec4 mode4(uint, uint);

void main() {
    uint x = uint(screenCoord.x);
    uint y = uint(screenCoord.y);

    uint DISPCNT = readIOreg(0, y);

    vec4 outColor;
    switch(DISPCNT & 7u) {
        case 0u:
            outColor = mode0(x, y);
            break;
        case 1u:
            outColor = mode1(x, y, screenCoord);
            break;
        case 3u:
            outColor = mode3(x, y);
            break;
        case 4u:
            outColor = mode4(x, y);
            break;
        default:
            outColor = vec4(float(x) / float(++VISIBLE_SCREEN_WIDTH++), float(y) / float(++VISIBLE_SCREEN_HEIGHT++), 1, 1);
            break;
    }

    FragColor = outColor;
}

// END FragmentShaderSource