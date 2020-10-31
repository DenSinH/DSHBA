#ifndef GC__SHADER_H
#define GC__SHADER_H

// FragmentShaderSource (from fragment.glsl, lines 2 to 190)
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
"uint VRAMIndex(uint Tx, uint Ty, uint Size) {\n"  // l:56
"    uint temp = ((Ty & 0x1fu) << 6);\n"  // l:57
"    temp |= temp | ((Tx & 0x1fu) << 1);\n"  // l:58
"    switch (Size) {\n"  // l:59
"        case 0:  // 32x32\n"  // l:60
"        break;\n"  // l:61
"        case 1:  // 64x32\n"  // l:62
"        if ((Tx & 0x20u) != 0) {\n"  // l:63
"            temp |= 0x800u;\n"  // l:64
"        }\n"  // l:65
"        break;\n"  // l:66
"        case 2:  // 32x64\n"  // l:67
"        if ((Ty & 0x20u) != 0) {\n"  // l:68
"            temp |= 0x800u;\n"  // l:69
"        }\n"  // l:70
"        break;\n"  // l:71
"        case 3:  // 64x64\n"  // l:72
"        if ((Ty & 0x20u) != 0) {\n"  // l:73
"            temp |= 0x1000u;\n"  // l:74
"        }\n"  // l:75
"        if ((Tx & 0x20u) != 0) {\n"  // l:76
"            temp |= 0x800u;\n"  // l:77
"        }\n"  // l:78
"        break;\n"  // l:79
"        default:\n"  // l:80
"        // invalid, should not happen\n"  // l:81
"        return 0;\n"  // l:82
"    }\n"  // l:83
"    return temp;\n"  // l:84
"}\n"  // l:85
"\n"  // l:86
"vec4 regularScreenEntryPixel(uint dx, uint dy, uint scanline, uint ScreenEntry, uint CBB, bool ColorMode) {\n"  // l:87
"    uint PaletteBank = ScreenEntry >> 12;  // 16 bits, we need top 4\n"  // l:88
"    if ((ScreenEntry & 0x0800u) != 0) {\n"  // l:89
"        // VFlip\n"  // l:90
"        dy = 7 - dy;\n"  // l:91
"    }\n"  // l:92
"\n"  // l:93
"    if ((ScreenEntry & 0x0400u) != 0) {\n"  // l:94
"        // HFlip\n"  // l:95
"        dx = 7 - dx;\n"  // l:96
"    }\n"  // l:97
"\n"  // l:98
"    uint TID     = ScreenEntry & 0x3ffu;\n"  // l:99
"    uint Address = CBB << 14;\n"  // l:100
"\n"  // l:101
"    if (!ColorMode) {\n"  // l:102
"        // 4bpp\n"  // l:103
"        Address += TID << 5; // beginning of tile\n"  // l:104
"        Address += dy << 2;  // beginning of sliver\n"  // l:105
"\n"  // l:106
"        Address += dx >> 1;  // offset into sliver\n"  // l:107
"        uint VRAMEntry = readVRAM8(Address);\n"  // l:108
"        if ((dx & 1u) != 0) {\n"  // l:109
"            VRAMEntry >>= 4;     // odd x, upper nibble\n"  // l:110
"        }\n"  // l:111
"        else {\n"  // l:112
"            VRAMEntry &= 0xfu;  // even x, lower nibble\n"  // l:113
"        }\n"  // l:114
"\n"  // l:115
"        if (VRAMEntry != 0) {\n"  // l:116
"            return vec4(readPALentry((PaletteBank << 4) + VRAMEntry, scanline).xyz, 1);\n"  // l:117
"        }\n"  // l:118
"    }\n"  // l:119
"    else {\n"  // l:120
"        // 8bpp\n"  // l:121
"        Address += TID << 6; // beginning of tile\n"  // l:122
"        Address += dy << 3;  // beginning of sliver\n"  // l:123
"\n"  // l:124
"        Address += dx;       // offset into sliver\n"  // l:125
"        uint VRAMEntry = readVRAM8(Address);\n"  // l:126
"\n"  // l:127
"        if (VRAMEntry != 0) {\n"  // l:128
"            return vec4(readPALentry(VRAMEntry, scanline).xyz, 1);\n"  // l:129
"        }\n"  // l:130
"    }\n"  // l:131
"\n"  // l:132
"    // transparent\n"  // l:133
"    return vec4(0, 0, 0, 0);\n"  // l:134
"}\n"  // l:135
"\n"  // l:136
"vec4 regularBGPixel(uint BGCNT, uint BG, uint x, uint y) {\n"  // l:137
"    uint HOFS, VOFS;\n"  // l:138
"    uint CBB, SBB, Size;\n"  // l:139
"    bool ColorMode;\n"  // l:140
"\n"  // l:141
"    HOFS  = readIOreg(0x10u + (BG << 2), y) & 0x1ffu;\n"  // l:142
"    VOFS  = readIOreg(0x12u + (BG << 2), y) & 0x1ffu;\n"  // l:143
"\n"  // l:144
"    CBB       = (BGCNT >> 2) & 3u;\n"  // l:145
"    ColorMode = (BGCNT & 0x80u) != 0;  // 0: 4bpp, 1: 8bpp\n"  // l:146
"    SBB       = (BGCNT >> 8) & 0x1fu;\n"  // l:147
"    Size      = (BGCNT >> 14) & 3u;\n"  // l:148
"\n"  // l:149
"    uint x_eff = (x + HOFS) & 0xffffu;\n"  // l:150
"    uint y_eff = (y + VOFS) & 0xffffu;\n"  // l:151
"\n"  // l:152
"    uint ScreenEntryIndex = VRAMIndex(x_eff >> 3u, y_eff >> 3u, Size);\n"  // l:153
"    ScreenEntryIndex += (SBB << 11u);\n"  // l:154
"    uint ScreenEntry = readVRAM16(ScreenEntryIndex);  // always halfword aligned\n"  // l:155
"\n"  // l:156
"    return regularScreenEntryPixel(x_eff & 7u, y_eff & 7u, y, ScreenEntry, CBB, ColorMode);\n"  // l:157
"}\n"  // l:158
"\n"  // l:159
"vec4 mode0(uint, uint);\n"  // l:160
"vec4 mode3(uint, uint);\n"  // l:161
"vec4 mode4(uint, uint);\n"  // l:162
"\n"  // l:163
"void main() {\n"  // l:164
"    uint x = uint(screenCoord.x);\n"  // l:165
"    uint y = uint(screenCoord.y);\n"  // l:166
"\n"  // l:167
"    uint DISPCNT = readIOreg(0, y);\n"  // l:168
"\n"  // l:169
"    vec4 outColor;\n"  // l:170
"    switch(DISPCNT & 7u) {\n"  // l:171
"        case 0u:\n"  // l:172
"            outColor = mode0(x, y);\n"  // l:173
"            break;\n"  // l:174
"        case 3u:\n"  // l:175
"            outColor = mode3(x, y);\n"  // l:176
"            break;\n"  // l:177
"        case 4u:\n"  // l:178
"            outColor = mode4(x, y);\n"  // l:179
"            break;\n"  // l:180
"        default:\n"  // l:181
"            outColor = vec4(1, 1, 1, 1);\n"  // l:182
"            break;\n"  // l:183
"    }\n"  // l:184
"\n"  // l:185
"    FragColor = outColor;\n"  // l:186
"}\n"  // l:187
"\n"  // l:188
;


// FragmentShaderMode0Source (from fragment_mode0.glsl, lines 2 to 46)
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
"vec4 regularBGPixel(uint BGCNT, uint BG, uint x, uint y);\n"  // l:11
"\n"  // l:12
"vec4 mode0(uint x, uint y) {\n"  // l:13
"    uint DISPCNT = readIOreg(0x00u, y);\n"  // l:14
"\n"  // l:15
"    uint BGCNT[4];\n"  // l:16
"\n"  // l:17
"    for (uint BG = 0; BG < 4; BG++) {\n"  // l:18
"        BGCNT[BG] = readIOreg(0x08u + (BG << 1), y);\n"  // l:19
"    }\n"  // l:20
"\n"  // l:21
"    vec4 Color;\n"  // l:22
"    for (uint priority = 0; priority < 4; priority++) {\n"  // l:23
"        for (uint BG = 0; BG < 4; BG++) {\n"  // l:24
"            if ((DISPCNT & (0x0100u << BG)) == 0) {\n"  // l:25
"                continue;  // background disabled\n"  // l:26
"            }\n"  // l:27
"\n"  // l:28
"            if ((BGCNT[BG] & 0x3u) != priority) {\n"  // l:29
"                // background priority\n"  // l:30
"                continue;\n"  // l:31
"            }\n"  // l:32
"\n"  // l:33
"            Color = regularBGPixel(BGCNT[BG], BG, x, y);\n"  // l:34
"\n"  // l:35
"            if (Color.w != 0) {\n"  // l:36
"                return Color;\n"  // l:37
"            }\n"  // l:38
"        }\n"  // l:39
"    }\n"  // l:40
"\n"  // l:41
"    return vec4(readPALentry(0, y).xyz, 1);\n"  // l:42
"}\n"  // l:43
"\n"  // l:44
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