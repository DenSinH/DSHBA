// BEGIN FragmentShaderMode4Source

#version 430 core

uint readVRAM8(uint address);
uint readVRAM16(uint address);
uint readVRAM32(uint address);

uint readIOreg(uint address, uint scanline);
vec4 readPALentry(uint index, uint scanline);
uvec4 readOAMentry(uint index, uint scanline);

vec4 mode4(uint x, uint y) {

    uint DISPCNT = readIOreg(0, y);

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

    vec4 Color = readPALentry(PaletteIndex, y);
    uint Priority = readIOreg(++BG2CNT++, y);
    gl_FragDepth = float(Priority) / 4.0;

    // We already converted to BGR when buffering data
    return vec4(Color.rgb, 1.0);
}

// END FragmentShaderMode4Source