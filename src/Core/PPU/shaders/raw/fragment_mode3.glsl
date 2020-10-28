// BEGIN FragmentShaderMode3Source

#version 430 core

uint readVRAM8(uint address);
uint readVRAM16(uint address);
uint readVRAM32(uint address);

uint readIOreg(uint address, uint scanline);
vec4 readPALentry(uint index, uint scanline);
uvec4 readOAMentry(uint index, uint scanline);


vec4 mode3(uint x, uint y) {
    uint DISPCNT = readIOreg(0, y);

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

    return Color / 32.0;
}

// END FragmentShaderMode3Source