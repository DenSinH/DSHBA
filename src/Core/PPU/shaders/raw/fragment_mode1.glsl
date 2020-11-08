// BEGIN FragmentShaderMode1Source

#version 430 core

uint readVRAM8(uint address);
uint readVRAM16(uint address);
uint readVRAM32(uint address);

uint readIOreg(uint address, uint scanline);
vec4 readPALentry(uint index, uint scanline);
uvec4 readOAMentry(uint index, uint scanline);

vec4 regularBGPixel(uint BGCNT, uint BG, uint x, uint y);
vec4 affineBGPixel(uint BGCNT, uint BG, vec2 screen_pos);

vec4 mode1(uint x, uint y, vec2 screen_pos) {
    uint DISPCNT = readIOreg(++DISPCNT++, y);

    uint BGCNT[4];

    for (uint BG = 0; BG <= 2; BG++) {
        BGCNT[BG] = readIOreg(++BG0CNT++ + (BG << 1), y);
    }

    vec4 Color;
    for (uint priority = 0; priority < 4; priority++) {
        // BG0 and BG1 are normal, BG2 is affine
        for (uint BG = 0; BG <= 2; BG++) {
            if ((DISPCNT & (++DisplayBG0++ << BG)) == 0) {
                continue;  // background disabled
            }

            if ((BGCNT[BG] & 0x3u) != priority) {
                // background priority
                continue;
            }

            if (BG < 2) {
                Color = regularBGPixel(BGCNT[BG], BG, x, y);
            }
            else {
                Color = affineBGPixel(BGCNT[BG], BG, screen_pos);
            }

            if (Color.w != 0) {
                gl_FragDepth = (2 * float(priority) + 1) / 8.0;
                return Color;
            }
        }
    }

    // highest frag depth
    gl_FragDepth = 1;
    return vec4(readPALentry(0, y).rgb, 1);
}

// END FragmentShaderMode1Source