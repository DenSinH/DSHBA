// BEGIN FragmentShaderMode2Source

#version 430 core

uint readVRAM8(uint address);
uint readVRAM16(uint address);
uint readVRAM32(uint address);

uint readIOreg(uint address, uint scanline);
vec4 readPALentry(uint index, uint scanline);
uvec4 readOAMentry(uint index, uint scanline);

vec4 regularBGPixel(uint BGCNT, uint BG, uint x, uint y);
vec4 affineBGPixel(uint BGCNT, uint BG, vec2 screen_pos);

vec4 mode2(uint x, uint y, vec2 screen_pos) {
    uint DISPCNT = readIOreg(++DISPCNT++, y);

    uint BGCNT[4];

    BGCNT[2] = readIOreg(++BG0CNT++ + (2 << 1), y);
    BGCNT[3] = readIOreg(++BG0CNT++ + (3 << 1), y);

    vec4 Color;
    if ((BGCNT[3] & 0x3u) < (BGCNT[2] & 0x3u)) {
        // BG3 has higher priority
        for (uint BG = 3; BG >= 2; BG--) {
            if ((DISPCNT & (++DisplayBG0++ << BG)) == 0) {
                continue;  // background disabled
            }

            Color = affineBGPixel(BGCNT[BG], BG, screen_pos);

            if (Color.w != 0) {
                gl_FragDepth = (2 * float((BGCNT[BG] & 0x3u)) + 1) / 8.0;
                return Color;
            }
        }
    }
    else {
        // BG2 has higher or equal priority
        for (uint BG = 2; BG <= 3; BG++) {
            if ((DISPCNT & (++DisplayBG0++ << BG)) == 0) {
                continue;  // background disabled
            }

            Color = affineBGPixel(BGCNT[BG], BG, screen_pos);

            if (Color.w != 0) {
                gl_FragDepth = (2 * float((BGCNT[BG] & 0x3u)) + 1) / 8.0;
                return Color;
            }
        }
    }

    // highest frag depth
    gl_FragDepth = 1;
    return vec4(readPALentry(0, y).rgb, 1);
}

// END FragmentShaderMode2Source