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
uniform isampler1D OAM;

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

uint readIOreg(uint address) {
    return texelFetch(
        IO, ivec2(address >> 1u, uint(OnScreenPos.y)), 0
    ).r;
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

vec4 RegularObject(bool OAM2DMapping) {
    uint TID = OBJ.attr2 & ++ATTR2_TID++;
    uint Priority = (OBJ.attr2 & 0x0c00u) >> 10;
    gl_FragDepth = float(Priority) / 4.0;

    uint dx = uint(InObjPos.x);
    uint dy = uint(InObjPos.y);

    // mosaic effect
    if ((OBJ.attr0 & ++ATTR0_MOSAIC++) != 0) {
        uint MOSAIC = readIOreg(++MOSAIC++);
        dx -= dx % (((MOSAIC & 0xf00u) >> 8) + 1);
        dy -= dy % (((MOSAIC & 0xf000u) >> 12) + 1);
    }

    uint PixelAddress;
    if ((OBJ.attr0 & ++ATTR0_CM++) == ++ATTR0_4BPP++) {
        uint PaletteBank = OBJ.attr2 >> 12;
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
        PixelAddress += (dx >> 3) << 5;    // of tile
        PixelAddress += ((dx & 7u) >> 1);  // in tile

        uint VRAMEntry = readVRAM8(PixelAddress);
        if ((dx & 1u) != 0) {
            // upper nibble
            VRAMEntry >>= 4;
        }
        else {
            VRAMEntry &= 0x0fu;
        }

        if (VRAMEntry != 0) {
            return vec4(readPALentry(0x100 + (PaletteBank << 4) + VRAMEntry).rgb, 1);
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
            return vec4(readPALentry(0x100 + VRAMEntry).rgb, 1);
        }
        else {
            // transparent
            discard;
        }
    }
}

bool InsideBox(vec2 v, vec2 bottomLeft, vec2 topRight) {
    vec2 s = step(bottomLeft, v) - step(topRight, v);
    return (s.x * s.y) != 0.0;
}

vec4 AffineObject(bool OAM2DMapping) {
    uint TID = OBJ.attr2 & ++ATTR2_TID++;
    uint Priority = (OBJ.attr2 & 0x0c00u) >> 10;
    gl_FragDepth = float(Priority) / 4.0;

    uint AffineIndex = (OBJ.attr1 & 0x3e00u) >> 9;
    AffineIndex <<= 2;  // goes in groups of 4

    // scaling parameters
    int PA = readOAMentry(AffineIndex).attr3;
    int PB = readOAMentry(AffineIndex + 1).attr3;
    int PC = readOAMentry(AffineIndex + 2).attr3;
    int PD = readOAMentry(AffineIndex + 3).attr3;

    // reference point
    vec2 p0 = vec2(
        float(ObjWidth  >> 1),
        float(ObjHeight >> 1)
    );

    vec2 p1;
    if ((OBJ.attr0 & ++ATTR0_OM++) == ++ATTR0_AFF_DBL++) {
        // double rendering
        p1 = 2 * p0;
    }
    else {
        p1 = p0;
    }

    mat2x2 rotscale = mat2x2(
        float(PA), float(PC),
        float(PB), float(PD)
    ) / 256.0;  // fixed point stuff

    ivec2 pos = ivec2(rotscale * (InObjPos - p1) + p0);
    if (!InsideBox(pos, vec2(0, 0), vec2(ObjWidth, ObjHeight))) {
        // out of bounds
        discard;
    }

    // mosaic effect
    if ((OBJ.attr0 & ++ATTR0_MOSAIC++) != 0) {
        uint MOSAIC = readIOreg(++MOSAIC++);
        pos.x -= pos.x % int(((MOSAIC & 0xf00u) >> 8) + 1);
        pos.y -= pos.y % int(((MOSAIC & 0xf000u) >> 12) + 1);
    }

    // get actual pixel
    uint PixelAddress = 0x10000;  // OBJ VRAM starts at 0x10000 in VRAM
    PixelAddress += TID << 5;
    if (OAM2DMapping) {
        PixelAddress += ObjWidth * (pos.y & ~7) >> 1;
    }
    else {
        PixelAddress += 32 * 0x20 * (pos.y >> 3);
    }

    // the rest is very similar to regular sprites:
    if ((OBJ.attr0 & ++ATTR0_CM++) == ++ATTR0_4BPP++) {
        uint PaletteBank = OBJ.attr2 >> 12;
        PixelAddress += (pos.y & 7) << 2; // offset within tile for sliver

        // horizontal offset:
        PixelAddress += (pos.x >> 3) << 5;    // of tile
        PixelAddress += (pos.x & 7) >> 1;  // in tile

        uint VRAMEntry = readVRAM8(PixelAddress);
        if ((pos.x & 1) != 0) {
            // upper nibble
            VRAMEntry >>= 4;
        }
        else {
            VRAMEntry &= 0x0fu;
        }

        if (VRAMEntry != 0) {
            return vec4(readPALentry(0x100 + (PaletteBank << 4) + VRAMEntry).rgb, 1);
        }
        else {
            // transparent
            discard;
        }
    }
    else {
        PixelAddress += (uint(pos.y) & 7u) << 3; // offset within tile for sliver

        // horizontal offset:
        PixelAddress += (pos.x >> 3) << 6;  // of tile
        PixelAddress += (pos.x & 7);        // in tile

        uint VRAMEntry = readVRAM8(PixelAddress);

        if (VRAMEntry != 0) {
            return vec4(readPALentry(0x100 + VRAMEntry).rgb, 1);
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
    if (OnScreenPos.y > float(YClipEnd)) {
        discard;
    }

    uint DISPCNT      = readIOreg(++DISPCNT++);
    if ((DISPCNT & ++DisplayOBJ++) == 0) {
        // objects disabled in this scanline
        discard;
    }

    bool OAM2DMapping = (DISPCNT & (++OAM2DMap++)) != 0;

    if ((OBJ.attr0 & ++ATTR0_OM++) == ++ATTR0_REG++) {
        FragColor = RegularObject(OAM2DMapping);
    }
    else{
        FragColor = AffineObject(OAM2DMapping);
    }
    // FragColor = vec4(InObjPos.x / float(ObjWidth), InObjPos.y / float(ObjHeight), 1, 1);
}

// END ObjectFragmentShaderSource