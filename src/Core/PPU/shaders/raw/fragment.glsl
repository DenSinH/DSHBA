// BEGIN FragmentShaderSource

#version 430 core


in vec2 texCoord;

out vec4 FragColor;

layout (std430, binding = ++VRAMSSBO++) readonly buffer VRAMSSBO
{
    uint VRAM[++VRAMSize++ >> 2];
};

uint readVRAM8(uint address) {
    uint alignment = address & 3u;
    uint value = VRAM[address >> 2];
    value = (value) >> (alignment << 3u);
    value &= 0xffu;
    return value;
}

uint readVRAM16(uint address) {
    uint alignment = address & 1u;
    uint value = VRAM[address >> 2];
    value = (value) >> (alignment << 4u);
    value &= 0xffffu;
    return value;
}

uint readVRAM32(uint address) {
    return VRAM[address >> 2];
}

vec4 mode4(vec2);

void main() {
    FragColor = mode4(texCoord);
}

// END FragmentShaderSource