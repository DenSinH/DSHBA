// BEGIN ObjectFragmentShaderSource

#version 430 core

#define attr0 x
#define attr1 y
#define attr2 z
#define attr3 w

in vec2 InObjPos;
in vec2 OnScreenPos;
flat in uvec4 OBJ;
flat in uint ObjWidth;
flat in uint ObjHeight;

uniform sampler2D PAL;
uniform usampler2D IO;

uniform uint YClipStart;
uniform uint YClipEnd;

layout (std430, binding = ++VRAMSSBO++) readonly buffer VRAMSSBO
{
    uint VRAM[++VRAMSize++ >> 2];
};

out vec4 FragColor;
out float gl_FragDepth;

/* same stuff as background program: */

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

vec4 RegularObject(bool OAM2DMapping, uint scanline) {
    uint TID = OBJ.attr2 & ++ATTR2_TID++;
    uint Priority = (OBJ.attr2 & 0x0c00u) >> 10;
    gl_FragDepth = float(Priority) / 4.0;

    uint PaletteBank = OBJ.attr2 >> 12;

    // todo: mosaic
    uint dx = uint(InObjPos.x);
    uint dy = uint(InObjPos.y);

    uint PixelAddress;
    if ((OBJ.attr0 & ++ATTR0_CM++) == ++ATTR0_4BPP++) {
        PixelAddress = TID << 5;

        // get base address for line of tiles (vertically)
        if (OAM2DMapping) {
            PixelAddress += ObjWidth * (dy >> 3) << 2;
        }
        else {
            PixelAddress += 32 * 0x20 * (dy >> 3);
        }
        PixelAddress += (dy & 7u) << 2; // offset within tile for sliver

        // Sprites VRAM base address is 0x10000
        PixelAddress = (PixelAddress & 0x7fffu) | 0x10000u;

        // horizontal offset:
        PixelAddress += (dx >> 3) << 5;  // of tile
        PixelAddress += ((dx & 7u) >> 1);       // in tile

        uint VRAMEntry = readVRAM8(PixelAddress);
        if ((dx & 1u) != 0) {
            // upper nibble
            VRAMEntry >>= 4;
        }
        else {
            VRAMEntry &= 0x0fu;
        }

        if (VRAMEntry != 0) {
            return vec4(readPALentry(0x100 + (PaletteBank << 4) + VRAMEntry, scanline).xyz, 1);
        }
        else {
            // transparent
            discard;
        }
    }
    else {
        // 8bpp
        PixelAddress = TID << 5;

        if (OAM2DMapping) {
            PixelAddress += ObjWidth * (dy & ~7u);
        }
        else {
            PixelAddress += 32 * 0x20 * (dy >> 3);
        }
        PixelAddress += (dy & 7u) << 3;

        // Sprites VRAM base address is 0x10000
        PixelAddress = (PixelAddress & 0x7fffu) | 0x10000u;

        // horizontal offset:
        PixelAddress += (dx >> 3) << 6;
        PixelAddress += dx & 7u;

        uint VRAMEntry = readVRAM8(PixelAddress);


        if (VRAMEntry != 0) {
            return vec4(readPALentry(0x100 + VRAMEntry, scanline).xyz, 1);
        }
        else {
            // transparent
            discard;
        }
    }
}

void main() {
    if (OnScreenPos.x < 0) {
        discard;
    }
    if (OnScreenPos.x > ++VISIBLE_SCREEN_WIDTH++) {
        discard;
    }

    if (OnScreenPos.y < float(YClipStart)) {
        discard;
    }
    if (OnScreenPos.y >= float(YClipEnd)) {
        discard;
    }

    uint scanline = uint(OnScreenPos.y);
    uint DISPCNT      = readIOreg(++DISPCNT++, scanline);
    bool OAM2DMapping = (DISPCNT & (++OAM2DMap++)) != 0;

    switch (OBJ.attr0 & ++ATTR0_OM++) {
        case ++ATTR0_REG++:
            FragColor = RegularObject(OAM2DMapping, scanline);
            break;
        default:
            FragColor = vec4(InObjPos.x / float(ObjWidth), InObjPos.y / float(ObjHeight), 1, 1);
            break;
    }
}

// END ObjectFragmentShaderSource