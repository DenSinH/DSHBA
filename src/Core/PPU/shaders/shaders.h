#ifndef GC__SHADER_H
#define GC__SHADER_H

// FragmentShaderSource (from fragment.glsl, lines 2 to 34)
const char* FragmentShaderSource = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"in vec2 texCoord;\n"  // l:3
"\n"  // l:4
"out vec4 FragColor;\n"  // l:5
"\n"  // l:6
"layout (std140, binding = 3) uniform PAL\n"  // l:7
"{\n"  // l:8
"    uint PALdata[0x400 >> 2];\n"  // l:9
"};\n"  // l:10
"\n"  // l:11
"layout (std140, binding = 4) uniform OAM\n"  // l:12
"{\n"  // l:13
"    uint OAMdata[0x400 >> 2];\n"  // l:14
"};\n"  // l:15
"\n"  // l:16
"layout (std140, binding = 5) uniform IO\n"  // l:17
"{\n"  // l:18
"    uint IOdata[0x400 >> 2];\n"  // l:19
"};\n"  // l:20
"\n"  // l:21
"layout (std430, binding = 6) readonly buffer VRAMSSBO\n"  // l:22
"{\n"  // l:23
"    uint VRAM[0x18000 >> 2];\n"  // l:24
"};\n"  // l:25
"\n"  // l:26
"vec4 mode4(vec2);\n"  // l:27
"\n"  // l:28
"void main() {\n"  // l:29
"    FragColor = mode4(texCoord);\n"  // l:30
"}\n"  // l:31
"\n"  // l:32
;


// FragmentShaderMode4Source (from fragment_mode4.glsl, lines 2 to 54)
const char* FragmentShaderMode4Source = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"layout (std140, binding = 3) uniform PAL\n"  // l:3
"{\n"  // l:4
"    uint PALdata[0x400 >> 2];\n"  // l:5
"};\n"  // l:6
"\n"  // l:7
"layout (std140, binding = 4) uniform OAM\n"  // l:8
"{\n"  // l:9
"    uint OAMdata[0x400 >> 2];\n"  // l:10
"};\n"  // l:11
"\n"  // l:12
"layout (std140, binding = 5) uniform IO\n"  // l:13
"{\n"  // l:14
"    uint IOdata[0x400 >> 2];\n"  // l:15
"};\n"  // l:16
"\n"  // l:17
"layout (std430, binding = 6) readonly buffer VRAMSSBO\n"  // l:18
"{\n"  // l:19
"    uint VRAM[0x18000 >> 2];\n"  // l:20
"};\n"  // l:21
"\n"  // l:22
"vec4 mode4(vec2 texCoord) {\n"  // l:23
"    uint x = uint(round(texCoord.x * 240)) - 1;\n"  // l:24
"    uint y = uint(round(texCoord.y * 160)) - 1;\n"  // l:25
"\n"  // l:26
"    // todo: offset\n"  // l:27
"    uint VRAMAddr = (240 * y + x);\n"  // l:28
"    uint Alignment = VRAMAddr & 3u;\n"  // l:29
"    uint PaletteIndex = VRAM[VRAMAddr >> 2];\n"  // l:30
"    PaletteIndex = (PaletteIndex) >> (Alignment << 3u);\n"  // l:31
"    PaletteIndex &= 0xffu;\n"  // l:32
"\n"  // l:33
"    if ((PaletteIndex & 1u) != 0u) {\n"  // l:34
"        return vec4(\n"  // l:35
"            1.0,\n"  // l:36
"            1.0,\n"  // l:37
"            1.0,\n"  // l:38
"            1.0\n"  // l:39
"        );\n"  // l:40
"    }\n"  // l:41
"    else {\n"  // l:42
"        return vec4(\n"  // l:43
"            0.0,\n"  // l:44
"            0.0,\n"  // l:45
"            0.0,\n"  // l:46
"            1.0\n"  // l:47
"        );\n"  // l:48
"    }\n"  // l:49
"\n"  // l:50
"}\n"  // l:51
"\n"  // l:52
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