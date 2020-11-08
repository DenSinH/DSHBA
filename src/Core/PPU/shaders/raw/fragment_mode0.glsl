// BEGIN FragmentShaderMode0Source

#version 430 core

uniform uint BG;

uint readVRAM8(uint address);
uint readVRAM16(uint address);
uint readVRAM32(uint address);

uint readIOreg(uint address);
vec4 readPALentry(uint index);

vec4 regularBGPixel(uint BGCNT, uint x, uint y);
float getDepth(uint BGCNT);

vec4 mode0(uint x, uint y) {
    uint DISPCNT = readIOreg(++DISPCNT++);

    uint BGCNT = readIOreg(++BG0CNT++ + (BG << 1));

    vec4 Color;
    if ((DISPCNT & (++DisplayBG0++ << BG)) == 0) {
        discard;  // background disabled
    }

    Color = regularBGPixel(BGCNT, x, y);

    if (Color.w != 0) {
        // priority
        gl_FragDepth = getDepth(BGCNT);
        return Color;
    }
    else {
        discard;
    }
}

// END FragmentShaderMode0Source