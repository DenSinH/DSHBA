// externally added
#version 330 core

// BEGIN ObjectFragmentShaderSource

#define attr0 x
#define attr1 y
#define attr2 z
#define attr3 w

in vec2 InObjPos;
in vec2 OnScreenPos;
flat in uvec4 OBJ;
flat in uint ObjWidth;
flat in uint ObjHeight;

uniform bool Affine;
uniform uint YClipStart;
uniform uint YClipEnd;

#ifdef OBJ_WINDOW
    out uvec4 FragColor;
#else
    out vec4 FragColor;
#endif

out float gl_FragDepth;

vec4 ColorCorrect(vec4 color);

uint readVRAM8(uint address);
uint readVRAM16(uint address);
uint readVRAM32(uint address);

uint readIOreg(uint address);
ivec4 readOAMentry(uint index);
vec4 readPALentry(uint index);

uint getWindow(uint x, uint y);

vec4 RegularObject(bool OAM2DMapping) {
    uint TID = OBJ.attr2 & ++ATTR2_TID++;
    uint Priority = (OBJ.attr2 & 0x0c00u) >> 10;
    gl_FragDepth = float(Priority) / 4.0;

    uint dx = uint(InObjPos.x);
    uint dy = uint(InObjPos.y);

    // mosaic effect
    if ((OBJ.attr0 & ++ATTR0_MOSAIC++) != 0u) {
        uint MOSAIC = readIOreg(++MOSAIC++);
        dx -= dx % (((MOSAIC & 0xf00u) >> 8) + 1u);
        dy -= dy % (((MOSAIC & 0xf000u) >> 12) + 1u);
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
            PixelAddress += 32u * 0x20u * (dy >> 3);
        }
        PixelAddress += (dy & 7u) << 2; // offset within tile for sliver

        // Sprites VRAM base address is 0x10000
        PixelAddress = (PixelAddress & 0x7fffu) | 0x10000u;

        // horizontal offset:
        PixelAddress += (dx >> 3) << 5;    // of tile
        PixelAddress += ((dx & 7u) >> 1);  // in tile

        uint VRAMEntry = readVRAM8(PixelAddress);
        if ((dx & 1u) != 0u) {
            // upper nibble
            VRAMEntry >>= 4;
        }
        else {
            VRAMEntry &= 0x0fu;
        }

        if (VRAMEntry != 0u) {
            return vec4(readPALentry(0x100u + (PaletteBank << 4) + VRAMEntry).rgb, 1);
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
            PixelAddress += 32u * 0x20u * (dy >> 3);
        }
        PixelAddress += (dy & 7u) << 3;

        // Sprites VRAM base address is 0x10000
        PixelAddress = (PixelAddress & 0x7fffu) | 0x10000u;

        // horizontal offset:
        PixelAddress += (dx >> 3) << 6;
        PixelAddress += dx & 7u;

        uint VRAMEntry = readVRAM8(PixelAddress);

        if (VRAMEntry != 0u) {
            return vec4(readPALentry(0x100u + VRAMEntry).rgb, 1);
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
    int PB = readOAMentry(AffineIndex + 1u).attr3;
    int PC = readOAMentry(AffineIndex + 2u).attr3;
    int PD = readOAMentry(AffineIndex + 3u).attr3;

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
    if ((OBJ.attr0 & ++ATTR0_MOSAIC++) != 0u) {
        uint MOSAIC = readIOreg(++MOSAIC++);
        pos.x -= pos.x % int(((MOSAIC & 0xf00u) >> 8) + 1u);
        pos.y -= pos.y % int(((MOSAIC & 0xf000u) >> 12) + 1u);
    }

    // get actual pixel
    uint PixelAddress = 0x10000u;  // OBJ VRAM starts at 0x10000 in VRAM
    PixelAddress += TID << 5;
    if (OAM2DMapping) {
        PixelAddress += ObjWidth * uint(pos.y & ~7) >> 1;
    }
    else {
        PixelAddress += 32u * 0x20u * uint(pos.y >> 3);
    }

    // the rest is very similar to regular sprites:
    if ((OBJ.attr0 & ++ATTR0_CM++) == ++ATTR0_4BPP++) {
        uint PaletteBank = OBJ.attr2 >> 12;
        PixelAddress += uint(pos.y & 7) << 2; // offset within tile for sliver

        // horizontal offset:
        PixelAddress += uint(pos.x >> 3) << 5;    // of tile
        PixelAddress += uint(pos.x & 7) >> 1;  // in tile

        uint VRAMEntry = readVRAM8(PixelAddress);
        if ((pos.x & 1) != 0) {
            // upper nibble
            VRAMEntry >>= 4;
        }
        else {
            VRAMEntry &= 0x0fu;
        }

        if (VRAMEntry != 0u) {
            return vec4(readPALentry(0x100u + (PaletteBank << 4) + VRAMEntry).rgb, 1);
        }
        else {
            // transparent
            discard;
        }
    }
    else {
        PixelAddress += (uint(pos.y) & 7u) << 3; // offset within tile for sliver

        // horizontal offset:
        PixelAddress += uint(pos.x >> 3) << 6;  // of tile
        PixelAddress += uint(pos.x & 7);        // in tile

        uint VRAMEntry = readVRAM8(PixelAddress);

        if (VRAMEntry != 0u) {
            return vec4(readPALentry(0x100u + VRAMEntry).rgb, 1);
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

    uint DISPCNT = readIOreg(++DISPCNT++);
#ifndef OBJ_WINDOW
    if ((DISPCNT & ++DisplayOBJ++) == 0u) {
        // objects disabled in this scanline
        discard;
    }
    if ((getWindow(uint(OnScreenPos.x), uint(OnScreenPos.y)) & 0x10u) == 0u) {
        // disabled by window
        discard;
    }
#else
    if ((DISPCNT & ++DisplayWinObj++) == 0u) {
        // object window disabled in this scanline
        discard;
    }
#endif

    bool OAM2DMapping = (DISPCNT & (++OAM2DMap++)) != 0u;

    vec4 Color;
    if (!Affine) {
        Color = RegularObject(OAM2DMapping);
    }
    else{
        Color = AffineObject(OAM2DMapping);
    }

#ifndef OBJ_WINDOW
    FragColor = ColorCorrect(Color);
    // FragColor = vec4(InObjPos.x / float(ObjWidth), InObjPos.y / float(ObjHeight), 1, 1);
#else
    // RegularObject/AffineObject will only return if it is nontransparent
    uint WINOBJ = (readIOreg(++WINOUT++) >> 8) & 0x3fu;

    FragColor.r = WINOBJ;
    gl_FragDepth = -0.5;  // between WIN1 and WINOUT
#endif
}

// END ObjectFragmentShaderSource