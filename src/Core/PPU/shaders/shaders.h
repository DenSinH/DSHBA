#ifndef GC__SHADER_H
#define GC__SHADER_H

// FragmentShaderSource (from fragment.glsl, lines 2 to 15)
const char* FragmentShaderSource = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"\n"  // l:3
"in vec2 texCoord;\n"  // l:4
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


// FragmentShaderMode4Source (from fragment_mode4.glsl, lines 2 to 49)
const char* FragmentShaderMode4Source = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"uniform sampler2D PAL;\n"  // l:3
"uniform usampler2D OAM;\n"  // l:4
"uniform usampler2D IO;\n"  // l:5
"\n"  // l:6
"layout (std430, binding = 4) readonly buffer VRAMSSBO\n"  // l:7
"{\n"  // l:8
"    uint VRAM[0x18000u >> 2];\n"  // l:9
"};\n"  // l:10
"\n"  // l:11
"vec4 mode4(vec2 texCoord) {\n"  // l:12
"    uint x = uint(round(texCoord.x * 240)) - 1;\n"  // l:13
"    uint y = uint(round(texCoord.y * 160)) - 1;\n"  // l:14
"\n"  // l:15
"    uint DISPCNT = texelFetch(\n"  // l:16
"        IO, ivec2(0, y), 0\n"  // l:17
"    ).r;\n"  // l:18
"\n"  // l:19
"    if ((DISPCNT & 0x0400u) == 0) {\n"  // l:20
"        // background 2 is disabled\n"  // l:21
"        discard;\n"  // l:22
"    }\n"  // l:23
"\n"  // l:24
"    // offset is specified in DISPCNT\n"  // l:25
"    uint Offset = 0;\n"  // l:26
"    if ((DISPCNT & 0x0010u) != 0) {\n"  // l:27
"        // offset\n"  // l:28
"        Offset = 0xa000u;\n"  // l:29
"    }\n"  // l:30
"\n"  // l:31
"    uint VRAMAddr = (240 * y + x);\n"  // l:32
"    VRAMAddr += Offset;\n"  // l:33
"    uint Alignment = VRAMAddr & 3u;\n"  // l:34
"    uint PaletteIndex = VRAM[VRAMAddr >> 2];\n"  // l:35
"    PaletteIndex = (PaletteIndex) >> (Alignment << 3u);\n"  // l:36
"    PaletteIndex &= 0xffu;\n"  // l:37
"\n"  // l:38
"    // PaletteIndex should actually be multiplied by 2, but since we store bytes in uints, we divide by 4 right after\n"  // l:39
"    vec4 Color = texelFetch(\n"  // l:40
"        PAL, ivec2(PaletteIndex, y), 0\n"  // l:41
"    );\n"  // l:42
"\n"  // l:43
"    // We already converted to BGR when buffering data\n"  // l:44
"    return vec4(Color.rgb, 1.0);\n"  // l:45
"}\n"  // l:46
"\n"  // l:47
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
"    gl_Position = vec4(position.x, 1.0 - 2 * position.y / 160, 0, 1);\n"  // l:8
"\n"  // l:9
"    // flip vertically\n"  // l:10
"    texCoord = vec2((1.0 + position.x) / 2.0, position.y / 160);\n"  // l:11
"}\n"  // l:12
"\n"  // l:13
;

#endif  // GC__SHADER_H