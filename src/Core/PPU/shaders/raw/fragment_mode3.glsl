// BEGIN FragmentShaderMode3Source

#version 430 core

uniform uint BG;

uint readVRAM8(uint address);
uint readVRAM16(uint address);
uint readVRAM32(uint address);

uint readIOreg(uint address);
vec4 readPALentry(uint index);
float getDepth(uint BGCNT);


vec4 mode3(uint x, uint y) {
    if (BG != 2) {
        // only BG2 is drawn
        discard;
    }

    uint DISPCNT = readIOreg(++DISPCNT++);

    if ((DISPCNT & ++DisplayBG2++) == 0) {
        // background 2 is disabled
        discard;
    }

    uint VRAMAddr = (++VISIBLE_SCREEN_WIDTH++ * y + x) << 1;  // 16bpp

    uint PackedColor = readVRAM16(VRAMAddr);

    vec4 Color = vec4(0, 0, 0, 32);  // to be scaled later

    // BGR format
    Color.r = PackedColor & 0x1fu;
    PackedColor >>= 5u;
    Color.g = PackedColor & 0x1fu;
    PackedColor >>= 5u;
    Color.b = PackedColor & 0x1fu;

    uint BGCNT = readIOreg(++BG2CNT++);
    gl_FragDepth = getDepth(BGCNT);

    return Color / 32.0;
}

// END FragmentShaderMode3Source