#ifndef GC__SHADER_H
#define GC__SHADER_H

// FragmentShaderSource (from fragment.glsl, lines 2 to 82)
const char* FragmentShaderSource = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"\n"  // l:3
"in vec2 screenCoord;\n"  // l:4
"\n"  // l:5
"out vec4 FragColor;\n"  // l:6
"\n"  // l:7
"uniform sampler2D PAL;\n"  // l:8
"uniform usampler2D OAM;\n"  // l:9
"uniform usampler2D IO;\n"  // l:10
"\n"  // l:11
"layout (std430, binding = 4) readonly buffer VRAMSSBO\n"  // l:12
"{\n"  // l:13
"    uint VRAM[0x18000u >> 2];\n"  // l:14
"};\n"  // l:15
"\n"  // l:16
"uint readVRAM8(uint address) {\n"  // l:17
"    uint alignment = address & 3u;\n"  // l:18
"    uint value = VRAM[address >> 2];\n"  // l:19
"    value = (value) >> (alignment << 3u);\n"  // l:20
"    value &= 0xffu;\n"  // l:21
"    return value;\n"  // l:22
"}\n"  // l:23
"\n"  // l:24
"uint readVRAM16(uint address) {\n"  // l:25
"    uint alignment = address & 2u;\n"  // l:26
"    uint value = VRAM[address >> 2];\n"  // l:27
"    value = (value) >> (alignment << 3u);\n"  // l:28
"    value &= 0xffffu;\n"  // l:29
"    return value;\n"  // l:30
"}\n"  // l:31
"\n"  // l:32
"uint readVRAM32(uint address) {\n"  // l:33
"    return VRAM[address >> 2];\n"  // l:34
"}\n"  // l:35
"\n"  // l:36
"uint readIOreg(uint address, uint scanline) {\n"  // l:37
"    return texelFetch(\n"  // l:38
"        IO, ivec2(address >> 1u, scanline), 0\n"  // l:39
"    ).r;\n"  // l:40
"}\n"  // l:41
"\n"  // l:42
"vec4 readPALentry(uint index, uint scanline) {\n"  // l:43
"    // Conveniently, since PAL stores the converted colors already, getting a color from an index is as simple as this:\n"  // l:44
"    return texelFetch(\n"  // l:45
"        PAL, ivec2(index, scanline), 0\n"  // l:46
"    );\n"  // l:47
"}\n"  // l:48
"\n"  // l:49
"uvec4 readOAMentry(uint index, uint scanline) {\n"  // l:50
"    return texelFetch(\n"  // l:51
"        OAM, ivec2(index, scanline), 0\n"  // l:52
"    );\n"  // l:53
"}\n"  // l:54
"\n"  // l:55
"vec4 mode3(uint, uint);\n"  // l:56
"vec4 mode4(uint, uint);\n"  // l:57
"\n"  // l:58
"void main() {\n"  // l:59
"    uint x = uint(screenCoord.x);\n"  // l:60
"    uint y = uint(screenCoord.y);\n"  // l:61
"\n"  // l:62
"    uint DISPCNT = readIOreg(0, y);\n"  // l:63
"\n"  // l:64
"    vec4 outColor;\n"  // l:65
"    switch(DISPCNT & 7u) {\n"  // l:66
"        case 3u:\n"  // l:67
"            outColor = mode3(x, y);\n"  // l:68
"            break;\n"  // l:69
"        case 4u:\n"  // l:70
"            outColor = mode4(x, y);\n"  // l:71
"            break;\n"  // l:72
"        default:\n"  // l:73
"            outColor = vec4(1, 0, 0, 1); // mode3(x, y);\n"  // l:74
"            break;\n"  // l:75
"    }\n"  // l:76
"\n"  // l:77
"    FragColor = outColor;\n"  // l:78
"}\n"  // l:79
"\n"  // l:80
;


// FragmentShaderMode3Source (from fragment_mode3.glsl, lines 2 to 37)
const char* FragmentShaderMode3Source = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"uint readVRAM8(uint address);\n"  // l:3
"uint readVRAM16(uint address);\n"  // l:4
"uint readVRAM32(uint address);\n"  // l:5
"\n"  // l:6
"uint readIOreg(uint address, uint scanline);\n"  // l:7
"vec4 readPALentry(uint index, uint scanline);\n"  // l:8
"uvec4 readOAMentry(uint index, uint scanline);\n"  // l:9
"\n"  // l:10
"\n"  // l:11
"vec4 mode3(uint x, uint y) {\n"  // l:12
"    uint DISPCNT = readIOreg(0, y);\n"  // l:13
"\n"  // l:14
"    if ((DISPCNT & 0x0400u) == 0) {\n"  // l:15
"        // background 2 is disabled\n"  // l:16
"        discard;\n"  // l:17
"    }\n"  // l:18
"\n"  // l:19
"    uint VRAMAddr = (240 * y + x) << 1;  // 16bpp\n"  // l:20
"\n"  // l:21
"    uint PackedColor = readVRAM16(VRAMAddr);\n"  // l:22
"\n"  // l:23
"    vec4 Color = vec4(0, 0, 0, 32);  // to be scaled later\n"  // l:24
"\n"  // l:25
"    // BGR format\n"  // l:26
"    Color.r = PackedColor & 0x1fu;\n"  // l:27
"    PackedColor >>= 5u;\n"  // l:28
"    Color.g = PackedColor & 0x1fu;\n"  // l:29
"    PackedColor >>= 5u;\n"  // l:30
"    Color.b = PackedColor & 0x1fu;\n"  // l:31
"\n"  // l:32
"    return Color / 32.0;\n"  // l:33
"}\n"  // l:34
"\n"  // l:35
;


// FragmentShaderMode4Source (from fragment_mode4.glsl, lines 2 to 38)
const char* FragmentShaderMode4Source = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"uint readVRAM8(uint address);\n"  // l:3
"uint readVRAM16(uint address);\n"  // l:4
"uint readVRAM32(uint address);\n"  // l:5
"\n"  // l:6
"uint readIOreg(uint address, uint scanline);\n"  // l:7
"vec4 readPALentry(uint index, uint scanline);\n"  // l:8
"uvec4 readOAMentry(uint index, uint scanline);\n"  // l:9
"\n"  // l:10
"vec4 mode4(uint x, uint y) {\n"  // l:11
"\n"  // l:12
"    uint DISPCNT = readIOreg(0, y);\n"  // l:13
"\n"  // l:14
"    if ((DISPCNT & 0x0400u) == 0) {\n"  // l:15
"        // background 2 is disabled\n"  // l:16
"        discard;\n"  // l:17
"    }\n"  // l:18
"\n"  // l:19
"    // offset is specified in DISPCNT\n"  // l:20
"    uint Offset = 0;\n"  // l:21
"    if ((DISPCNT & 0x0010u) != 0) {\n"  // l:22
"        // offset\n"  // l:23
"        Offset = 0xa000u;\n"  // l:24
"    }\n"  // l:25
"\n"  // l:26
"    uint VRAMAddr = (240 * y + x);\n"  // l:27
"    VRAMAddr += Offset;\n"  // l:28
"    uint PaletteIndex = readVRAM8(VRAMAddr);\n"  // l:29
"\n"  // l:30
"    vec4 Color = readPALentry(PaletteIndex, y);\n"  // l:31
"\n"  // l:32
"    // We already converted to BGR when buffering data\n"  // l:33
"    return vec4(Color.rgb, 1.0);\n"  // l:34
"}\n"  // l:35
"\n"  // l:36
;


// VertexShaderSource (from vertex.glsl, lines 2 to 21)
const char* VertexShaderSource = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"layout (location = 0) in vec2 position;\n"  // l:3
"\n"  // l:4
"out vec2 screenCoord;\n"  // l:5
"\n"  // l:6
"void main() {\n"  // l:7
"    // convert y coordinate from scanline to screen coordinate\n"  // l:8
"    gl_Position = vec4(\n"  // l:9
"        position.x,\n"  // l:10
"        1.0 - (2.0 * position.y) / float(160), 0, 1\n"  // l:11
"    );\n"  // l:12
"\n"  // l:13
"    screenCoord = vec2(\n"  // l:14
"        float(240) * float((1.0 + position.x)) / 2.0,\n"  // l:15
"        position.y\n"  // l:16
"    );\n"  // l:17
"}\n"  // l:18
"\n"  // l:19
;

#endif  // GC__SHADER_H