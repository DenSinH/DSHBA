// BEGIN FragmentShaderMode4Source

#version 430 core

uniform uint BG;

uint readVRAM8(uint address);
uint readVRAM16(uint address);
uint readVRAM32(uint address);

uint readIOreg(uint address);
vec4 readPALentry(uint index);
float getDepth(uint BGCNT);

vec4 mode4(uint x, uint y) {
    if (BG != 2) {
        // only BG2 is drawn
        discard;
    }

    uint DISPCNT = readIOreg(++DISPCNT++);

    if ((DISPCNT & ++DisplayBG2++) == 0) {
        // background 2 is disabled
        discard;
    }

    // offset is specified in DISPCNT
    uint Offset = 0;
    if ((DISPCNT & ++DPFrameSelect++) != 0) {
        // offset
        Offset = 0xa000u;
    }

    uint VRAMAddr = (++VISIBLE_SCREEN_WIDTH++ * y + x);
    VRAMAddr += Offset;
    uint PaletteIndex = readVRAM8(VRAMAddr);

    vec4 Color = readPALentry(PaletteIndex);
    uint BGCNT = readIOreg(++BG2CNT++);
    gl_FragDepth = getDepth(BGCNT);;

    // We already converted to BGR when buffering data
    return vec4(Color.rgb, 1.0);
}

// END FragmentShaderMode4Source