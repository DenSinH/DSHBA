#ifndef GC__SHADER_H
#define GC__SHADER_H

// FragmentShaderSource (from fragment.glsl, lines 2 to 15)
const char* FragmentShaderSource = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"in vec2 texCoord;\n"  // l:3
"\n"  // l:4
"\n"  // l:5
"out vec4 FragColor;\n"  // l:6
"\n"  // l:7
"vec4 mode4(vec2);\n"  // l:8
"\n"  // l:9
"void main() {\n"  // l:10
"    FragColor = mode4(texCoord);\n"  // l:11
"}\n"  // l:12
"\n"  // l:13
;


// FragmentShaderMode4Source (from fragment_mode4.glsl, lines 2 to 65)
const char* FragmentShaderMode4Source = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"layout (std430, binding = 1) readonly buffer PAL\n"  // l:3
"{\n"  // l:4
"    uint PALdata[0x400u >> 2];\n"  // l:5
"};\n"  // l:6
"\n"  // l:7
"layout (std430, binding = 2) readonly buffer OAM\n"  // l:8
"{\n"  // l:9
"    uint OAMdata[0x400u >> 2];\n"  // l:10
"};\n"  // l:11
"\n"  // l:12
"uniform uint IOdata[0x54u >> 2];\n"  // l:13
"\n"  // l:14
"layout (std430, binding = 4) readonly buffer VRAMSSBO\n"  // l:15
"{\n"  // l:16
"    uint VRAM[0x18000u >> 2];\n"  // l:17
"};\n"  // l:18
"\n"  // l:19
"vec4 mode4(vec2 texCoord) {\n"  // l:20
"    uint x = uint(round(texCoord.x * 240)) - 1;\n"  // l:21
"    uint y = uint(round(texCoord.y * 160)) - 1;\n"  // l:22
"\n"  // l:23
"    uint DISPCNT = IOdata[0];\n"  // l:24
"    if ((DISPCNT & 0x0400u) == 0) {\n"  // l:25
"        // background 2 is disabled\n"  // l:26
"        discard;\n"  // l:27
"    }\n"  // l:28
"\n"  // l:29
"    // offset is specified in DISPCNT\n"  // l:30
"    uint Offset = 0;\n"  // l:31
"    if ((DISPCNT & 0x0010u) != 0) {\n"  // l:32
"        // offset\n"  // l:33
"        Offset = 0xa000u;\n"  // l:34
"    }\n"  // l:35
"\n"  // l:36
"    uint VRAMAddr = (240 * y + x);\n"  // l:37
"    VRAMAddr += Offset;\n"  // l:38
"    uint Alignment = VRAMAddr & 3u;\n"  // l:39
"    uint PaletteIndex = VRAM[VRAMAddr >> 2];\n"  // l:40
"    PaletteIndex = (PaletteIndex) >> (Alignment << 3u);\n"  // l:41
"    PaletteIndex &= 0xffu;\n"  // l:42
"\n"  // l:43
"    // PaletteIndex should actually be multiplied by 2, but since we store bytes in uints, we divide by 4 right after\n"  // l:44
"    uint BitColor = PALdata[PaletteIndex >> 1];\n"  // l:45
"    if ((PaletteIndex & 1u) != 0) {\n"  // l:46
"        // misaligned\n"  // l:47
"        BitColor >>= 16;\n"  // l:48
"    }\n"  // l:49
"    // 16 bit RGB555 format, top bit unused\n"  // l:50
"    BitColor &= 0x7fffu;\n"  // l:51
"\n"  // l:52
"    vec3 Color;\n"  // l:53
"    // GBA uses BGR colors\n"  // l:54
"    Color.b = float(BitColor & 0x1fu);\n"  // l:55
"    BitColor >>= 5;\n"  // l:56
"    Color.g = float(BitColor & 0x1fu);\n"  // l:57
"    BitColor >>= 5;\n"  // l:58
"    Color.r = float(BitColor);  // top bit was unused\n"  // l:59
"\n"  // l:60
"    return vec4(Color / 32.0, 1.0);\n"  // l:61
"}\n"  // l:62
"\n"  // l:63
;


// VertexShaderSource (from vertex.glsl, lines 2 to 15)
const char* VertexShaderSource = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"layout (location = 0) in vec2 position;\n"  // l:3
"\n"  // l:4
"out vec2 texCoord;\n"  // l:5
"\n"  // l:6
"void main() {\n"  // l:7
"    gl_Position = vec4(position, 0.0, 1.0);\n"  // l:8
"\n"  // l:9
"    // flip vertically\n"  // l:10
"    texCoord = vec2((1.0 + position.x) / 2.0, (1.0 - position.y) / 2.0);\n"  // l:11
"}\n"  // l:12
"\n"  // l:13
;

#endif  // GC__SHADER_H