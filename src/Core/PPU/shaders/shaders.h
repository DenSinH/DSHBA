#ifndef GC__SHADER_H
#define GC__SHADER_H

// FragmentShaderSource (from fragment.glsl, lines 2 to 40)
const char* FragmentShaderSource = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"\n"  // l:3
"in vec2 texCoord;\n"  // l:4
"\n"  // l:5
"out vec4 FragColor;\n"  // l:6
"\n"  // l:7
"layout (std430, binding = 4) readonly buffer VRAMSSBO\n"  // l:8
"{\n"  // l:9
"    uint VRAM[0x18000u >> 2];\n"  // l:10
"};\n"  // l:11
"\n"  // l:12
"uint readVRAM8(uint address) {\n"  // l:13
"    uint alignment = address & 3u;\n"  // l:14
"    uint value = VRAM[address >> 2];\n"  // l:15
"    value = (value) >> (alignment << 3u);\n"  // l:16
"    value &= 0xffu;\n"  // l:17
"    return value;\n"  // l:18
"}\n"  // l:19
"\n"  // l:20
"uint readVRAM16(uint address) {\n"  // l:21
"    uint alignment = address & 1u;\n"  // l:22
"    uint value = VRAM[address >> 2];\n"  // l:23
"    value = (value) >> (alignment << 4u);\n"  // l:24
"    value &= 0xffffu;\n"  // l:25
"    return value;\n"  // l:26
"}\n"  // l:27
"\n"  // l:28
"uint readVRAM32(uint address) {\n"  // l:29
"    return VRAM[address >> 2];\n"  // l:30
"}\n"  // l:31
"\n"  // l:32
"vec4 mode4(vec2);\n"  // l:33
"\n"  // l:34
"void main() {\n"  // l:35
"    FragColor = mode4(texCoord);\n"  // l:36
"}\n"  // l:37
"\n"  // l:38
;


// FragmentShaderMode4Source (from fragment_mode4.glsl, lines 2 to 45)
const char* FragmentShaderMode4Source = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"uniform sampler2D PAL;\n"  // l:3
"uniform usampler2D OAM;\n"  // l:4
"uniform usampler2D IO;\n"  // l:5
"\n"  // l:6
"uint readVRAM8(uint address);\n"  // l:7
"uint readVRAM16(uint address);\n"  // l:8
"uint readVRAM32(uint address);\n"  // l:9
"\n"  // l:10
"vec4 mode4(vec2 texCoord) {\n"  // l:11
"    uint x = uint(round(texCoord.x * 240)) - 1;\n"  // l:12
"    uint y = uint(round(texCoord.y * 160)) - 1;\n"  // l:13
"\n"  // l:14
"    uint DISPCNT = texelFetch(\n"  // l:15
"        IO, ivec2(0, y), 0\n"  // l:16
"    ).r;\n"  // l:17
"\n"  // l:18
"    if ((DISPCNT & 0x0400u) == 0) {\n"  // l:19
"        // background 2 is disabled\n"  // l:20
"        discard;\n"  // l:21
"    }\n"  // l:22
"\n"  // l:23
"    // offset is specified in DISPCNT\n"  // l:24
"    uint Offset = 0;\n"  // l:25
"    if ((DISPCNT & 0x0010u) != 0) {\n"  // l:26
"        // offset\n"  // l:27
"        Offset = 0xa000u;\n"  // l:28
"    }\n"  // l:29
"\n"  // l:30
"    uint VRAMAddr = (240 * y + x);\n"  // l:31
"    VRAMAddr += Offset;\n"  // l:32
"    uint PaletteIndex = readVRAM8(VRAMAddr);\n"  // l:33
"\n"  // l:34
"    // Conveniently, since PAL stores the converted colors already, getting a color from an index is as simple as this:\n"  // l:35
"    vec4 Color = texelFetch(\n"  // l:36
"        PAL, ivec2(PaletteIndex, y), 0\n"  // l:37
"    );\n"  // l:38
"\n"  // l:39
"    // We already converted to BGR when buffering data\n"  // l:40
"    return vec4(Color.rgb, 1.0);\n"  // l:41
"}\n"  // l:42
"\n"  // l:43
;


// VertexShaderSource (from vertex.glsl, lines 2 to 16)
const char* VertexShaderSource = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"layout (location = 0) in vec2 position;\n"  // l:3
"\n"  // l:4
"out vec2 texCoord;\n"  // l:5
"\n"  // l:6
"void main() {\n"  // l:7
"    // convert y coordinate from scanline to screen coordinate\n"  // l:8
"    gl_Position = vec4(position.x, 1.0 - 2 * position.y / 160, 0, 1);\n"  // l:9
"\n"  // l:10
"    // flip vertically\n"  // l:11
"    texCoord = vec2((1.0 + position.x) / 2.0, position.y / 160);\n"  // l:12
"}\n"  // l:13
"\n"  // l:14
;

#endif  // GC__SHADER_H