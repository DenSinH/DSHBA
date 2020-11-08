#ifndef GC__SHADER_H
#define GC__SHADER_H

// FragmentShaderSource (from fragment.glsl, lines 2 to 289)
const char* FragmentShaderSource = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"#define attr0 x\n"  // l:3
"#define attr1 y\n"  // l:4
"#define attr2 z\n"  // l:5
"#define attr3 w\n"  // l:6
"\n"  // l:7
"in vec2 screenCoord;\n"  // l:8
"\n"  // l:9
"out float gl_FragDepth;\n"  // l:10
"out vec4 FragColor;\n"  // l:11
"\n"  // l:12
"uniform sampler2D PAL;\n"  // l:13
"uniform usampler2D IO;\n"  // l:14
"\n"  // l:15
"uniform uint ReferenceLine2[160];\n"  // l:16
"uniform uint ReferenceLine3[160];\n"  // l:17
"\n"  // l:18
"layout (std430, binding = 4) readonly buffer VRAMSSBO\n"  // l:19
"{\n"  // l:20
"    uint VRAM[0x18000u >> 2];\n"  // l:21
"};\n"  // l:22
"\n"  // l:23
"uint readVRAM8(uint address) {\n"  // l:24
"    uint alignment = address & 3u;\n"  // l:25
"    uint value = VRAM[address >> 2];\n"  // l:26
"    value = (value) >> (alignment << 3u);\n"  // l:27
"    value &= 0xffu;\n"  // l:28
"    return value;\n"  // l:29
"}\n"  // l:30
"\n"  // l:31
"uint readVRAM16(uint address) {\n"  // l:32
"    uint alignment = address & 2u;\n"  // l:33
"    uint value = VRAM[address >> 2];\n"  // l:34
"    value = (value) >> (alignment << 3u);\n"  // l:35
"    value &= 0xffffu;\n"  // l:36
"    return value;\n"  // l:37
"}\n"  // l:38
"\n"  // l:39
"uint readVRAM32(uint address) {\n"  // l:40
"    return VRAM[address >> 2];\n"  // l:41
"}\n"  // l:42
"\n"  // l:43
"uint readIOreg(uint address, uint scanline) {\n"  // l:44
"    return texelFetch(\n"  // l:45
"        IO, ivec2(address >> 1u, scanline), 0\n"  // l:46
"    ).r;\n"  // l:47
"}\n"  // l:48
"\n"  // l:49
"vec4 readPALentry(uint index, uint scanline) {\n"  // l:50
"    // Conveniently, since PAL stores the converted colors already, getting a color from an index is as simple as this:\n"  // l:51
"    return texelFetch(\n"  // l:52
"        PAL, ivec2(index, scanline), 0\n"  // l:53
"    );\n"  // l:54
"}\n"  // l:55
"\n"  // l:56
"uint VRAMIndex(uint Tx, uint Ty, uint Size) {\n"  // l:57
"    uint temp = ((Ty & 0x1fu) << 6);\n"  // l:58
"    temp |= temp | ((Tx & 0x1fu) << 1);\n"  // l:59
"    switch (Size) {\n"  // l:60
"        case 0:  // 32x32\n"  // l:61
"            break;\n"  // l:62
"        case 1:  // 64x32\n"  // l:63
"            if ((Tx & 0x20u) != 0) {\n"  // l:64
"                temp |= 0x800u;\n"  // l:65
"            }\n"  // l:66
"            break;\n"  // l:67
"        case 2:  // 32x64\n"  // l:68
"            if ((Ty & 0x20u) != 0) {\n"  // l:69
"                temp |= 0x800u;\n"  // l:70
"            }\n"  // l:71
"            break;\n"  // l:72
"        case 3:  // 64x64\n"  // l:73
"            if ((Ty & 0x20u) != 0) {\n"  // l:74
"                temp |= 0x1000u;\n"  // l:75
"            }\n"  // l:76
"            if ((Tx & 0x20u) != 0) {\n"  // l:77
"                temp |= 0x800u;\n"  // l:78
"            }\n"  // l:79
"            break;\n"  // l:80
"        default:\n"  // l:81
"            // invalid, should not happen\n"  // l:82
"            return 0;\n"  // l:83
"    }\n"  // l:84
"    return temp;\n"  // l:85
"}\n"  // l:86
"\n"  // l:87
"vec4 regularScreenEntryPixel(uint dx, uint dy, uint scanline, uint ScreenEntry, uint CBB, bool ColorMode) {\n"  // l:88
"    uint PaletteBank = ScreenEntry >> 12;  // 16 bits, we need top 4\n"  // l:89
"    if ((ScreenEntry & 0x0800u) != 0) {\n"  // l:90
"        // VFlip\n"  // l:91
"        dy = 7 - dy;\n"  // l:92
"    }\n"  // l:93
"\n"  // l:94
"    if ((ScreenEntry & 0x0400u) != 0) {\n"  // l:95
"        // HFlip\n"  // l:96
"        dx = 7 - dx;\n"  // l:97
"    }\n"  // l:98
"\n"  // l:99
"    uint TID     = ScreenEntry & 0x3ffu;\n"  // l:100
"    uint Address = CBB << 14;\n"  // l:101
"\n"  // l:102
"    if (!ColorMode) {\n"  // l:103
"        // 4bpp\n"  // l:104
"        Address += TID << 5; // beginning of tile\n"  // l:105
"        Address += dy << 2;  // beginning of sliver\n"  // l:106
"\n"  // l:107
"        Address += dx >> 1;  // offset into sliver\n"  // l:108
"        uint VRAMEntry = readVRAM8(Address);\n"  // l:109
"        if ((dx & 1u) != 0) {\n"  // l:110
"            VRAMEntry >>= 4;     // odd x, upper nibble\n"  // l:111
"        }\n"  // l:112
"        else {\n"  // l:113
"            VRAMEntry &= 0xfu;  // even x, lower nibble\n"  // l:114
"        }\n"  // l:115
"\n"  // l:116
"        if (VRAMEntry != 0) {\n"  // l:117
"            return vec4(readPALentry((PaletteBank << 4) + VRAMEntry, scanline).rgb, 1);\n"  // l:118
"        }\n"  // l:119
"    }\n"  // l:120
"    else {\n"  // l:121
"        // 8bpp\n"  // l:122
"        Address += TID << 6; // beginning of tile\n"  // l:123
"        Address += dy << 3;  // beginning of sliver\n"  // l:124
"\n"  // l:125
"        Address += dx;       // offset into sliver\n"  // l:126
"        uint VRAMEntry = readVRAM8(Address);\n"  // l:127
"\n"  // l:128
"        if (VRAMEntry != 0) {\n"  // l:129
"            return vec4(readPALentry(VRAMEntry, scanline).rgb, 1);\n"  // l:130
"        }\n"  // l:131
"    }\n"  // l:132
"\n"  // l:133
"    // transparent\n"  // l:134
"    return vec4(0, 0, 0, 0);\n"  // l:135
"}\n"  // l:136
"\n"  // l:137
"vec4 regularBGPixel(uint BGCNT, uint BG, uint x, uint y) {\n"  // l:138
"    uint HOFS, VOFS;\n"  // l:139
"    uint CBB, SBB, Size;\n"  // l:140
"    bool ColorMode;\n"  // l:141
"\n"  // l:142
"    HOFS  = readIOreg(0x10u + (BG << 2), y) & 0x1ffu;\n"  // l:143
"    VOFS  = readIOreg(0x12u + (BG << 2), y) & 0x1ffu;\n"  // l:144
"\n"  // l:145
"    CBB       = (BGCNT >> 2) & 3u;\n"  // l:146
"    ColorMode = (BGCNT & 0x80u) != 0;  // 0: 4bpp, 1: 8bpp\n"  // l:147
"    SBB       = (BGCNT >> 8) & 0x1fu;\n"  // l:148
"    Size      = (BGCNT >> 14) & 3u;\n"  // l:149
"\n"  // l:150
"    uint x_eff = (x + HOFS) & 0xffffu;\n"  // l:151
"    uint y_eff = (y + VOFS) & 0xffffu;\n"  // l:152
"\n"  // l:153
"    uint ScreenEntryIndex = VRAMIndex(x_eff >> 3u, y_eff >> 3u, Size);\n"  // l:154
"    ScreenEntryIndex += (SBB << 11u);\n"  // l:155
"    uint ScreenEntry = readVRAM16(ScreenEntryIndex);  // always halfword aligned\n"  // l:156
"\n"  // l:157
"    return regularScreenEntryPixel(x_eff & 7u, y_eff & 7u, y, ScreenEntry, CBB, ColorMode);\n"  // l:158
"}\n"  // l:159
"\n"  // l:160
"const uint AffineBGSizeTable[4] = {\n"  // l:161
"    128, 256, 512, 1024\n"  // l:162
"};\n"  // l:163
"\n"  // l:164
"vec4 affineBGPixel(uint BGCNT, uint BG, vec2 screen_pos) {\n"  // l:165
"    uint x = uint(screen_pos.x);\n"  // l:166
"    uint y = uint(screen_pos.y);\n"  // l:167
"\n"  // l:168
"    uint ReferenceLine;\n"  // l:169
"    uint BGX_raw, BGY_raw;\n"  // l:170
"    int PA, PB, PC, PD;\n"  // l:171
"    if (BG == 2) {\n"  // l:172
"        ReferenceLine = ReferenceLine2[y];\n"  // l:173
"\n"  // l:174
"        BGX_raw  = readIOreg(0x28u, y);\n"  // l:175
"        BGX_raw |= readIOreg(0x2au, y) << 16;\n"  // l:176
"        BGY_raw  = readIOreg(0x2cu, y);\n"  // l:177
"        BGY_raw |= readIOreg(0x2eu, y) << 16;\n"  // l:178
"        PA = int(readIOreg(0x20u, y)) << 16;\n"  // l:179
"        PB = int(readIOreg(0x22u, y)) << 16;\n"  // l:180
"        PC = int(readIOreg(0x24u, y)) << 16;\n"  // l:181
"        PD = int(readIOreg(0x26u, y)) << 16;\n"  // l:182
"    }\n"  // l:183
"    else {\n"  // l:184
"        ReferenceLine = ReferenceLine3[y];\n"  // l:185
"\n"  // l:186
"        BGX_raw  = readIOreg(0x38u, y);\n"  // l:187
"        BGX_raw |= readIOreg(0x3au, y) << 16;\n"  // l:188
"        BGY_raw  = readIOreg(0x3cu, y);\n"  // l:189
"        BGY_raw |= readIOreg(0x3eu, y) << 16;\n"  // l:190
"        PA = int(readIOreg(0x30u, y)) << 16;\n"  // l:191
"        PB = int(readIOreg(0x32u, y)) << 16;\n"  // l:192
"        PC = int(readIOreg(0x34u, y)) << 16;\n"  // l:193
"        PD = int(readIOreg(0x36u, y)) << 16;\n"  // l:194
"    }\n"  // l:195
"\n"  // l:196
"    // convert to signed\n"  // l:197
"    int BGX = int(BGX_raw) << 4;\n"  // l:198
"    int BGY = int(BGY_raw) << 4;\n"  // l:199
"    BGX >>= 4;\n"  // l:200
"    BGY >>= 4;\n"  // l:201
"\n"  // l:202
"    // was already shifted left\n"  // l:203
"    PA >>= 16;\n"  // l:204
"    PB >>= 16;\n"  // l:205
"    PC >>= 16;\n"  // l:206
"    PD >>= 16;\n"  // l:207
"\n"  // l:208
"    uint CBB, SBB, Size;\n"  // l:209
"    bool ColorMode;\n"  // l:210
"\n"  // l:211
"    CBB       = (BGCNT >> 2) & 3u;\n"  // l:212
"    SBB       = (BGCNT >> 8) & 0x1fu;\n"  // l:213
"    Size      = AffineBGSizeTable[(BGCNT >> 14) & 3u];\n"  // l:214
"\n"  // l:215
"    mat2x2 RotationScaling = mat2x2(\n"  // l:216
"        float(PA), float(PC),  // first column\n"  // l:217
"        float(PB), float(PD)   // second column\n"  // l:218
"    );\n"  // l:219
"\n"  // l:220
"    vec2 pos  = screen_pos - vec2(0, ReferenceLine);\n"  // l:221
"    int x_eff = int(BGX + dot(vec2(PA, PB), pos));\n"  // l:222
"    int y_eff = int(BGY + dot(vec2(PC, PD), pos));\n"  // l:223
"\n"  // l:224
"    // correct for fixed point math\n"  // l:225
"    x_eff >>= 8;\n"  // l:226
"    y_eff >>= 8;\n"  // l:227
"\n"  // l:228
"    if ((x_eff < 0) || (x_eff > Size) || (y_eff < 0) || (y_eff > Size)) {\n"  // l:229
"        if ((BGCNT & 0x2000u) == 0) {\n"  // l:230
"            // no display area overflow\n"  // l:231
"            return vec4(0, 0, 0, 0);\n"  // l:232
"        }\n"  // l:233
"\n"  // l:234
"        // wrapping\n"  // l:235
"        x_eff &= int(Size - 1);\n"  // l:236
"        y_eff &= int(Size - 1);\n"  // l:237
"    }\n"  // l:238
"\n"  // l:239
"    uint TIDAddress = (SBB << 11u);  // base\n"  // l:240
"    TIDAddress += ((uint(y_eff) >> 3) * (Size >> 3)) | (uint(x_eff) >> 3);  // tile\n"  // l:241
"    uint TID = readVRAM8(TIDAddress);\n"  // l:242
"\n"  // l:243
"    uint PixelAddress = (CBB << 14) | (TID << 6) | ((uint(y_eff) & 7u) << 3) | (uint(x_eff) & 7u);\n"  // l:244
"    uint VRAMEntry = readVRAM8(PixelAddress);\n"  // l:245
"\n"  // l:246
"    // transparent\n"  // l:247
"    if (VRAMEntry == 0) {\n"  // l:248
"        return vec4(0, 0, 0, 0);\n"  // l:249
"    }\n"  // l:250
"\n"  // l:251
"    return vec4(readPALentry(VRAMEntry, y).rgb, 1);\n"  // l:252
"}\n"  // l:253
"\n"  // l:254
"vec4 mode0(uint, uint);\n"  // l:255
"vec4 mode1(uint, uint, vec2);\n"  // l:256
"vec4 mode3(uint, uint);\n"  // l:257
"vec4 mode4(uint, uint);\n"  // l:258
"\n"  // l:259
"void main() {\n"  // l:260
"    uint x = uint(screenCoord.x);\n"  // l:261
"    uint y = uint(screenCoord.y);\n"  // l:262
"\n"  // l:263
"    uint DISPCNT = readIOreg(0, y);\n"  // l:264
"\n"  // l:265
"    vec4 outColor;\n"  // l:266
"    switch(DISPCNT & 7u) {\n"  // l:267
"        case 0u:\n"  // l:268
"            outColor = mode0(x, y);\n"  // l:269
"            break;\n"  // l:270
"        case 1u:\n"  // l:271
"            outColor = mode1(x, y, screenCoord);\n"  // l:272
"            break;\n"  // l:273
"        case 3u:\n"  // l:274
"            outColor = mode3(x, y);\n"  // l:275
"            break;\n"  // l:276
"        case 4u:\n"  // l:277
"            outColor = mode4(x, y);\n"  // l:278
"            break;\n"  // l:279
"        default:\n"  // l:280
"            outColor = vec4(float(x) / float(240), float(y) / float(160), 1, 1);\n"  // l:281
"            break;\n"  // l:282
"    }\n"  // l:283
"\n"  // l:284
"    FragColor = outColor;\n"  // l:285
"}\n"  // l:286
"\n"  // l:287
;


// FragmentShaderMode0Source (from fragment_mode0.glsl, lines 2 to 49)
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
"                gl_FragDepth = (2 * float(priority) + 1) / 8.0;\n"  // l:37
"                return Color;\n"  // l:38
"            }\n"  // l:39
"        }\n"  // l:40
"    }\n"  // l:41
"\n"  // l:42
"    // highest frag depth\n"  // l:43
"    gl_FragDepth = 1;\n"  // l:44
"    return vec4(readPALentry(0, y).xyz, 1);\n"  // l:45
"}\n"  // l:46
"\n"  // l:47
;


// FragmentShaderMode1Source (from fragment_mode1.glsl, lines 2 to 56)
const char* FragmentShaderMode1Source = 
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
"vec4 affineBGPixel(uint BGCNT, uint BG, vec2 screen_pos);\n"  // l:12
"\n"  // l:13
"vec4 mode1(uint x, uint y, vec2 screen_pos) {\n"  // l:14
"    uint DISPCNT = readIOreg(0x00u, y);\n"  // l:15
"\n"  // l:16
"    uint BGCNT[4];\n"  // l:17
"\n"  // l:18
"    for (uint BG = 0; BG < 4; BG++) {\n"  // l:19
"        BGCNT[BG] = readIOreg(0x08u + (BG << 1), y);\n"  // l:20
"    }\n"  // l:21
"\n"  // l:22
"    vec4 Color;\n"  // l:23
"    for (uint priority = 0; priority < 4; priority++) {\n"  // l:24
"        // BG0 and BG1 are normal, BG2 is affine\n"  // l:25
"        for (uint BG = 0; BG <= 2; BG++) {\n"  // l:26
"            if ((DISPCNT & (0x0100u << BG)) == 0) {\n"  // l:27
"                continue;  // background disabled\n"  // l:28
"            }\n"  // l:29
"\n"  // l:30
"            if ((BGCNT[BG] & 0x3u) != priority) {\n"  // l:31
"                // background priority\n"  // l:32
"                continue;\n"  // l:33
"            }\n"  // l:34
"\n"  // l:35
"            if (BG < 2) {\n"  // l:36
"                Color = regularBGPixel(BGCNT[BG], BG, x, y);\n"  // l:37
"            }\n"  // l:38
"            else {\n"  // l:39
"                Color = affineBGPixel(BGCNT[BG], BG, screen_pos);\n"  // l:40
"            }\n"  // l:41
"\n"  // l:42
"            if (Color.w != 0) {\n"  // l:43
"                gl_FragDepth = (2 * float(priority) + 1) / 8.0;\n"  // l:44
"                return Color;\n"  // l:45
"            }\n"  // l:46
"        }\n"  // l:47
"    }\n"  // l:48
"\n"  // l:49
"    // highest frag depth\n"  // l:50
"    gl_FragDepth = 1;\n"  // l:51
"    return vec4(readPALentry(0, y).xyz, 1);\n"  // l:52
"}\n"  // l:53
"\n"  // l:54
;


// FragmentShaderMode3Source (from fragment_mode3.glsl, lines 2 to 40)
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
"    uint Priority = readIOreg(0x0cu, y);\n"  // l:33
"    gl_FragDepth = (2 * float(Priority) + 1) / 8.0;\n"  // l:34
"\n"  // l:35
"    return Color / 32.0;\n"  // l:36
"}\n"  // l:37
"\n"  // l:38
;


// FragmentShaderMode4Source (from fragment_mode4.glsl, lines 2 to 40)
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
"    uint Priority = readIOreg(0x0cu, y);\n"  // l:32
"    gl_FragDepth = (2 * float(Priority) + 1) / 8.0;\n"  // l:33
"\n"  // l:34
"    // We already converted to BGR when buffering data\n"  // l:35
"    return vec4(Color.rgb, 1.0);\n"  // l:36
"}\n"  // l:37
"\n"  // l:38
;


// ObjectFragmentShaderSource (from object_fragment.glsl, lines 2 to 278)
const char* ObjectFragmentShaderSource = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"#define attr0 x\n"  // l:3
"#define attr1 y\n"  // l:4
"#define attr2 z\n"  // l:5
"#define attr3 w\n"  // l:6
"\n"  // l:7
"in vec2 InObjPos;\n"  // l:8
"in vec2 OnScreenPos;\n"  // l:9
"flat in uvec4 OBJ;\n"  // l:10
"flat in uint ObjWidth;\n"  // l:11
"flat in uint ObjHeight;\n"  // l:12
"\n"  // l:13
"uniform sampler2D PAL;\n"  // l:14
"uniform usampler2D IO;\n"  // l:15
"uniform isampler1D OAM;\n"  // l:16
"\n"  // l:17
"uniform uint YClipStart;\n"  // l:18
"uniform uint YClipEnd;\n"  // l:19
"\n"  // l:20
"layout (std430, binding = 4) readonly buffer VRAMSSBO\n"  // l:21
"{\n"  // l:22
"    uint VRAM[0x18000u >> 2];\n"  // l:23
"};\n"  // l:24
"\n"  // l:25
"out vec4 FragColor;\n"  // l:26
"out float gl_FragDepth;\n"  // l:27
"\n"  // l:28
"/* same stuff as background program: */\n"  // l:29
"\n"  // l:30
"uint readVRAM8(uint address) {\n"  // l:31
"    uint alignment = address & 3u;\n"  // l:32
"    uint value = VRAM[address >> 2];\n"  // l:33
"    value = (value) >> (alignment << 3u);\n"  // l:34
"    value &= 0xffu;\n"  // l:35
"    return value;\n"  // l:36
"}\n"  // l:37
"\n"  // l:38
"uint readVRAM16(uint address) {\n"  // l:39
"    uint alignment = address & 2u;\n"  // l:40
"    uint value = VRAM[address >> 2];\n"  // l:41
"    value = (value) >> (alignment << 3u);\n"  // l:42
"    value &= 0xffffu;\n"  // l:43
"    return value;\n"  // l:44
"}\n"  // l:45
"\n"  // l:46
"uint readVRAM32(uint address) {\n"  // l:47
"    return VRAM[address >> 2];\n"  // l:48
"}\n"  // l:49
"\n"  // l:50
"uint readIOreg(uint address, uint scanline) {\n"  // l:51
"    return texelFetch(\n"  // l:52
"        IO, ivec2(address >> 1u, scanline), 0\n"  // l:53
"    ).r;\n"  // l:54
"}\n"  // l:55
"\n"  // l:56
"ivec4 readOAMentry(uint index) {\n"  // l:57
"    return texelFetch(\n"  // l:58
"        OAM, int(index), 0\n"  // l:59
"    );\n"  // l:60
"}\n"  // l:61
"\n"  // l:62
"vec4 readPALentry(uint index, uint scanline) {\n"  // l:63
"    // Conveniently, since PAL stores the converted colors already, getting a color from an index is as simple as this:\n"  // l:64
"    return texelFetch(\n"  // l:65
"        PAL, ivec2(index, scanline), 0\n"  // l:66
"    );\n"  // l:67
"}\n"  // l:68
"\n"  // l:69
"vec4 RegularObject(bool OAM2DMapping, uint scanline) {\n"  // l:70
"    uint TID = OBJ.attr2 & 0x03ffu;\n"  // l:71
"    uint Priority = (OBJ.attr2 & 0x0c00u) >> 10;\n"  // l:72
"    gl_FragDepth = float(Priority) / 4.0;\n"  // l:73
"\n"  // l:74
"    // todo: mosaic\n"  // l:75
"    uint dx = uint(InObjPos.x);\n"  // l:76
"    uint dy = uint(InObjPos.y);\n"  // l:77
"\n"  // l:78
"    uint PixelAddress;\n"  // l:79
"    if ((OBJ.attr0 & 0x2000u) == 0x0000u) {\n"  // l:80
"        uint PaletteBank = OBJ.attr2 >> 12;\n"  // l:81
"        PixelAddress = TID << 5;\n"  // l:82
"\n"  // l:83
"        // get base address for line of tiles (vertically)\n"  // l:84
"        if (OAM2DMapping) {\n"  // l:85
"            PixelAddress += ObjWidth * (dy >> 3) << 2;\n"  // l:86
"        }\n"  // l:87
"        else {\n"  // l:88
"            PixelAddress += 32 * 0x20 * (dy >> 3);\n"  // l:89
"        }\n"  // l:90
"        PixelAddress += (dy & 7u) << 2; // offset within tile for sliver\n"  // l:91
"\n"  // l:92
"        // Sprites VRAM base address is 0x10000\n"  // l:93
"        PixelAddress = (PixelAddress & 0x7fffu) | 0x10000u;\n"  // l:94
"\n"  // l:95
"        // horizontal offset:\n"  // l:96
"        PixelAddress += (dx >> 3) << 5;    // of tile\n"  // l:97
"        PixelAddress += ((dx & 7u) >> 1);  // in tile\n"  // l:98
"\n"  // l:99
"        uint VRAMEntry = readVRAM8(PixelAddress);\n"  // l:100
"        if ((dx & 1u) != 0) {\n"  // l:101
"            // upper nibble\n"  // l:102
"            VRAMEntry >>= 4;\n"  // l:103
"        }\n"  // l:104
"        else {\n"  // l:105
"            VRAMEntry &= 0x0fu;\n"  // l:106
"        }\n"  // l:107
"\n"  // l:108
"        if (VRAMEntry != 0) {\n"  // l:109
"            return vec4(readPALentry(0x100 + (PaletteBank << 4) + VRAMEntry, scanline).rgb, 1);\n"  // l:110
"        }\n"  // l:111
"        else {\n"  // l:112
"            // transparent\n"  // l:113
"            discard;\n"  // l:114
"        }\n"  // l:115
"    }\n"  // l:116
"    else {\n"  // l:117
"        // 8bpp\n"  // l:118
"        PixelAddress = TID << 5;\n"  // l:119
"\n"  // l:120
"        if (OAM2DMapping) {\n"  // l:121
"            PixelAddress += ObjWidth * (dy & ~7u);\n"  // l:122
"        }\n"  // l:123
"        else {\n"  // l:124
"            PixelAddress += 32 * 0x20 * (dy >> 3);\n"  // l:125
"        }\n"  // l:126
"        PixelAddress += (dy & 7u) << 3;\n"  // l:127
"\n"  // l:128
"        // Sprites VRAM base address is 0x10000\n"  // l:129
"        PixelAddress = (PixelAddress & 0x7fffu) | 0x10000u;\n"  // l:130
"\n"  // l:131
"        // horizontal offset:\n"  // l:132
"        PixelAddress += (dx >> 3) << 6;\n"  // l:133
"        PixelAddress += dx & 7u;\n"  // l:134
"\n"  // l:135
"        uint VRAMEntry = readVRAM8(PixelAddress);\n"  // l:136
"\n"  // l:137
"        if (VRAMEntry != 0) {\n"  // l:138
"            return vec4(readPALentry(0x100 + VRAMEntry, scanline).rgb, 1);\n"  // l:139
"        }\n"  // l:140
"        else {\n"  // l:141
"            // transparent\n"  // l:142
"            discard;\n"  // l:143
"        }\n"  // l:144
"    }\n"  // l:145
"}\n"  // l:146
"\n"  // l:147
"bool InsideBox(vec2 v, vec2 bottomLeft, vec2 topRight) {\n"  // l:148
"    vec2 s = step(bottomLeft, v) - step(topRight, v);\n"  // l:149
"    return (s.x * s.y) != 0.0;\n"  // l:150
"}\n"  // l:151
"\n"  // l:152
"vec4 AffineObject(bool OAM2DMapping, uint scanline) {\n"  // l:153
"    uint TID = OBJ.attr2 & 0x03ffu;\n"  // l:154
"    uint Priority = (OBJ.attr2 & 0x0c00u) >> 10;\n"  // l:155
"    gl_FragDepth = float(Priority) / 4.0;\n"  // l:156
"\n"  // l:157
"    uint AffineIndex = (OBJ.attr1 & 0x3e00u) >> 9;\n"  // l:158
"    AffineIndex <<= 2;  // goes in groups of 4\n"  // l:159
"\n"  // l:160
"    // scaling parameters\n"  // l:161
"    int PA = readOAMentry(AffineIndex).attr3;\n"  // l:162
"    int PB = readOAMentry(AffineIndex + 1).attr3;\n"  // l:163
"    int PC = readOAMentry(AffineIndex + 2).attr3;\n"  // l:164
"    int PD = readOAMentry(AffineIndex + 3).attr3;\n"  // l:165
"\n"  // l:166
"    // todo: mosaic\n"  // l:167
"    // reference point\n"  // l:168
"    vec2 p0 = vec2(\n"  // l:169
"        float(ObjWidth  >> 1),\n"  // l:170
"        float(ObjHeight >> 1)\n"  // l:171
"    );\n"  // l:172
"\n"  // l:173
"    vec2 p1;\n"  // l:174
"    if ((OBJ.attr0 & 0x0300u) == 0x0300u) {\n"  // l:175
"        // double rendering\n"  // l:176
"        p1 = 2 * p0;\n"  // l:177
"    }\n"  // l:178
"    else {\n"  // l:179
"        p1 = p0;\n"  // l:180
"    }\n"  // l:181
"\n"  // l:182
"    mat2x2 rotscale = mat2x2(\n"  // l:183
"        float(PA), float(PC),\n"  // l:184
"        float(PB), float(PD)\n"  // l:185
"    ) / 256.0;  // fixed point stuff\n"  // l:186
"\n"  // l:187
"    ivec2 pos = ivec2(rotscale * (InObjPos - p1) + p0);\n"  // l:188
"    if (!InsideBox(pos, vec2(0, 0), vec2(ObjWidth, ObjHeight))) {\n"  // l:189
"        // out of bounds\n"  // l:190
"        discard;\n"  // l:191
"    }\n"  // l:192
"\n"  // l:193
"    // get actual pixel\n"  // l:194
"    uint PixelAddress = 0x10000;  // OBJ VRAM starts at 0x10000 in VRAM\n"  // l:195
"    PixelAddress += TID << 5;\n"  // l:196
"    if (OAM2DMapping) {\n"  // l:197
"        PixelAddress += ObjWidth * (pos.y & ~7) >> 1;\n"  // l:198
"    }\n"  // l:199
"    else {\n"  // l:200
"        PixelAddress += 32 * 0x20 * (pos.y >> 3);\n"  // l:201
"    }\n"  // l:202
"\n"  // l:203
"    // the rest is very similar to regular sprites:\n"  // l:204
"    if ((OBJ.attr0 & 0x2000u) == 0x0000u) {\n"  // l:205
"        uint PaletteBank = OBJ.attr2 >> 12;\n"  // l:206
"        PixelAddress += (pos.y & 7) << 2; // offset within tile for sliver\n"  // l:207
"\n"  // l:208
"        // horizontal offset:\n"  // l:209
"        PixelAddress += (pos.x >> 3) << 5;    // of tile\n"  // l:210
"        PixelAddress += (pos.x & 7) >> 1;  // in tile\n"  // l:211
"\n"  // l:212
"        uint VRAMEntry = readVRAM8(PixelAddress);\n"  // l:213
"        if ((pos.x & 1) != 0) {\n"  // l:214
"            // upper nibble\n"  // l:215
"            VRAMEntry >>= 4;\n"  // l:216
"        }\n"  // l:217
"        else {\n"  // l:218
"            VRAMEntry &= 0x0fu;\n"  // l:219
"        }\n"  // l:220
"\n"  // l:221
"        if (VRAMEntry != 0) {\n"  // l:222
"            return vec4(readPALentry(0x100 + (PaletteBank << 4) + VRAMEntry, scanline).rgb, 1);\n"  // l:223
"        }\n"  // l:224
"        else {\n"  // l:225
"            // transparent\n"  // l:226
"            discard;\n"  // l:227
"        }\n"  // l:228
"    }\n"  // l:229
"    else {\n"  // l:230
"        PixelAddress += (uint(pos.y) & 7u) << 3; // offset within tile for sliver\n"  // l:231
"\n"  // l:232
"        // horizontal offset:\n"  // l:233
"        PixelAddress += (pos.x >> 3) << 6;  // of tile\n"  // l:234
"        PixelAddress += (pos.x & 7);        // in tile\n"  // l:235
"\n"  // l:236
"        uint VRAMEntry = readVRAM8(PixelAddress);\n"  // l:237
"\n"  // l:238
"        if (VRAMEntry != 0) {\n"  // l:239
"            return vec4(readPALentry(0x100 + VRAMEntry, scanline).rgb, 1);\n"  // l:240
"        }\n"  // l:241
"        else {\n"  // l:242
"            // transparent\n"  // l:243
"            discard;\n"  // l:244
"        }\n"  // l:245
"    }\n"  // l:246
"}\n"  // l:247
"\n"  // l:248
"void main() {\n"  // l:249
"    if (OnScreenPos.x < 0) {\n"  // l:250
"        discard;\n"  // l:251
"    }\n"  // l:252
"    if (OnScreenPos.x > 240) {\n"  // l:253
"        discard;\n"  // l:254
"    }\n"  // l:255
"\n"  // l:256
"    if (OnScreenPos.y < float(YClipStart)) {\n"  // l:257
"        discard;\n"  // l:258
"    }\n"  // l:259
"    if (OnScreenPos.y >= float(YClipEnd)) {\n"  // l:260
"        discard;\n"  // l:261
"    }\n"  // l:262
"\n"  // l:263
"    uint scanline = uint(OnScreenPos.y);\n"  // l:264
"    uint DISPCNT      = readIOreg(0x00u, scanline);\n"  // l:265
"    bool OAM2DMapping = (DISPCNT & (0x0040u)) != 0;\n"  // l:266
"\n"  // l:267
"    if ((OBJ.attr0 & 0x0300u) == 0x0000u) {\n"  // l:268
"        FragColor = RegularObject(OAM2DMapping, scanline);\n"  // l:269
"    }\n"  // l:270
"    else{\n"  // l:271
"        FragColor = AffineObject(OAM2DMapping, scanline);\n"  // l:272
"    }\n"  // l:273
"    // FragColor = vec4(InObjPos.x / float(ObjWidth), InObjPos.y / float(ObjHeight), 1, 1);\n"  // l:274
"}\n"  // l:275
"\n"  // l:276
;


// ObjectVertexShaderSource (from object_vertex.glsl, lines 2 to 104)
const char* ObjectVertexShaderSource = 
"#version 430 core\n"  // l:1
"\n"  // l:2
"#define attr0 x\n"  // l:3
"#define attr1 y\n"  // l:4
"#define attr2 z\n"  // l:5
"#define attr3 w\n"  // l:6
"\n"  // l:7
"layout (location = 0) in uvec4 InOBJ;\n"  // l:8
"\n"  // l:9
"out vec2 InObjPos;\n"  // l:10
"out vec2 OnScreenPos;\n"  // l:11
"flat out uvec4 OBJ;\n"  // l:12
"flat out uint ObjWidth;\n"  // l:13
"flat out uint ObjHeight;\n"  // l:14
"\n"  // l:15
"struct s_ObjSize {\n"  // l:16
"    uint width;\n"  // l:17
"    uint height;\n"  // l:18
"};\n"  // l:19
"\n"  // l:20
"const s_ObjSize ObjSizeTable[3][4] = {\n"  // l:21
"    { s_ObjSize(8, 8),  s_ObjSize(16, 16), s_ObjSize(32, 32), s_ObjSize(64, 64) },\n"  // l:22
"    { s_ObjSize(16, 8), s_ObjSize(32, 8),  s_ObjSize(32, 16), s_ObjSize(64, 32) },\n"  // l:23
"    { s_ObjSize(8, 16), s_ObjSize(8, 32),  s_ObjSize(16, 32), s_ObjSize(32, 62) }\n"  // l:24
"};\n"  // l:25
"\n"  // l:26
"struct s_Position {\n"  // l:27
"    bool right;\n"  // l:28
"    bool low;\n"  // l:29
"};\n"  // l:30
"\n"  // l:31
"const s_Position PositionTable[4] = {\n"  // l:32
"    s_Position(false, false), s_Position(true, false), s_Position(true, true), s_Position(false, true)\n"  // l:33
"};\n"  // l:34
"\n"  // l:35
"void main() {\n"  // l:36
"    OBJ = InOBJ;\n"  // l:37
"    s_Position Position = PositionTable[gl_VertexID & 3];\n"  // l:38
"\n"  // l:39
"    uint Shape = OBJ.attr0 >> 14;\n"  // l:40
"    uint Size  = OBJ.attr1 >> 14;\n"  // l:41
"\n"  // l:42
"    s_ObjSize ObjSize = ObjSizeTable[Shape][Size];\n"  // l:43
"    ObjWidth = ObjSize.width;\n"  // l:44
"    ObjHeight = ObjSize.height;\n"  // l:45
"\n"  // l:46
"    ivec2 ScreenPos = ivec2(OBJ.attr1 & 0x1ffu, OBJ.attr0 & 0xffu);\n"  // l:47
"\n"  // l:48
"    // correct position for screen wrapping\n"  // l:49
"    if (ScreenPos.x > int(240)) {\n"  // l:50
"        ScreenPos.x -= 0x200;\n"  // l:51
"    }\n"  // l:52
"\n"  // l:53
"    if (ScreenPos.y > int(160)) {\n"  // l:54
"        ScreenPos.y -= 0x100;\n"  // l:55
"    }\n"  // l:56
"\n"  // l:57
"    InObjPos = uvec2(0, 0);\n"  // l:58
"    if (Position.right) {\n"  // l:59
"        InObjPos.x  += ObjWidth;\n"  // l:60
"        ScreenPos.x += int(ObjWidth);\n"  // l:61
"\n"  // l:62
"        if ((OBJ.attr0 & 0x0300u) == 0x0300u) {\n"  // l:63
"            // double rendering\n"  // l:64
"            InObjPos.x  += ObjWidth;\n"  // l:65
"            ScreenPos.x += int(ObjWidth);\n"  // l:66
"        }\n"  // l:67
"    }\n"  // l:68
"\n"  // l:69
"    if (Position.low) {\n"  // l:70
"        InObjPos.y  += ObjHeight;\n"  // l:71
"        ScreenPos.y += int(ObjHeight);\n"  // l:72
"\n"  // l:73
"        if ((OBJ.attr0 & 0x0300u) == 0x0300u) {\n"  // l:74
"            // double rendering\n"  // l:75
"            InObjPos.y  += ObjHeight;\n"  // l:76
"            ScreenPos.y += int(ObjHeight);\n"  // l:77
"        }\n"  // l:78
"    }\n"  // l:79
"\n"  // l:80
"    // flipping only applies to regular sprites\n"  // l:81
"    if ((OBJ.attr0 & 0x0300u) == 0x0000u) {\n"  // l:82
"        if ((OBJ.attr1 & 0x2000u) != 0) {\n"  // l:83
"            // VFlip\n"  // l:84
"            InObjPos.y = ObjHeight - InObjPos.y;\n"  // l:85
"        }\n"  // l:86
"\n"  // l:87
"        if ((OBJ.attr1 & 0x1000u) != 0) {\n"  // l:88
"            // HFlip\n"  // l:89
"            InObjPos.x = ObjWidth - InObjPos.x;\n"  // l:90
"        }\n"  // l:91
"    }\n"  // l:92
"\n"  // l:93
"    OnScreenPos = vec2(ScreenPos);\n"  // l:94
"    gl_Position = vec4(\n"  // l:95
"        -1.0 + 2.0 * OnScreenPos.x / float(240),\n"  // l:96
"        1.0 - 2.0 * OnScreenPos.y / float(160),\n"  // l:97
"        0,\n"  // l:98
"        1\n"  // l:99
"    );\n"  // l:100
"}\n"  // l:101
"\n"  // l:102
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