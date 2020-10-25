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
"//layout (std140, binding = 3) uniform PAL\n"  // l:7
"//{\n"  // l:8
"//    uint PALdata[0x400u >> 2];\n"  // l:9
"//};\n"  // l:10
"//\n"  // l:11
"//layout (std140, binding = 4) uniform OAM\n"  // l:12
"//{\n"  // l:13
"//    uint OAMdata[0x400u >> 2];\n"  // l:14
"//};\n"  // l:15
"//\n"  // l:16
"//layout (std140, binding = 5) uniform IO\n"  // l:17
"//{\n"  // l:18
"//    uint IOdata[0x54u >> 2];\n"  // l:19
"//};\n"  // l:20
"//\n"  // l:21
"//layout (std430, binding = 6) readonly buffer VRAMSSBO\n"  // l:22
"//{\n"  // l:23
"//    uint VRAM[0x18000u >> 2];\n"  // l:24
"//};\n"  // l:25
"\n"  // l:26
"vec4 mode4(vec2);\n"  // l:27
"\n"  // l:28
"void main() {\n"  // l:29
"    FragColor = mode4(texCoord);\n"  // l:30
"}\n"  // l:31
"\n"  // l:32
;


// FragmentShaderMode4Source (from fragment_mode4.glsl, lines 2 to 87)
const char* FragmentShaderMode4Source = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"layout (std140, binding = 3) uniform PAL\n"  // l:3
"{\n"  // l:4
"    uint PALdata[0x400u >> 2];\n"  // l:5
"};\n"  // l:6
"\n"  // l:7
"layout (std140, binding = 4) uniform OAM\n"  // l:8
"{\n"  // l:9
"    uint OAMdata[0x400u >> 2];\n"  // l:10
"};\n"  // l:11
"\n"  // l:12
"layout (std140, binding = 5) uniform IO\n"  // l:13
"{\n"  // l:14
"    uint IOdata[0x54u >> 2];\n"  // l:15
"};\n"  // l:16
"\n"  // l:17
"layout (std430, binding = 6) readonly buffer VRAMSSBO\n"  // l:18
"{\n"  // l:19
"    uint VRAM[0x18000u >> 2];\n"  // l:20
"};\n"  // l:21
"\n"  // l:22
"vec4 mode4(vec2 texCoord) {\n"  // l:23
"    uint x = uint(round(texCoord.x * 240)) - 1;\n"  // l:24
"    uint y = uint(round(texCoord.y * 160)) - 1;\n"  // l:25
"\n"  // l:26
"    uint DISPCNT = IOdata[0x00u];\n"  // l:27
"    if ((DISPCNT & 0x0400u) == 0) {\n"  // l:28
"        // background 2 is disabled\n"  // l:29
"        discard;\n"  // l:30
"    }\n"  // l:31
"\n"  // l:32
"    // offset is specified in DISPCNT\n"  // l:33
"    uint Offset = 0;\n"  // l:34
"    if ((DISPCNT & 0x0010u) != 0) {\n"  // l:35
"        // offset\n"  // l:36
"        Offset = 0xa000u;\n"  // l:37
"    }\n"  // l:38
"\n"  // l:39
"    uint VRAMAddr = (240 * y + x);\n"  // l:40
"    VRAMAddr += Offset;\n"  // l:41
"    uint Alignment = VRAMAddr & 3u;\n"  // l:42
"    uint PaletteIndex = VRAM[VRAMAddr >> 2];\n"  // l:43
"    PaletteIndex = (PaletteIndex) >> (Alignment << 3u);\n"  // l:44
"    PaletteIndex &= 0xffu;\n"  // l:45
"\n"  // l:46
"    if (PaletteIndex != 0) {\n"  // l:47
"        if (PALdata[1] == 0x7fff0018u) {\n"  // l:48
"            return vec4(0.0, 1.0, 0, 1);\n"  // l:49
"        }\n"  // l:50
"//        if (PALdata[0] == 0x03000000u) {\n"  // l:51
"//            return vec4(1, 0, 1, 1);\n"  // l:52
"//        }\n"  // l:53
"        for (int i = 1; i < 0x400 >> 2; i++) {\n"  // l:54
"            if (PALdata[i] != 0) {\n"  // l:55
"                return vec4(1, 1, 1, 1);\n"  // l:56
"            }\n"  // l:57
"        }\n"  // l:58
"        if (PALdata[1] == 0 && PALdata[2] == 0) {\n"  // l:59
"            return vec4(0, 0, 1, 1);\n"  // l:60
"        }\n"  // l:61
"        return vec4(float(PaletteIndex) / 5.0, 1, 1, 1);\n"  // l:62
"    }\n"  // l:63
"    discard;\n"  // l:64
"\n"  // l:65
"    // PaletteIndex should actually be multiplied by 2, but since we store bytes in uints, we divide by 4 right after\n"  // l:66
"    uint BitColor = PALdata[PaletteIndex >> 1];\n"  // l:67
"    if ((PaletteIndex & 1u) != 0) {\n"  // l:68
"        // misaligned\n"  // l:69
"        BitColor >>= 16;\n"  // l:70
"    }\n"  // l:71
"    // 16 bit RGB555 format, top bit unused\n"  // l:72
"    BitColor &= 0x7fffu;\n"  // l:73
"\n"  // l:74
"    vec3 Color;\n"  // l:75
"    // GBA uses BGR colors\n"  // l:76
"    Color.b = float(BitColor & 0x1fu);\n"  // l:77
"    BitColor >>= 5;\n"  // l:78
"    Color.g = float(BitColor & 0x1fu);\n"  // l:79
"    BitColor >>= 5;\n"  // l:80
"    Color.r = float(BitColor);  // top bit was unused\n"  // l:81
"\n"  // l:82
"    return vec4(Color / 32.0, 1.0);\n"  // l:83
"}\n"  // l:84
"\n"  // l:85
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