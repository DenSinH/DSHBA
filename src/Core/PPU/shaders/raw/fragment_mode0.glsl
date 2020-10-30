// BEGIN FragmentShaderMode0Source

#version 430 core

uint readVRAM8(uint address);
uint readVRAM16(uint address);
uint readVRAM32(uint address);

uint readIOreg(uint address, uint scanline);
vec4 readPALentry(uint index, uint scanline);
uvec4 readOAMentry(uint index, uint scanline);

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

vec4 regularScreenEntryPixel(uint x, uint y, uint scanline, uint ScreenEntry, uint CBB, bool ColorMode) {
    uint PaletteBank = ScreenEntry >> 12;  // 16 bits, we need top 4
    // todo: flipping

    uint TID     = ScreenEntry & 0x3ffu;
    uint Address = CBB << 14;

    if (!ColorMode) {
        // 4bpp
        Address += TID << 5;       // beginning of tile
        Address += (y & 7u) << 2;  // beginning of sliver

        Address += (x & 7u) >> 1;  // offset into sliver
        uint VRAMEntry = readVRAM8(Address);
        if ((x & 1u) != 0) {
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
        Address += TID << 6;       // beginning of tile
        Address += (y & 7u) << 2;  // beginning of sliver

        Address += (x & 7u);       // offset into sliver
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

    return regularScreenEntryPixel(x_eff, y_eff, y, ScreenEntry, CBB, ColorMode);
}


vec4 mode0(uint x, uint y) {
    uint DISPCNT = readIOreg(++DISPCNT++, y);

    uint BGCNT[4];

    for (uint BG = 0; BG < 4; BG++) {
        BGCNT[BG] = readIOreg(++BG0CNT++ + (BG << 1), y);
    }

    vec4 Color;
    for (uint priority = 0; priority < 4; priority++) {
        for (uint BG = 0; BG < 4; BG++) {
            if ((DISPCNT & (++DisplayBG0++ << BG)) == 0) {
                continue;  // background disabled
            }

            if ((BGCNT[BG] & 0x3u) != priority) {
                continue;
            }

            Color = regularBGPixel(BGCNT[BG], BG, x, y);

            if (Color.w != 0) {
                return Color;
            }
        }
    }

    return vec4(readPALentry(0, y).xyz, 1);
}

// END FragmentShaderMode0Source