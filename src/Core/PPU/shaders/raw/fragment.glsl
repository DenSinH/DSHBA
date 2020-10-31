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
            return vec4(readPALentry((PaletteBank << 4) + VRAMEntry, scanline).xyz, 1);
        }
    }
    else {
        // 8bpp
        Address += TID << 6; // beginning of tile
        Address += dy << 3;  // beginning of sliver

        Address += dx;       // offset into sliver
        uint VRAMEntry = readVRAM8(Address);

        if (VRAMEntry != 0) {
            return vec4(readPALentry(VRAMEntry, scanline).xyz, 1);
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

vec4 mode0(uint, uint);
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
        case 3u:
            outColor = mode3(x, y);
            break;
        case 4u:
            outColor = mode4(x, y);
            break;
        default:
            outColor = vec4(1, 1, 1, 1);
            break;
    }

    FragColor = outColor;
}

// END FragmentShaderSource