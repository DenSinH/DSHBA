#ifndef GC__SHADER_H
#define GC__SHADER_H

// FragmentShaderSource (from fragment.glsl, lines 2 to 86)
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
"vec4 mode0(uint, uint);\n"  // l:56
"vec4 mode3(uint, uint);\n"  // l:57
"vec4 mode4(uint, uint);\n"  // l:58
"\n"  // l:59
"void main() {\n"  // l:60
"    uint x = uint(screenCoord.x);\n"  // l:61
"    uint y = uint(screenCoord.y);\n"  // l:62
"\n"  // l:63
"    uint DISPCNT = readIOreg(0, y);\n"  // l:64
"\n"  // l:65
"    vec4 outColor;\n"  // l:66
"    switch(DISPCNT & 7u) {\n"  // l:67
"        case 0u:\n"  // l:68
"            outColor = mode0(x, y);\n"  // l:69
"            break;\n"  // l:70
"        case 3u:\n"  // l:71
"            outColor = mode3(x, y);\n"  // l:72
"            break;\n"  // l:73
"        case 4u:\n"  // l:74
"            outColor = mode4(x, y);\n"  // l:75
"            break;\n"  // l:76
"        default:\n"  // l:77
"            outColor = vec4(1, 1, 1, 1);\n"  // l:78
"            break;\n"  // l:79
"    }\n"  // l:80
"\n"  // l:81
"    FragColor = outColor;\n"  // l:82
"}\n"  // l:83
"\n"  // l:84
;


// FragmentShaderMode0Source (from fragment_mode0.glsl, lines 2 to 148)
const char* FragmentShaderMode0Source = 
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
"uint VRAMIndex(uint Tx, uint Ty, uint Size) {\n"  // l:11
"    uint temp = ((Ty & 0x1fu) << 6);\n"  // l:12
"    temp |= temp | ((Tx & 0x1fu) << 1);\n"  // l:13
"    switch (Size) {\n"  // l:14
"        case 0:  // 32x32\n"  // l:15
"            break;\n"  // l:16
"        case 1:  // 64x32\n"  // l:17
"            if ((Tx & 0x20u) != 0) {\n"  // l:18
"                temp |= 0x800u;\n"  // l:19
"            }\n"  // l:20
"            break;\n"  // l:21
"        case 2:  // 32x64\n"  // l:22
"            if ((Ty & 0x20u) != 0) {\n"  // l:23
"                temp |= 0x800u;\n"  // l:24
"            }\n"  // l:25
"            break;\n"  // l:26
"        case 3:  // 64x64\n"  // l:27
"            if ((Ty & 0x20u) != 0) {\n"  // l:28
"                temp |= 0x1000u;\n"  // l:29
"            }\n"  // l:30
"            if ((Tx & 0x20u) != 0) {\n"  // l:31
"                temp |= 0x800u;\n"  // l:32
"            }\n"  // l:33
"            break;\n"  // l:34
"        default:\n"  // l:35
"            // invalid, should not happen\n"  // l:36
"            return 0;\n"  // l:37
"    }\n"  // l:38
"    return temp;\n"  // l:39
"}\n"  // l:40
"\n"  // l:41
"vec4 regularScreenEntryPixel(uint dx, uint dy, uint scanline, uint ScreenEntry, uint CBB, bool ColorMode) {\n"  // l:42
"    uint PaletteBank = ScreenEntry >> 12;  // 16 bits, we need top 4\n"  // l:43
"    if ((ScreenEntry & 0x0800u) != 0) {\n"  // l:44
"        // VFlip\n"  // l:45
"        dy = 7 - dy;\n"  // l:46
"    }\n"  // l:47
"\n"  // l:48
"    if ((ScreenEntry & 0x0400u) != 0) {\n"  // l:49
"        // HFlip\n"  // l:50
"        dx = 7 - dx;\n"  // l:51
"    }\n"  // l:52
"\n"  // l:53
"    uint TID     = ScreenEntry & 0x3ffu;\n"  // l:54
"    uint Address = CBB << 14;\n"  // l:55
"\n"  // l:56
"    if (!ColorMode) {\n"  // l:57
"        // 4bpp\n"  // l:58
"        Address += TID << 5;       // beginning of tile\n"  // l:59
"        Address += dy << 2;  // beginning of sliver\n"  // l:60
"\n"  // l:61
"        Address += dx >> 1;  // offset into sliver\n"  // l:62
"        uint VRAMEntry = readVRAM8(Address);\n"  // l:63
"        if ((dx & 1u) != 0) {\n"  // l:64
"            VRAMEntry >>= 4;     // odd x, upper nibble\n"  // l:65
"        }\n"  // l:66
"        else {\n"  // l:67
"            VRAMEntry &= 0xfu;  // even x, lower nibble\n"  // l:68
"        }\n"  // l:69
"\n"  // l:70
"        if (VRAMEntry != 0) {\n"  // l:71
"            return vec4(readPALentry((PaletteBank << 4) + VRAMEntry, scanline).xyz, 1);\n"  // l:72
"        }\n"  // l:73
"    }\n"  // l:74
"    else {\n"  // l:75
"        // 8bpp\n"  // l:76
"        Address += TID << 6;       // beginning of tile\n"  // l:77
"        Address += dy << 2;  // beginning of sliver\n"  // l:78
"\n"  // l:79
"        Address += dx;       // offset into sliver\n"  // l:80
"        uint VRAMEntry = readVRAM8(Address);\n"  // l:81
"\n"  // l:82
"        if (VRAMEntry != 0) {\n"  // l:83
"            return vec4(readPALentry(VRAMEntry, scanline).xyz, 1);\n"  // l:84
"        }\n"  // l:85
"    }\n"  // l:86
"\n"  // l:87
"    // transparent\n"  // l:88
"    return vec4(0, 0, 0, 0);\n"  // l:89
"}\n"  // l:90
"\n"  // l:91
"vec4 regularBGPixel(uint BGCNT, uint BG, uint x, uint y) {\n"  // l:92
"    uint HOFS, VOFS;\n"  // l:93
"    uint CBB, SBB, Size;\n"  // l:94
"    bool ColorMode;\n"  // l:95
"\n"  // l:96
"    HOFS  = readIOreg(0x10u + (BG << 2), y) & 0x1ffu;\n"  // l:97
"    VOFS  = readIOreg(0x12u + (BG << 2), y) & 0x1ffu;\n"  // l:98
"\n"  // l:99
"    CBB       = (BGCNT >> 2) & 3u;\n"  // l:100
"    ColorMode = (BGCNT & 0x80u) != 0;  // 0: 4bpp, 1: 8bpp\n"  // l:101
"    SBB       = (BGCNT >> 8) & 0x1fu;\n"  // l:102
"    Size      = (BGCNT >> 14) & 3u;\n"  // l:103
"\n"  // l:104
"    uint x_eff = (x + HOFS) & 0xffffu;\n"  // l:105
"    uint y_eff = (y + VOFS) & 0xffffu;\n"  // l:106
"\n"  // l:107
"    uint ScreenEntryIndex = VRAMIndex(x_eff >> 3u, y_eff >> 3u, Size);\n"  // l:108
"    ScreenEntryIndex += (SBB << 11u);\n"  // l:109
"    uint ScreenEntry = readVRAM16(ScreenEntryIndex);  // always halfword aligned\n"  // l:110
"\n"  // l:111
"    return regularScreenEntryPixel(x_eff & 7u, y_eff & 7u, y, ScreenEntry, CBB, ColorMode);\n"  // l:112
"}\n"  // l:113
"\n"  // l:114
"\n"  // l:115
"vec4 mode0(uint x, uint y) {\n"  // l:116
"    uint DISPCNT = readIOreg(0x00u, y);\n"  // l:117
"\n"  // l:118
"    uint BGCNT[4];\n"  // l:119
"\n"  // l:120
"    for (uint BG = 0; BG < 4; BG++) {\n"  // l:121
"        BGCNT[BG] = readIOreg(0x08u + (BG << 1), y);\n"  // l:122
"    }\n"  // l:123
"\n"  // l:124
"    vec4 Color;\n"  // l:125
"    for (uint priority = 0; priority < 4; priority++) {\n"  // l:126
"        for (uint BG = 0; BG < 4; BG++) {\n"  // l:127
"            if ((DISPCNT & (0x0100u << BG)) == 0) {\n"  // l:128
"                continue;  // background disabled\n"  // l:129
"            }\n"  // l:130
"\n"  // l:131
"            if ((BGCNT[BG] & 0x3u) != priority) {\n"  // l:132
"                continue;\n"  // l:133
"            }\n"  // l:134
"\n"  // l:135
"            Color = regularBGPixel(BGCNT[BG], BG, x, y);\n"  // l:136
"\n"  // l:137
"            if (Color.w != 0) {\n"  // l:138
"                return Color;\n"  // l:139
"            }\n"  // l:140
"        }\n"  // l:141
"    }\n"  // l:142
"\n"  // l:143
"    return vec4(readPALentry(0, y).xyz, 1);\n"  // l:144
"}\n"  // l:145
"\n"  // l:146
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