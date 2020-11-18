// BEGIN FragmentShaderMode1Source

#version 330 core

uniform uint BG;

uint readVRAM8(uint address);
uint readVRAM16(uint address);
uint readVRAM32(uint address);

uint readIOreg(uint address);
vec4 readPALentry(uint index);

vec4 regularBGPixel(uint BGCNT, uint x, uint y);
vec4 affineBGPixel(uint BGCNT, vec2 screen_pos);
float getDepth(uint BGCNT);

vec4 mode1(uint x, uint y, vec2 screen_pos) {
    if (BG == 3u) {
        // BG 3 is not drawn
        discard;
    }

    uint DISPCNT = readIOreg(++DISPCNT++);

    uint BGCNT = readIOreg(++BG0CNT++ + (BG << 1));

    vec4 Color;
    if ((DISPCNT & (++DisplayBG0++ << BG)) == 0u) {
        discard;  // background disabled
    }


    if (BG < 2u) {
        Color = regularBGPixel(BGCNT, x, y);
    }
    else {
        Color = affineBGPixel(BGCNT, screen_pos);
    }

    if (Color.w != 0) {
        // background priority
        gl_FragDepth = getDepth(BGCNT);
        return Color;
    }
    else {
        discard;
    }
}

// END FragmentShaderMode1Source