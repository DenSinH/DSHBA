#ifndef GC__SHADER_H
#define GC__SHADER_H

// FragmentShaderSource (from fragment.glsl, lines 2 to 287)
const char* FragmentShaderSource = 
"#version 430 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"in vec2 screenCoord;\n                                                                             "    // l:3
"\n                                                                                                 "    // l:4
"out vec4 FragColor;\n                                                                              "    // l:5
"uniform uint ReferenceLine2[160];\n                                                                "    // l:6
"uniform uint ReferenceLine3[160];\n                                                                "    // l:7
"\n                                                                                                 "    // l:8
"// BG 0 - 3 or 4 for backdrop\n                                                                    "    // l:9
"uniform uint BG;\n                                                                                 "    // l:10
"\n                                                                                                 "    // l:11
"uint readVRAM8(uint address);\n                                                                    "    // l:12
"uint readVRAM16(uint address);\n                                                                   "    // l:13
"\n                                                                                                 "    // l:14
"uint readVRAM32(uint address);\n                                                                   "    // l:15
"uint readIOreg(uint address);\n                                                                    "    // l:16
"vec4 readPALentry(uint index);\n                                                                   "    // l:17
"\n                                                                                                 "    // l:18
"uint getWindow(uint x, uint y);\n                                                                  "    // l:19
"\n                                                                                                 "    // l:20
"float getDepth(uint BGCNT) {\n                                                                     "    // l:21
"    return ((2 * float(BGCNT & 3u)) / 8.0) + (float(1 + BG) / 32.0);\n                             "    // l:22
"}\n                                                                                                "    // l:23
"\n                                                                                                 "    // l:24
"uint VRAMIndex(uint Tx, uint Ty, uint Size) {\n                                                    "    // l:25
"    uint temp = ((Ty & 0x1fu) << 6);\n                                                             "    // l:26
"    temp |= temp | ((Tx & 0x1fu) << 1);\n                                                          "    // l:27
"    switch (Size) {\n                                                                              "    // l:28
"        case 0:  // 32x32\n                                                                        "    // l:29
"            break;\n                                                                               "    // l:30
"        case 1:  // 64x32\n                                                                        "    // l:31
"            if ((Tx & 0x20u) != 0) {\n                                                             "    // l:32
"                temp |= 0x800u;\n                                                                  "    // l:33
"            }\n                                                                                    "    // l:34
"            break;\n                                                                               "    // l:35
"        case 2:  // 32x64\n                                                                        "    // l:36
"            if ((Ty & 0x20u) != 0) {\n                                                             "    // l:37
"                temp |= 0x800u;\n                                                                  "    // l:38
"            }\n                                                                                    "    // l:39
"            break;\n                                                                               "    // l:40
"        case 3:  // 64x64\n                                                                        "    // l:41
"            if ((Ty & 0x20u) != 0) {\n                                                             "    // l:42
"                temp |= 0x1000u;\n                                                                 "    // l:43
"            }\n                                                                                    "    // l:44
"            if ((Tx & 0x20u) != 0) {\n                                                             "    // l:45
"                temp |= 0x800u;\n                                                                  "    // l:46
"            }\n                                                                                    "    // l:47
"            break;\n                                                                               "    // l:48
"        default:\n                                                                                 "    // l:49
"            // invalid, should not happen\n                                                        "    // l:50
"            return 0;\n                                                                            "    // l:51
"    }\n                                                                                            "    // l:52
"    return temp;\n                                                                                 "    // l:53
"}\n                                                                                                "    // l:54
"\n                                                                                                 "    // l:55
"vec4 regularScreenEntryPixel(uint dx, uint dy, uint ScreenEntry, uint CBB, bool ColorMode) {\n     "    // l:56
"    uint PaletteBank = ScreenEntry >> 12;  // 16 bits, we need top 4\n                             "    // l:57
"    if ((ScreenEntry & 0x0800u) != 0) {\n                                                          "    // l:58
"        // VFlip\n                                                                                 "    // l:59
"        dy = 7 - dy;\n                                                                             "    // l:60
"    }\n                                                                                            "    // l:61
"\n                                                                                                 "    // l:62
"    if ((ScreenEntry & 0x0400u) != 0) {\n                                                          "    // l:63
"        // HFlip\n                                                                                 "    // l:64
"        dx = 7 - dx;\n                                                                             "    // l:65
"    }\n                                                                                            "    // l:66
"\n                                                                                                 "    // l:67
"    uint TID     = ScreenEntry & 0x3ffu;\n                                                         "    // l:68
"    uint Address = CBB << 14;\n                                                                    "    // l:69
"\n                                                                                                 "    // l:70
"    if (!ColorMode) {\n                                                                            "    // l:71
"        // 4bpp\n                                                                                  "    // l:72
"        Address += TID << 5; // beginning of tile\n                                                "    // l:73
"        Address += dy << 2;  // beginning of sliver\n                                              "    // l:74
"\n                                                                                                 "    // l:75
"        Address += dx >> 1;  // offset into sliver\n                                               "    // l:76
"        uint VRAMEntry = readVRAM8(Address);\n                                                     "    // l:77
"        if ((dx & 1u) != 0) {\n                                                                    "    // l:78
"            VRAMEntry >>= 4;     // odd x, upper nibble\n                                          "    // l:79
"        }\n                                                                                        "    // l:80
"        else {\n                                                                                   "    // l:81
"            VRAMEntry &= 0xfu;  // even x, lower nibble\n                                          "    // l:82
"        }\n                                                                                        "    // l:83
"\n                                                                                                 "    // l:84
"        if (VRAMEntry != 0) {\n                                                                    "    // l:85
"            return vec4(readPALentry((PaletteBank << 4) + VRAMEntry).rgb, 1);\n                    "    // l:86
"        }\n                                                                                        "    // l:87
"    }\n                                                                                            "    // l:88
"    else {\n                                                                                       "    // l:89
"        // 8bpp\n                                                                                  "    // l:90
"        Address += TID << 6; // beginning of tile\n                                                "    // l:91
"        Address += dy << 3;  // beginning of sliver\n                                              "    // l:92
"\n                                                                                                 "    // l:93
"        Address += dx;       // offset into sliver\n                                               "    // l:94
"        uint VRAMEntry = readVRAM8(Address);\n                                                     "    // l:95
"\n                                                                                                 "    // l:96
"        if (VRAMEntry != 0) {\n                                                                    "    // l:97
"            return vec4(readPALentry(VRAMEntry).rgb, 1);\n                                         "    // l:98
"        }\n                                                                                        "    // l:99
"    }\n                                                                                            "    // l:100
"\n                                                                                                 "    // l:101
"    // transparent\n                                                                               "    // l:102
"    discard;\n                                                                                     "    // l:103
"}\n                                                                                                "    // l:104
"\n                                                                                                 "    // l:105
"vec4 regularBGPixel(uint BGCNT, uint x, uint y) {\n                                                "    // l:106
"    uint HOFS, VOFS;\n                                                                             "    // l:107
"    uint CBB, SBB, Size;\n                                                                         "    // l:108
"    bool ColorMode;\n                                                                              "    // l:109
"\n                                                                                                 "    // l:110
"    HOFS  = readIOreg(0x10u + (BG << 2)) & 0x1ffu;\n                                               "    // l:111
"    VOFS  = readIOreg(0x12u + (BG << 2)) & 0x1ffu;\n                                               "    // l:112
"\n                                                                                                 "    // l:113
"    CBB       = (BGCNT >> 2) & 3u;\n                                                               "    // l:114
"    ColorMode = (BGCNT & 0x0080u) == 0x0080u;  // 0: 4bpp, 1: 8bpp\n                               "    // l:115
"    SBB       = (BGCNT >> 8) & 0x1fu;\n                                                            "    // l:116
"    Size      = (BGCNT >> 14) & 3u;\n                                                              "    // l:117
"\n                                                                                                 "    // l:118
"    uint x_eff = (x + HOFS) & 0xffffu;\n                                                           "    // l:119
"    uint y_eff = (y + VOFS) & 0xffffu;\n                                                           "    // l:120
"\n                                                                                                 "    // l:121
"    // mosaic effect\n                                                                             "    // l:122
"    if ((BGCNT & 0x0040u) != 0) {\n                                                                "    // l:123
"        uint MOSAIC = readIOreg(0x4cu);\n                                                          "    // l:124
"        x_eff -= x_eff % ((MOSAIC & 0xfu) + 1);\n                                                  "    // l:125
"        y_eff -= y_eff % (((MOSAIC & 0xf0u) >> 4) + 1);\n                                          "    // l:126
"    }\n                                                                                            "    // l:127
"\n                                                                                                 "    // l:128
"    uint ScreenEntryIndex = VRAMIndex(x_eff >> 3u, y_eff >> 3u, Size);\n                           "    // l:129
"    ScreenEntryIndex += (SBB << 11u);\n                                                            "    // l:130
"    uint ScreenEntry = readVRAM16(ScreenEntryIndex);  // always halfword aligned\n                 "    // l:131
"\n                                                                                                 "    // l:132
"    return regularScreenEntryPixel(x_eff & 7u, y_eff & 7u, ScreenEntry, CBB, ColorMode);\n         "    // l:133
"}\n                                                                                                "    // l:134
"\n                                                                                                 "    // l:135
"const uint AffineBGSizeTable[4] = {\n                                                              "    // l:136
"    128, 256, 512, 1024\n                                                                          "    // l:137
"};\n                                                                                               "    // l:138
"\n                                                                                                 "    // l:139
"vec4 affineBGPixel(uint BGCNT, vec2 screen_pos) {\n                                                "    // l:140
"    uint x = uint(screen_pos.x);\n                                                                 "    // l:141
"    uint y = uint(screen_pos.y);\n                                                                 "    // l:142
"\n                                                                                                 "    // l:143
"    uint ReferenceLine;\n                                                                          "    // l:144
"    uint BGX_raw, BGY_raw;\n                                                                       "    // l:145
"    int PA, PB, PC, PD;\n                                                                          "    // l:146
"    if (BG == 2) {\n                                                                               "    // l:147
"        ReferenceLine = ReferenceLine2[y];\n                                                       "    // l:148
"\n                                                                                                 "    // l:149
"        BGX_raw  = readIOreg(0x28u);\n                                                             "    // l:150
"        BGX_raw |= readIOreg(0x2au) << 16;\n                                                       "    // l:151
"        BGY_raw  = readIOreg(0x2cu);\n                                                             "    // l:152
"        BGY_raw |= readIOreg(0x2eu) << 16;\n                                                       "    // l:153
"        PA = int(readIOreg(0x20u)) << 16;\n                                                        "    // l:154
"        PB = int(readIOreg(0x22u)) << 16;\n                                                        "    // l:155
"        PC = int(readIOreg(0x24u)) << 16;\n                                                        "    // l:156
"        PD = int(readIOreg(0x26u)) << 16;\n                                                        "    // l:157
"    }\n                                                                                            "    // l:158
"    else {\n                                                                                       "    // l:159
"        ReferenceLine = ReferenceLine3[y];\n                                                       "    // l:160
"\n                                                                                                 "    // l:161
"        BGX_raw  = readIOreg(0x38u);\n                                                             "    // l:162
"        BGX_raw |= readIOreg(0x3au) << 16;\n                                                       "    // l:163
"        BGY_raw  = readIOreg(0x3cu);\n                                                             "    // l:164
"        BGY_raw |= readIOreg(0x3eu) << 16;\n                                                       "    // l:165
"        PA = int(readIOreg(0x30u)) << 16;\n                                                        "    // l:166
"        PB = int(readIOreg(0x32u)) << 16;\n                                                        "    // l:167
"        PC = int(readIOreg(0x34u)) << 16;\n                                                        "    // l:168
"        PD = int(readIOreg(0x36u)) << 16;\n                                                        "    // l:169
"    }\n                                                                                            "    // l:170
"\n                                                                                                 "    // l:171
"    // convert to signed\n                                                                         "    // l:172
"    int BGX = int(BGX_raw) << 4;\n                                                                 "    // l:173
"    int BGY = int(BGY_raw) << 4;\n                                                                 "    // l:174
"    BGX >>= 4;\n                                                                                   "    // l:175
"    BGY >>= 4;\n                                                                                   "    // l:176
"\n                                                                                                 "    // l:177
"    // was already shifted left\n                                                                  "    // l:178
"    PA >>= 16;\n                                                                                   "    // l:179
"    PB >>= 16;\n                                                                                   "    // l:180
"    PC >>= 16;\n                                                                                   "    // l:181
"    PD >>= 16;\n                                                                                   "    // l:182
"\n                                                                                                 "    // l:183
"    uint CBB, SBB, Size;\n                                                                         "    // l:184
"    bool ColorMode;\n                                                                              "    // l:185
"\n                                                                                                 "    // l:186
"    CBB       = (BGCNT >> 2) & 3u;\n                                                               "    // l:187
"    SBB       = (BGCNT >> 8) & 0x1fu;\n                                                            "    // l:188
"    Size      = AffineBGSizeTable[(BGCNT >> 14) & 3u];\n                                           "    // l:189
"\n                                                                                                 "    // l:190
"    mat2x2 RotationScaling = mat2x2(\n                                                             "    // l:191
"        float(PA), float(PC),  // first column\n                                                   "    // l:192
"        float(PB), float(PD)   // second column\n                                                  "    // l:193
"    );\n                                                                                           "    // l:194
"\n                                                                                                 "    // l:195
"    vec2 pos  = screen_pos - vec2(0, ReferenceLine);\n                                             "    // l:196
"    int x_eff = int(BGX + dot(vec2(PA, PB), pos));\n                                               "    // l:197
"    int y_eff = int(BGY + dot(vec2(PC, PD), pos));\n                                               "    // l:198
"\n                                                                                                 "    // l:199
"    // correct for fixed point math\n                                                              "    // l:200
"    x_eff >>= 8;\n                                                                                 "    // l:201
"    y_eff >>= 8;\n                                                                                 "    // l:202
"\n                                                                                                 "    // l:203
"    if ((x_eff < 0) || (x_eff > Size) || (y_eff < 0) || (y_eff > Size)) {\n                        "    // l:204
"        if ((BGCNT & 0x2000u) == 0) {\n                                                            "    // l:205
"            // no display area overflow\n                                                          "    // l:206
"            discard;\n                                                                             "    // l:207
"        }\n                                                                                        "    // l:208
"\n                                                                                                 "    // l:209
"        // wrapping\n                                                                              "    // l:210
"        x_eff &= int(Size - 1);\n                                                                  "    // l:211
"        y_eff &= int(Size - 1);\n                                                                  "    // l:212
"    }\n                                                                                            "    // l:213
"\n                                                                                                 "    // l:214
"    // mosaic effect\n                                                                             "    // l:215
"    if ((BGCNT & 0x0040u) != 0) {\n                                                                "    // l:216
"        uint MOSAIC = readIOreg(0x4cu);\n                                                          "    // l:217
"        x_eff -= x_eff % int((MOSAIC & 0xfu) + 1);\n                                               "    // l:218
"        y_eff -= y_eff % int(((MOSAIC & 0xf0u) >> 4) + 1);\n                                       "    // l:219
"    }\n                                                                                            "    // l:220
"\n                                                                                                 "    // l:221
"    uint TIDAddress = (SBB << 11u);  // base\n                                                     "    // l:222
"    TIDAddress += ((uint(y_eff) >> 3) * (Size >> 3)) | (uint(x_eff) >> 3);  // tile\n              "    // l:223
"    uint TID = readVRAM8(TIDAddress);\n                                                            "    // l:224
"\n                                                                                                 "    // l:225
"    uint PixelAddress = (CBB << 14) | (TID << 6) | ((uint(y_eff) & 7u) << 3) | (uint(x_eff) & 7u);\n"    // l:226
"    uint VRAMEntry = readVRAM8(PixelAddress);\n                                                    "    // l:227
"\n                                                                                                 "    // l:228
"    // transparent\n                                                                               "    // l:229
"    if (VRAMEntry == 0) {\n                                                                        "    // l:230
"        discard;\n                                                                                 "    // l:231
"    }\n                                                                                            "    // l:232
"\n                                                                                                 "    // l:233
"    return vec4(readPALentry(VRAMEntry).rgb, 1);\n                                                 "    // l:234
"}\n                                                                                                "    // l:235
"\n                                                                                                 "    // l:236
"vec4 mode0(uint, uint);\n                                                                          "    // l:237
"vec4 mode1(uint, uint, vec2);\n                                                                    "    // l:238
"vec4 mode2(uint, uint, vec2);\n                                                                    "    // l:239
"vec4 mode3(uint, uint);\n                                                                          "    // l:240
"vec4 mode4(uint, uint);\n                                                                          "    // l:241
"\n                                                                                                 "    // l:242
"void main() {\n                                                                                    "    // l:243
"    if (BG >= 4) {\n                                                                               "    // l:244
"        // backdrop, highest frag depth\n                                                          "    // l:245
"        gl_FragDepth = 1;\n                                                                        "    // l:246
"        FragColor = vec4(readPALentry(0).rgb, 1);\n                                                "    // l:247
"        return;\n                                                                                  "    // l:248
"    }\n                                                                                            "    // l:249
"\n                                                                                                 "    // l:250
"    uint x = uint(screenCoord.x);\n                                                                "    // l:251
"    uint y = uint(screenCoord.y);\n                                                                "    // l:252
"\n                                                                                                 "    // l:253
"    // disabled by window\n                                                                        "    // l:254
"    if ((getWindow(x, y) & (1u << BG)) == 0) {\n                                                   "    // l:255
"        discard;\n                                                                                 "    // l:256
"    }\n                                                                                            "    // l:257
"\n                                                                                                 "    // l:258
"    uint DISPCNT = readIOreg(0);\n                                                                 "    // l:259
"\n                                                                                                 "    // l:260
"    vec4 outColor;\n                                                                               "    // l:261
"    switch(DISPCNT & 7u) {\n                                                                       "    // l:262
"        case 0u:\n                                                                                 "    // l:263
"            outColor = mode0(x, y);\n                                                              "    // l:264
"            break;\n                                                                               "    // l:265
"        case 1u:\n                                                                                 "    // l:266
"            outColor = mode1(x, y, screenCoord);\n                                                 "    // l:267
"            break;\n                                                                               "    // l:268
"        case 2u:\n                                                                                 "    // l:269
"            outColor = mode2(x, y, screenCoord);\n                                                 "    // l:270
"            break;\n                                                                               "    // l:271
"        case 3u:\n                                                                                 "    // l:272
"            outColor = mode3(x, y);\n                                                              "    // l:273
"            break;\n                                                                               "    // l:274
"        case 4u:\n                                                                                 "    // l:275
"            outColor = mode4(x, y);\n                                                              "    // l:276
"            break;\n                                                                               "    // l:277
"        default:\n                                                                                 "    // l:278
"            outColor = vec4(float(x) / float(240), float(y) / float(160), 1, 1);\n                 "    // l:279
"            break;\n                                                                               "    // l:280
"    }\n                                                                                            "    // l:281
"\n                                                                                                 "    // l:282
"    FragColor = outColor;\n                                                                        "    // l:283
"}\n                                                                                                "    // l:284
"\n                                                                                                 "    // l:285
;


// FragmentHelperSource (from fragment_helpers.glsl, lines 2 to 64)
const char* FragmentHelperSource = 
"#version 430 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"/* GENERAL */\n                                                                                    "    // l:3
"in vec2 OnScreenPos;\n                                                                             "    // l:4
"\n                                                                                                 "    // l:5
"uniform sampler2D PAL;\n                                                                           "    // l:6
"uniform usampler2D IO;\n                                                                           "    // l:7
"uniform isampler1D OAM;\n                                                                          "    // l:8
"uniform usampler2D Window;\n                                                                       "    // l:9
"\n                                                                                                 "    // l:10
"layout (std430, binding = 4) readonly buffer VRAMSSBO\n                                            "    // l:11
"{\n                                                                                                "    // l:12
"    uint VRAM[0x18000u >> 2];\n                                                                    "    // l:13
"};\n                                                                                               "    // l:14
"\n                                                                                                 "    // l:15
"/* same stuff as background program: */\n                                                          "    // l:16
"\n                                                                                                 "    // l:17
"uint readVRAM8(uint address) {\n                                                                   "    // l:18
"    uint alignment = address & 3u;\n                                                               "    // l:19
"    uint value = VRAM[address >> 2];\n                                                             "    // l:20
"    value = (value) >> (alignment << 3u);\n                                                        "    // l:21
"    value &= 0xffu;\n                                                                              "    // l:22
"    return value;\n                                                                                "    // l:23
"}\n                                                                                                "    // l:24
"\n                                                                                                 "    // l:25
"uint readVRAM16(uint address) {\n                                                                  "    // l:26
"    uint alignment = address & 2u;\n                                                               "    // l:27
"    uint value = VRAM[address >> 2];\n                                                             "    // l:28
"    value = (value) >> (alignment << 3u);\n                                                        "    // l:29
"    value &= 0xffffu;\n                                                                            "    // l:30
"    return value;\n                                                                                "    // l:31
"}\n                                                                                                "    // l:32
"\n                                                                                                 "    // l:33
"uint readVRAM32(uint address) {\n                                                                  "    // l:34
"    return VRAM[address >> 2];\n                                                                   "    // l:35
"}\n                                                                                                "    // l:36
"\n                                                                                                 "    // l:37
"uint readIOreg(uint address) {\n                                                                   "    // l:38
"    return texelFetch(\n                                                                           "    // l:39
"        IO, ivec2(address >> 1u, uint(OnScreenPos.y)), 0\n                                         "    // l:40
"    ).x;\n                                                                                         "    // l:41
"}\n                                                                                                "    // l:42
"\n                                                                                                 "    // l:43
"ivec4 readOAMentry(uint index) {\n                                                                 "    // l:44
"    return texelFetch(\n                                                                           "    // l:45
"        OAM, int(index), 0\n                                                                       "    // l:46
"    );\n                                                                                           "    // l:47
"}\n                                                                                                "    // l:48
"\n                                                                                                 "    // l:49
"vec4 readPALentry(uint index) {\n                                                                  "    // l:50
"    // Conveniently, since PAL stores the converted colors already, getting a color from an index is as simple as this:\n"    // l:51
"    return texelFetch(\n                                                                           "    // l:52
"        PAL, ivec2(index, uint(OnScreenPos.y)), 0\n                                                "    // l:53
"    );\n                                                                                           "    // l:54
"}\n                                                                                                "    // l:55
"\n                                                                                                 "    // l:56
"uint getWindow(uint x, uint y) {\n                                                                 "    // l:57
"    return texelFetch(\n                                                                           "    // l:58
"        Window, ivec2(x, y), 0\n                                                                   "    // l:59
"    ).r;\n                                                                                         "    // l:60
"}\n                                                                                                "    // l:61
"\n                                                                                                 "    // l:62
;


// FragmentShaderMode0Source (from fragment_mode0.glsl, lines 2 to 38)
const char* FragmentShaderMode0Source = 
"#version 430 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"uniform uint BG;\n                                                                                 "    // l:3
"\n                                                                                                 "    // l:4
"uint readVRAM8(uint address);\n                                                                    "    // l:5
"uint readVRAM16(uint address);\n                                                                   "    // l:6
"uint readVRAM32(uint address);\n                                                                   "    // l:7
"\n                                                                                                 "    // l:8
"uint readIOreg(uint address);\n                                                                    "    // l:9
"vec4 readPALentry(uint index);\n                                                                   "    // l:10
"\n                                                                                                 "    // l:11
"vec4 regularBGPixel(uint BGCNT, uint x, uint y);\n                                                 "    // l:12
"float getDepth(uint BGCNT);\n                                                                      "    // l:13
"\n                                                                                                 "    // l:14
"vec4 mode0(uint x, uint y) {\n                                                                     "    // l:15
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:16
"\n                                                                                                 "    // l:17
"    uint BGCNT = readIOreg(0x08u + (BG << 1));\n                                                   "    // l:18
"\n                                                                                                 "    // l:19
"    vec4 Color;\n                                                                                  "    // l:20
"    if ((DISPCNT & (0x0100u << BG)) == 0) {\n                                                      "    // l:21
"        discard;  // background disabled\n                                                         "    // l:22
"    }\n                                                                                            "    // l:23
"\n                                                                                                 "    // l:24
"    Color = regularBGPixel(BGCNT, x, y);\n                                                         "    // l:25
"\n                                                                                                 "    // l:26
"    if (Color.w != 0) {\n                                                                          "    // l:27
"        // priority\n                                                                              "    // l:28
"        gl_FragDepth = getDepth(BGCNT);\n                                                          "    // l:29
"        return Color;\n                                                                            "    // l:30
"    }\n                                                                                            "    // l:31
"    else {\n                                                                                       "    // l:32
"        discard;\n                                                                                 "    // l:33
"    }\n                                                                                            "    // l:34
"}\n                                                                                                "    // l:35
"\n                                                                                                 "    // l:36
;


// FragmentShaderMode1Source (from fragment_mode1.glsl, lines 2 to 50)
const char* FragmentShaderMode1Source = 
"#version 430 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"uniform uint BG;\n                                                                                 "    // l:3
"\n                                                                                                 "    // l:4
"uint readVRAM8(uint address);\n                                                                    "    // l:5
"uint readVRAM16(uint address);\n                                                                   "    // l:6
"uint readVRAM32(uint address);\n                                                                   "    // l:7
"\n                                                                                                 "    // l:8
"uint readIOreg(uint address);\n                                                                    "    // l:9
"vec4 readPALentry(uint index);\n                                                                   "    // l:10
"\n                                                                                                 "    // l:11
"vec4 regularBGPixel(uint BGCNT, uint x, uint y);\n                                                 "    // l:12
"vec4 affineBGPixel(uint BGCNT, vec2 screen_pos);\n                                                 "    // l:13
"float getDepth(uint BGCNT);\n                                                                      "    // l:14
"\n                                                                                                 "    // l:15
"vec4 mode1(uint x, uint y, vec2 screen_pos) {\n                                                    "    // l:16
"    if (BG == 3) {\n                                                                               "    // l:17
"        // BG 3 is not drawn\n                                                                     "    // l:18
"        discard;\n                                                                                 "    // l:19
"    }\n                                                                                            "    // l:20
"\n                                                                                                 "    // l:21
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:22
"\n                                                                                                 "    // l:23
"    uint BGCNT = readIOreg(0x08u + (BG << 1));\n                                                   "    // l:24
"\n                                                                                                 "    // l:25
"    vec4 Color;\n                                                                                  "    // l:26
"    if ((DISPCNT & (0x0100u << BG)) == 0) {\n                                                      "    // l:27
"        discard;  // background disabled\n                                                         "    // l:28
"    }\n                                                                                            "    // l:29
"\n                                                                                                 "    // l:30
"\n                                                                                                 "    // l:31
"    if (BG < 2) {\n                                                                                "    // l:32
"        Color = regularBGPixel(BGCNT, x, y);\n                                                     "    // l:33
"    }\n                                                                                            "    // l:34
"    else {\n                                                                                       "    // l:35
"        Color = affineBGPixel(BGCNT, screen_pos);\n                                                "    // l:36
"    }\n                                                                                            "    // l:37
"\n                                                                                                 "    // l:38
"    if (Color.w != 0) {\n                                                                          "    // l:39
"        // background priority\n                                                                   "    // l:40
"        gl_FragDepth = getDepth(BGCNT);\n                                                          "    // l:41
"        return Color;\n                                                                            "    // l:42
"    }\n                                                                                            "    // l:43
"    else {\n                                                                                       "    // l:44
"        discard;\n                                                                                 "    // l:45
"    }\n                                                                                            "    // l:46
"}\n                                                                                                "    // l:47
"\n                                                                                                 "    // l:48
;


// FragmentShaderMode2Source (from fragment_mode2.glsl, lines 2 to 44)
const char* FragmentShaderMode2Source = 
"#version 430 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"uniform uint BG;\n                                                                                 "    // l:3
"\n                                                                                                 "    // l:4
"uint readVRAM8(uint address);\n                                                                    "    // l:5
"uint readVRAM16(uint address);\n                                                                   "    // l:6
"uint readVRAM32(uint address);\n                                                                   "    // l:7
"\n                                                                                                 "    // l:8
"uint readIOreg(uint address);\n                                                                    "    // l:9
"vec4 readPALentry(uint index);\n                                                                   "    // l:10
"\n                                                                                                 "    // l:11
"vec4 regularBGPixel(uint BGCNT, uint x, uint y);\n                                                 "    // l:12
"vec4 affineBGPixel(uint BGCNT, vec2 screen_pos);\n                                                 "    // l:13
"float getDepth(uint BGCNT);\n                                                                      "    // l:14
"\n                                                                                                 "    // l:15
"vec4 mode2(uint x, uint y, vec2 screen_pos) {\n                                                    "    // l:16
"    if (BG < 2) {\n                                                                                "    // l:17
"        // only BG2 and 3 enabled\n                                                                "    // l:18
"        discard;\n                                                                                 "    // l:19
"    }\n                                                                                            "    // l:20
"\n                                                                                                 "    // l:21
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:22
"\n                                                                                                 "    // l:23
"    uint BGCNT = readIOreg(0x08u + (BG << 1));\n                                                   "    // l:24
"\n                                                                                                 "    // l:25
"    vec4 Color;\n                                                                                  "    // l:26
"    if ((DISPCNT & (0x0100u << BG)) == 0) {\n                                                      "    // l:27
"        discard;  // background disabled\n                                                         "    // l:28
"    }\n                                                                                            "    // l:29
"\n                                                                                                 "    // l:30
"    Color = affineBGPixel(BGCNT, screen_pos);\n                                                    "    // l:31
"\n                                                                                                 "    // l:32
"    if (Color.w != 0) {\n                                                                          "    // l:33
"        // BG priority\n                                                                           "    // l:34
"        gl_FragDepth = getDepth(BGCNT);\n                                                          "    // l:35
"        return Color;\n                                                                            "    // l:36
"    }\n                                                                                            "    // l:37
"    else {\n                                                                                       "    // l:38
"        discard;\n                                                                                 "    // l:39
"    }\n                                                                                            "    // l:40
"}\n                                                                                                "    // l:41
"\n                                                                                                 "    // l:42
;


// FragmentShaderMode3Source (from fragment_mode3.glsl, lines 2 to 47)
const char* FragmentShaderMode3Source = 
"#version 430 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"uniform uint BG;\n                                                                                 "    // l:3
"\n                                                                                                 "    // l:4
"uint readVRAM8(uint address);\n                                                                    "    // l:5
"uint readVRAM16(uint address);\n                                                                   "    // l:6
"uint readVRAM32(uint address);\n                                                                   "    // l:7
"\n                                                                                                 "    // l:8
"uint readIOreg(uint address);\n                                                                    "    // l:9
"vec4 readPALentry(uint index);\n                                                                   "    // l:10
"float getDepth(uint BGCNT);\n                                                                      "    // l:11
"\n                                                                                                 "    // l:12
"\n                                                                                                 "    // l:13
"vec4 mode3(uint x, uint y) {\n                                                                     "    // l:14
"    if (BG != 2) {\n                                                                               "    // l:15
"        // only BG2 is drawn\n                                                                     "    // l:16
"        discard;\n                                                                                 "    // l:17
"    }\n                                                                                            "    // l:18
"\n                                                                                                 "    // l:19
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:20
"\n                                                                                                 "    // l:21
"    if ((DISPCNT & 0x0400u) == 0) {\n                                                              "    // l:22
"        // background 2 is disabled\n                                                              "    // l:23
"        discard;\n                                                                                 "    // l:24
"    }\n                                                                                            "    // l:25
"\n                                                                                                 "    // l:26
"    uint VRAMAddr = (240 * y + x) << 1;  // 16bpp\n                                                "    // l:27
"\n                                                                                                 "    // l:28
"    uint PackedColor = readVRAM16(VRAMAddr);\n                                                     "    // l:29
"\n                                                                                                 "    // l:30
"    vec4 Color = vec4(0, 0, 0, 32);  // to be scaled later\n                                       "    // l:31
"\n                                                                                                 "    // l:32
"    // BGR format\n                                                                                "    // l:33
"    Color.r = PackedColor & 0x1fu;\n                                                               "    // l:34
"    PackedColor >>= 5u;\n                                                                          "    // l:35
"    Color.g = PackedColor & 0x1fu;\n                                                               "    // l:36
"    PackedColor >>= 5u;\n                                                                          "    // l:37
"    Color.b = PackedColor & 0x1fu;\n                                                               "    // l:38
"\n                                                                                                 "    // l:39
"    uint BGCNT = readIOreg(0x0cu);\n                                                               "    // l:40
"    gl_FragDepth = getDepth(BGCNT);\n                                                              "    // l:41
"\n                                                                                                 "    // l:42
"    return Color / 32.0;\n                                                                         "    // l:43
"}\n                                                                                                "    // l:44
"\n                                                                                                 "    // l:45
;


// FragmentShaderMode4Source (from fragment_mode4.glsl, lines 2 to 46)
const char* FragmentShaderMode4Source = 
"#version 430 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"uniform uint BG;\n                                                                                 "    // l:3
"\n                                                                                                 "    // l:4
"uint readVRAM8(uint address);\n                                                                    "    // l:5
"uint readVRAM16(uint address);\n                                                                   "    // l:6
"uint readVRAM32(uint address);\n                                                                   "    // l:7
"\n                                                                                                 "    // l:8
"uint readIOreg(uint address);\n                                                                    "    // l:9
"vec4 readPALentry(uint index);\n                                                                   "    // l:10
"float getDepth(uint BGCNT);\n                                                                      "    // l:11
"\n                                                                                                 "    // l:12
"vec4 mode4(uint x, uint y) {\n                                                                     "    // l:13
"    if (BG != 2) {\n                                                                               "    // l:14
"        // only BG2 is drawn\n                                                                     "    // l:15
"        discard;\n                                                                                 "    // l:16
"    }\n                                                                                            "    // l:17
"\n                                                                                                 "    // l:18
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:19
"\n                                                                                                 "    // l:20
"    if ((DISPCNT & 0x0400u) == 0) {\n                                                              "    // l:21
"        // background 2 is disabled\n                                                              "    // l:22
"        discard;\n                                                                                 "    // l:23
"    }\n                                                                                            "    // l:24
"\n                                                                                                 "    // l:25
"    // offset is specified in DISPCNT\n                                                            "    // l:26
"    uint Offset = 0;\n                                                                             "    // l:27
"    if ((DISPCNT & 0x0010u) != 0) {\n                                                              "    // l:28
"        // offset\n                                                                                "    // l:29
"        Offset = 0xa000u;\n                                                                        "    // l:30
"    }\n                                                                                            "    // l:31
"\n                                                                                                 "    // l:32
"    uint VRAMAddr = (240 * y + x);\n                                                               "    // l:33
"    VRAMAddr += Offset;\n                                                                          "    // l:34
"    uint PaletteIndex = readVRAM8(VRAMAddr);\n                                                     "    // l:35
"\n                                                                                                 "    // l:36
"    vec4 Color = readPALentry(PaletteIndex);\n                                                     "    // l:37
"    uint BGCNT = readIOreg(0x0cu);\n                                                               "    // l:38
"    gl_FragDepth = getDepth(BGCNT);;\n                                                             "    // l:39
"\n                                                                                                 "    // l:40
"    // We already converted to BGR when buffering data\n                                           "    // l:41
"    return vec4(Color.rgb, 1.0);\n                                                                 "    // l:42
"}\n                                                                                                "    // l:43
"\n                                                                                                 "    // l:44
;


// ObjectFragmentShaderSource (from object_fragment.glsl, lines 2 to 258)
const char* ObjectFragmentShaderSource = 
"#version 430 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"#define attr0 x\n                                                                                  "    // l:3
"#define attr1 y\n                                                                                  "    // l:4
"#define attr2 z\n                                                                                  "    // l:5
"#define attr3 w\n                                                                                  "    // l:6
"\n                                                                                                 "    // l:7
"in vec2 InObjPos;\n                                                                                "    // l:8
"in vec2 OnScreenPos;\n                                                                             "    // l:9
"flat in uvec4 OBJ;\n                                                                               "    // l:10
"flat in uint ObjWidth;\n                                                                           "    // l:11
"flat in uint ObjHeight;\n                                                                          "    // l:12
"\n                                                                                                 "    // l:13
"uniform uint YClipStart;\n                                                                         "    // l:14
"uniform uint YClipEnd;\n                                                                           "    // l:15
"out vec4 FragColor;\n                                                                              "    // l:16
"out float gl_FragDepth;\n                                                                          "    // l:17
"\n                                                                                                 "    // l:18
"uint readVRAM8(uint address);\n                                                                    "    // l:19
"uint readVRAM16(uint address);\n                                                                   "    // l:20
"uint readVRAM32(uint address);\n                                                                   "    // l:21
"\n                                                                                                 "    // l:22
"uint readIOreg(uint address);\n                                                                    "    // l:23
"ivec4 readOAMentry(uint index);\n                                                                  "    // l:24
"vec4 readPALentry(uint index);\n                                                                   "    // l:25
"\n                                                                                                 "    // l:26
"uint getWindow(uint x, uint y);\n                                                                  "    // l:27
"\n                                                                                                 "    // l:28
"vec4 RegularObject(bool OAM2DMapping) {\n                                                          "    // l:29
"    uint TID = OBJ.attr2 & 0x03ffu;\n                                                              "    // l:30
"    uint Priority = (OBJ.attr2 & 0x0c00u) >> 10;\n                                                 "    // l:31
"    gl_FragDepth = float(Priority) / 4.0;\n                                                        "    // l:32
"\n                                                                                                 "    // l:33
"    uint dx = uint(InObjPos.x);\n                                                                  "    // l:34
"    uint dy = uint(InObjPos.y);\n                                                                  "    // l:35
"\n                                                                                                 "    // l:36
"    if ((getWindow(uint(OnScreenPos.x), uint(OnScreenPos.y)) & 0x10u) == 0) {\n                    "    // l:37
"        // disabled by window\n                                                                    "    // l:38
"        discard;\n                                                                                 "    // l:39
"    }\n                                                                                            "    // l:40
"\n                                                                                                 "    // l:41
"    // mosaic effect\n                                                                             "    // l:42
"    if ((OBJ.attr0 & 0x1000u) != 0) {\n                                                            "    // l:43
"        uint MOSAIC = readIOreg(0x4cu);\n                                                          "    // l:44
"        dx -= dx % (((MOSAIC & 0xf00u) >> 8) + 1);\n                                               "    // l:45
"        dy -= dy % (((MOSAIC & 0xf000u) >> 12) + 1);\n                                             "    // l:46
"    }\n                                                                                            "    // l:47
"\n                                                                                                 "    // l:48
"    uint PixelAddress;\n                                                                           "    // l:49
"    if ((OBJ.attr0 & 0x2000u) == 0x0000u) {\n                                                      "    // l:50
"        uint PaletteBank = OBJ.attr2 >> 12;\n                                                      "    // l:51
"        PixelAddress = TID << 5;\n                                                                 "    // l:52
"\n                                                                                                 "    // l:53
"        // get base address for line of tiles (vertically)\n                                       "    // l:54
"        if (OAM2DMapping) {\n                                                                      "    // l:55
"            PixelAddress += ObjWidth * (dy >> 3) << 2;\n                                           "    // l:56
"        }\n                                                                                        "    // l:57
"        else {\n                                                                                   "    // l:58
"            PixelAddress += 32 * 0x20 * (dy >> 3);\n                                               "    // l:59
"        }\n                                                                                        "    // l:60
"        PixelAddress += (dy & 7u) << 2; // offset within tile for sliver\n                         "    // l:61
"\n                                                                                                 "    // l:62
"        // Sprites VRAM base address is 0x10000\n                                                  "    // l:63
"        PixelAddress = (PixelAddress & 0x7fffu) | 0x10000u;\n                                      "    // l:64
"\n                                                                                                 "    // l:65
"        // horizontal offset:\n                                                                    "    // l:66
"        PixelAddress += (dx >> 3) << 5;    // of tile\n                                            "    // l:67
"        PixelAddress += ((dx & 7u) >> 1);  // in tile\n                                            "    // l:68
"\n                                                                                                 "    // l:69
"        uint VRAMEntry = readVRAM8(PixelAddress);\n                                                "    // l:70
"        if ((dx & 1u) != 0) {\n                                                                    "    // l:71
"            // upper nibble\n                                                                      "    // l:72
"            VRAMEntry >>= 4;\n                                                                     "    // l:73
"        }\n                                                                                        "    // l:74
"        else {\n                                                                                   "    // l:75
"            VRAMEntry &= 0x0fu;\n                                                                  "    // l:76
"        }\n                                                                                        "    // l:77
"\n                                                                                                 "    // l:78
"        if (VRAMEntry != 0) {\n                                                                    "    // l:79
"            return vec4(readPALentry(0x100 + (PaletteBank << 4) + VRAMEntry).rgb, 1);\n            "    // l:80
"        }\n                                                                                        "    // l:81
"        else {\n                                                                                   "    // l:82
"            // transparent\n                                                                       "    // l:83
"            discard;\n                                                                             "    // l:84
"        }\n                                                                                        "    // l:85
"    }\n                                                                                            "    // l:86
"    else {\n                                                                                       "    // l:87
"        // 8bpp\n                                                                                  "    // l:88
"        PixelAddress = TID << 5;\n                                                                 "    // l:89
"\n                                                                                                 "    // l:90
"        if (OAM2DMapping) {\n                                                                      "    // l:91
"            PixelAddress += ObjWidth * (dy & ~7u);\n                                               "    // l:92
"        }\n                                                                                        "    // l:93
"        else {\n                                                                                   "    // l:94
"            PixelAddress += 32 * 0x20 * (dy >> 3);\n                                               "    // l:95
"        }\n                                                                                        "    // l:96
"        PixelAddress += (dy & 7u) << 3;\n                                                          "    // l:97
"\n                                                                                                 "    // l:98
"        // Sprites VRAM base address is 0x10000\n                                                  "    // l:99
"        PixelAddress = (PixelAddress & 0x7fffu) | 0x10000u;\n                                      "    // l:100
"\n                                                                                                 "    // l:101
"        // horizontal offset:\n                                                                    "    // l:102
"        PixelAddress += (dx >> 3) << 6;\n                                                          "    // l:103
"        PixelAddress += dx & 7u;\n                                                                 "    // l:104
"\n                                                                                                 "    // l:105
"        uint VRAMEntry = readVRAM8(PixelAddress);\n                                                "    // l:106
"\n                                                                                                 "    // l:107
"        if (VRAMEntry != 0) {\n                                                                    "    // l:108
"            return vec4(readPALentry(0x100 + VRAMEntry).rgb, 1);\n                                 "    // l:109
"        }\n                                                                                        "    // l:110
"        else {\n                                                                                   "    // l:111
"            // transparent\n                                                                       "    // l:112
"            discard;\n                                                                             "    // l:113
"        }\n                                                                                        "    // l:114
"    }\n                                                                                            "    // l:115
"}\n                                                                                                "    // l:116
"\n                                                                                                 "    // l:117
"bool InsideBox(vec2 v, vec2 bottomLeft, vec2 topRight) {\n                                         "    // l:118
"    vec2 s = step(bottomLeft, v) - step(topRight, v);\n                                            "    // l:119
"    return (s.x * s.y) != 0.0;\n                                                                   "    // l:120
"}\n                                                                                                "    // l:121
"\n                                                                                                 "    // l:122
"vec4 AffineObject(bool OAM2DMapping) {\n                                                           "    // l:123
"    uint TID = OBJ.attr2 & 0x03ffu;\n                                                              "    // l:124
"    uint Priority = (OBJ.attr2 & 0x0c00u) >> 10;\n                                                 "    // l:125
"    gl_FragDepth = float(Priority) / 4.0;\n                                                        "    // l:126
"\n                                                                                                 "    // l:127
"    uint AffineIndex = (OBJ.attr1 & 0x3e00u) >> 9;\n                                               "    // l:128
"    AffineIndex <<= 2;  // goes in groups of 4\n                                                   "    // l:129
"\n                                                                                                 "    // l:130
"    // scaling parameters\n                                                                        "    // l:131
"    int PA = readOAMentry(AffineIndex).attr3;\n                                                    "    // l:132
"    int PB = readOAMentry(AffineIndex + 1).attr3;\n                                                "    // l:133
"    int PC = readOAMentry(AffineIndex + 2).attr3;\n                                                "    // l:134
"    int PD = readOAMentry(AffineIndex + 3).attr3;\n                                                "    // l:135
"\n                                                                                                 "    // l:136
"    // reference point\n                                                                           "    // l:137
"    vec2 p0 = vec2(\n                                                                              "    // l:138
"        float(ObjWidth  >> 1),\n                                                                   "    // l:139
"        float(ObjHeight >> 1)\n                                                                    "    // l:140
"    );\n                                                                                           "    // l:141
"\n                                                                                                 "    // l:142
"    vec2 p1;\n                                                                                     "    // l:143
"    if ((OBJ.attr0 & 0x0300u) == 0x0300u) {\n                                                      "    // l:144
"        // double rendering\n                                                                      "    // l:145
"        p1 = 2 * p0;\n                                                                             "    // l:146
"    }\n                                                                                            "    // l:147
"    else {\n                                                                                       "    // l:148
"        p1 = p0;\n                                                                                 "    // l:149
"    }\n                                                                                            "    // l:150
"\n                                                                                                 "    // l:151
"    mat2x2 rotscale = mat2x2(\n                                                                    "    // l:152
"        float(PA), float(PC),\n                                                                    "    // l:153
"        float(PB), float(PD)\n                                                                     "    // l:154
"    ) / 256.0;  // fixed point stuff\n                                                             "    // l:155
"\n                                                                                                 "    // l:156
"    ivec2 pos = ivec2(rotscale * (InObjPos - p1) + p0);\n                                          "    // l:157
"    if (!InsideBox(pos, vec2(0, 0), vec2(ObjWidth, ObjHeight))) {\n                                "    // l:158
"        // out of bounds\n                                                                         "    // l:159
"        discard;\n                                                                                 "    // l:160
"    }\n                                                                                            "    // l:161
"\n                                                                                                 "    // l:162
"    // mosaic effect\n                                                                             "    // l:163
"    if ((OBJ.attr0 & 0x1000u) != 0) {\n                                                            "    // l:164
"        uint MOSAIC = readIOreg(0x4cu);\n                                                          "    // l:165
"        pos.x -= pos.x % int(((MOSAIC & 0xf00u) >> 8) + 1);\n                                      "    // l:166
"        pos.y -= pos.y % int(((MOSAIC & 0xf000u) >> 12) + 1);\n                                    "    // l:167
"    }\n                                                                                            "    // l:168
"\n                                                                                                 "    // l:169
"    // get actual pixel\n                                                                          "    // l:170
"    uint PixelAddress = 0x10000;  // OBJ VRAM starts at 0x10000 in VRAM\n                          "    // l:171
"    PixelAddress += TID << 5;\n                                                                    "    // l:172
"    if (OAM2DMapping) {\n                                                                          "    // l:173
"        PixelAddress += ObjWidth * (pos.y & ~7) >> 1;\n                                            "    // l:174
"    }\n                                                                                            "    // l:175
"    else {\n                                                                                       "    // l:176
"        PixelAddress += 32 * 0x20 * (pos.y >> 3);\n                                                "    // l:177
"    }\n                                                                                            "    // l:178
"\n                                                                                                 "    // l:179
"    // the rest is very similar to regular sprites:\n                                              "    // l:180
"    if ((OBJ.attr0 & 0x2000u) == 0x0000u) {\n                                                      "    // l:181
"        uint PaletteBank = OBJ.attr2 >> 12;\n                                                      "    // l:182
"        PixelAddress += (pos.y & 7) << 2; // offset within tile for sliver\n                       "    // l:183
"\n                                                                                                 "    // l:184
"        // horizontal offset:\n                                                                    "    // l:185
"        PixelAddress += (pos.x >> 3) << 5;    // of tile\n                                         "    // l:186
"        PixelAddress += (pos.x & 7) >> 1;  // in tile\n                                            "    // l:187
"\n                                                                                                 "    // l:188
"        uint VRAMEntry = readVRAM8(PixelAddress);\n                                                "    // l:189
"        if ((pos.x & 1) != 0) {\n                                                                  "    // l:190
"            // upper nibble\n                                                                      "    // l:191
"            VRAMEntry >>= 4;\n                                                                     "    // l:192
"        }\n                                                                                        "    // l:193
"        else {\n                                                                                   "    // l:194
"            VRAMEntry &= 0x0fu;\n                                                                  "    // l:195
"        }\n                                                                                        "    // l:196
"\n                                                                                                 "    // l:197
"        if (VRAMEntry != 0) {\n                                                                    "    // l:198
"            return vec4(readPALentry(0x100 + (PaletteBank << 4) + VRAMEntry).rgb, 1);\n            "    // l:199
"        }\n                                                                                        "    // l:200
"        else {\n                                                                                   "    // l:201
"            // transparent\n                                                                       "    // l:202
"            discard;\n                                                                             "    // l:203
"        }\n                                                                                        "    // l:204
"    }\n                                                                                            "    // l:205
"    else {\n                                                                                       "    // l:206
"        PixelAddress += (uint(pos.y) & 7u) << 3; // offset within tile for sliver\n                "    // l:207
"\n                                                                                                 "    // l:208
"        // horizontal offset:\n                                                                    "    // l:209
"        PixelAddress += (pos.x >> 3) << 6;  // of tile\n                                           "    // l:210
"        PixelAddress += (pos.x & 7);        // in tile\n                                           "    // l:211
"\n                                                                                                 "    // l:212
"        uint VRAMEntry = readVRAM8(PixelAddress);\n                                                "    // l:213
"\n                                                                                                 "    // l:214
"        if (VRAMEntry != 0) {\n                                                                    "    // l:215
"            return vec4(readPALentry(0x100 + VRAMEntry).rgb, 1);\n                                 "    // l:216
"        }\n                                                                                        "    // l:217
"        else {\n                                                                                   "    // l:218
"            // transparent\n                                                                       "    // l:219
"            discard;\n                                                                             "    // l:220
"        }\n                                                                                        "    // l:221
"    }\n                                                                                            "    // l:222
"}\n                                                                                                "    // l:223
"\n                                                                                                 "    // l:224
"void main() {\n                                                                                    "    // l:225
"    if (OnScreenPos.x < 0) {\n                                                                     "    // l:226
"        discard;\n                                                                                 "    // l:227
"    }\n                                                                                            "    // l:228
"    if (OnScreenPos.x > 240) {\n                                                                   "    // l:229
"        discard;\n                                                                                 "    // l:230
"    }\n                                                                                            "    // l:231
"\n                                                                                                 "    // l:232
"    if (OnScreenPos.y < float(YClipStart)) {\n                                                     "    // l:233
"        discard;\n                                                                                 "    // l:234
"    }\n                                                                                            "    // l:235
"    if (OnScreenPos.y > float(YClipEnd)) {\n                                                       "    // l:236
"        discard;\n                                                                                 "    // l:237
"    }\n                                                                                            "    // l:238
"\n                                                                                                 "    // l:239
"    uint DISPCNT      = readIOreg(0x00u);\n                                                        "    // l:240
"    if ((DISPCNT & 0x1000u) == 0) {\n                                                              "    // l:241
"        // objects disabled in this scanline\n                                                     "    // l:242
"        discard;\n                                                                                 "    // l:243
"    }\n                                                                                            "    // l:244
"\n                                                                                                 "    // l:245
"    bool OAM2DMapping = (DISPCNT & (0x0040u)) != 0;\n                                              "    // l:246
"\n                                                                                                 "    // l:247
"    if ((OBJ.attr0 & 0x0300u) == 0x0000u) {\n                                                      "    // l:248
"        FragColor = RegularObject(OAM2DMapping);\n                                                 "    // l:249
"    }\n                                                                                            "    // l:250
"    else{\n                                                                                        "    // l:251
"        FragColor = AffineObject(OAM2DMapping);\n                                                  "    // l:252
"    }\n                                                                                            "    // l:253
"    // FragColor = vec4(InObjPos.x / float(ObjWidth), InObjPos.y / float(ObjHeight), 1, 1);\n      "    // l:254
"}\n                                                                                                "    // l:255
"\n                                                                                                 "    // l:256
;


// ObjectVertexShaderSource (from object_vertex.glsl, lines 2 to 104)
const char* ObjectVertexShaderSource = 
"#version 430 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"#define attr0 x\n                                                                                  "    // l:3
"#define attr1 y\n                                                                                  "    // l:4
"#define attr2 z\n                                                                                  "    // l:5
"#define attr3 w\n                                                                                  "    // l:6
"\n                                                                                                 "    // l:7
"layout (location = 0) in uvec4 InOBJ;\n                                                            "    // l:8
"\n                                                                                                 "    // l:9
"out vec2 InObjPos;\n                                                                               "    // l:10
"out vec2 OnScreenPos;\n                                                                            "    // l:11
"flat out uvec4 OBJ;\n                                                                              "    // l:12
"flat out uint ObjWidth;\n                                                                          "    // l:13
"flat out uint ObjHeight;\n                                                                         "    // l:14
"\n                                                                                                 "    // l:15
"struct s_ObjSize {\n                                                                               "    // l:16
"    uint width;\n                                                                                  "    // l:17
"    uint height;\n                                                                                 "    // l:18
"};\n                                                                                               "    // l:19
"\n                                                                                                 "    // l:20
"const s_ObjSize ObjSizeTable[3][4] = {\n                                                           "    // l:21
"    { s_ObjSize(8, 8),  s_ObjSize(16, 16), s_ObjSize(32, 32), s_ObjSize(64, 64) },\n               "    // l:22
"    { s_ObjSize(16, 8), s_ObjSize(32, 8),  s_ObjSize(32, 16), s_ObjSize(64, 32) },\n               "    // l:23
"    { s_ObjSize(8, 16), s_ObjSize(8, 32),  s_ObjSize(16, 32), s_ObjSize(32, 62) }\n                "    // l:24
"};\n                                                                                               "    // l:25
"\n                                                                                                 "    // l:26
"struct s_Position {\n                                                                              "    // l:27
"    bool right;\n                                                                                  "    // l:28
"    bool low;\n                                                                                    "    // l:29
"};\n                                                                                               "    // l:30
"\n                                                                                                 "    // l:31
"const s_Position PositionTable[4] = {\n                                                            "    // l:32
"    s_Position(false, false), s_Position(true, false), s_Position(true, true), s_Position(false, true)\n"    // l:33
"};\n                                                                                               "    // l:34
"\n                                                                                                 "    // l:35
"void main() {\n                                                                                    "    // l:36
"    OBJ = InOBJ;\n                                                                                 "    // l:37
"    s_Position Position = PositionTable[gl_VertexID & 3];\n                                        "    // l:38
"\n                                                                                                 "    // l:39
"    uint Shape = OBJ.attr0 >> 14;\n                                                                "    // l:40
"    uint Size  = OBJ.attr1 >> 14;\n                                                                "    // l:41
"\n                                                                                                 "    // l:42
"    s_ObjSize ObjSize = ObjSizeTable[Shape][Size];\n                                               "    // l:43
"    ObjWidth = ObjSize.width;\n                                                                    "    // l:44
"    ObjHeight = ObjSize.height;\n                                                                  "    // l:45
"\n                                                                                                 "    // l:46
"    ivec2 ScreenPos = ivec2(OBJ.attr1 & 0x1ffu, OBJ.attr0 & 0xffu);\n                              "    // l:47
"\n                                                                                                 "    // l:48
"    // correct position for screen wrapping\n                                                      "    // l:49
"    if (ScreenPos.x > int(240)) {\n                                                                "    // l:50
"        ScreenPos.x -= 0x200;\n                                                                    "    // l:51
"    }\n                                                                                            "    // l:52
"\n                                                                                                 "    // l:53
"    if (ScreenPos.y > int(160)) {\n                                                                "    // l:54
"        ScreenPos.y -= 0x100;\n                                                                    "    // l:55
"    }\n                                                                                            "    // l:56
"\n                                                                                                 "    // l:57
"    InObjPos = uvec2(0, 0);\n                                                                      "    // l:58
"    if (Position.right) {\n                                                                        "    // l:59
"        InObjPos.x  += ObjWidth;\n                                                                 "    // l:60
"        ScreenPos.x += int(ObjWidth);\n                                                            "    // l:61
"\n                                                                                                 "    // l:62
"        if ((OBJ.attr0 & 0x0300u) == 0x0300u) {\n                                                  "    // l:63
"            // double rendering\n                                                                  "    // l:64
"            InObjPos.x  += ObjWidth;\n                                                             "    // l:65
"            ScreenPos.x += int(ObjWidth);\n                                                        "    // l:66
"        }\n                                                                                        "    // l:67
"    }\n                                                                                            "    // l:68
"\n                                                                                                 "    // l:69
"    if (Position.low) {\n                                                                          "    // l:70
"        InObjPos.y  += ObjHeight;\n                                                                "    // l:71
"        ScreenPos.y += int(ObjHeight);\n                                                           "    // l:72
"\n                                                                                                 "    // l:73
"        if ((OBJ.attr0 & 0x0300u) == 0x0300u) {\n                                                  "    // l:74
"            // double rendering\n                                                                  "    // l:75
"            InObjPos.y  += ObjHeight;\n                                                            "    // l:76
"            ScreenPos.y += int(ObjHeight);\n                                                       "    // l:77
"        }\n                                                                                        "    // l:78
"    }\n                                                                                            "    // l:79
"\n                                                                                                 "    // l:80
"    // flipping only applies to regular sprites\n                                                  "    // l:81
"    if ((OBJ.attr0 & 0x0300u) == 0x0000u) {\n                                                      "    // l:82
"        if ((OBJ.attr1 & 0x2000u) != 0) {\n                                                        "    // l:83
"            // VFlip\n                                                                             "    // l:84
"            InObjPos.y = ObjHeight - InObjPos.y;\n                                                 "    // l:85
"        }\n                                                                                        "    // l:86
"\n                                                                                                 "    // l:87
"        if ((OBJ.attr1 & 0x1000u) != 0) {\n                                                        "    // l:88
"            // HFlip\n                                                                             "    // l:89
"            InObjPos.x = ObjWidth - InObjPos.x;\n                                                  "    // l:90
"        }\n                                                                                        "    // l:91
"    }\n                                                                                            "    // l:92
"\n                                                                                                 "    // l:93
"    OnScreenPos = vec2(ScreenPos);\n                                                               "    // l:94
"    gl_Position = vec4(\n                                                                          "    // l:95
"        -1.0 + 2.0 * OnScreenPos.x / float(240),\n                                                 "    // l:96
"        1.0 - 2.0 * OnScreenPos.y / float(160),\n                                                  "    // l:97
"        0,\n                                                                                       "    // l:98
"        1\n                                                                                        "    // l:99
"    );\n                                                                                           "    // l:100
"}\n                                                                                                "    // l:101
"\n                                                                                                 "    // l:102
;


// VertexShaderSource (from vertex.glsl, lines 2 to 24)
const char* VertexShaderSource = 
"#version 430 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"layout (location = 0) in vec2 position;\n                                                          "    // l:3
"\n                                                                                                 "    // l:4
"out vec2 screenCoord;\n                                                                            "    // l:5
"out vec2 OnScreenPos;  // needed for fragment_helpers\n                                            "    // l:6
"\n                                                                                                 "    // l:7
"void main() {\n                                                                                    "    // l:8
"    // convert y coordinate from scanline to screen coordinate\n                                   "    // l:9
"    gl_Position = vec4(\n                                                                          "    // l:10
"        position.x,\n                                                                              "    // l:11
"        1.0 - (2.0 * position.y) / float(160), 0, 1\n                                              "    // l:12
"    );\n                                                                                           "    // l:13
"\n                                                                                                 "    // l:14
"    screenCoord = vec2(\n                                                                          "    // l:15
"        float(240) * float((1.0 + position.x)) / 2.0,\n                                            "    // l:16
"        position.y\n                                                                               "    // l:17
"    );\n                                                                                           "    // l:18
"\n                                                                                                 "    // l:19
"    OnScreenPos = screenCoord;\n                                                                   "    // l:20
"}\n                                                                                                "    // l:21
"\n                                                                                                 "    // l:22
;


// WindowFragmentShaderSource (from window_fragment.glsl, lines 2 to 86)
const char* WindowFragmentShaderSource = 
"#version 430 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"in vec2 screenCoord;\n                                                                             "    // l:3
"\n                                                                                                 "    // l:4
"out uvec4 FragColor;\n                                                                             "    // l:5
"\n                                                                                                 "    // l:6
"// BG 1 for BG 0 for obj\n                                                                         "    // l:7
"uniform uint BG;\n                                                                                 "    // l:8
"\n                                                                                                 "    // l:9
"uint readVRAM8(uint address);\n                                                                    "    // l:10
"uint readVRAM16(uint address);\n                                                                   "    // l:11
"uint readVRAM32(uint address);\n                                                                   "    // l:12
"\n                                                                                                 "    // l:13
"uint readIOreg(uint address);\n                                                                    "    // l:14
"vec4 readPALentry(uint index);\n                                                                   "    // l:15
"\n                                                                                                 "    // l:16
"\n                                                                                                 "    // l:17
"void main() {\n                                                                                    "    // l:18
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:19
"\n                                                                                                 "    // l:20
"    if ((DISPCNT & 0xe000u) == 0) {\n                                                              "    // l:21
"        // windows are disabled, enable all windows\n                                              "    // l:22
"        // we should have caught this before rendering, but eh, I guess we'll check again...\n     "    // l:23
"        FragColor.x = 0x3f;\n                                                                      "    // l:24
"        return;\n                                                                                  "    // l:25
"    }\n                                                                                            "    // l:26
"\n                                                                                                 "    // l:27
"    uint x = uint(screenCoord.x);\n                                                                "    // l:28
"    uint y = uint(screenCoord.y);\n                                                                "    // l:29
"\n                                                                                                 "    // l:30
"    // window 0 has higher priority\n                                                              "    // l:31
"    for (uint window = 0; window < 2; window++) {\n                                                "    // l:32
"        if ((DISPCNT & (0x2000u << window)) == 0) {\n                                              "    // l:33
"            // window disabled\n                                                                   "    // l:34
"            continue;\n                                                                            "    // l:35
"        }\n                                                                                        "    // l:36
"\n                                                                                                 "    // l:37
"        uint WINH = readIOreg(0x40u + 2 * window);\n                                               "    // l:38
"        uint WINV = readIOreg(0x44u + 2 * window);\n                                               "    // l:39
"        uint WININ = (readIOreg(0x48u) >> (window * 8)) & 0x3fu;\n                                 "    // l:40
"\n                                                                                                 "    // l:41
"        uint X1 = WINH >> 8;\n                                                                     "    // l:42
"        uint X2 = WINH & 0xffu;\n                                                                  "    // l:43
"        if (X2 > 240) {\n                                                                          "    // l:44
"            X2 = 240;\n                                                                            "    // l:45
"        }\n                                                                                        "    // l:46
"\n                                                                                                 "    // l:47
"        uint Y1 = WINV >> 8;\n                                                                     "    // l:48
"        uint Y2 = WINV & 0xffu;\n                                                                  "    // l:49
"\n                                                                                                 "    // l:50
"        if (Y1 <= Y2) {\n                                                                          "    // l:51
"            // no vert wrap and out of bounds, continue\n                                          "    // l:52
"            if (y < Y1 || y > Y2) {\n                                                              "    // l:53
"                continue;\n                                                                        "    // l:54
"            }\n                                                                                    "    // l:55
"        }\n                                                                                        "    // l:56
"        else {\n                                                                                   "    // l:57
"            // vert wrap and \"in bounds\":\n                                                      "    // l:58
"            if ((y < Y1) && (y > Y2)) {\n                                                          "    // l:59
"                continue;\n                                                                        "    // l:60
"            }\n                                                                                    "    // l:61
"        }\n                                                                                        "    // l:62
"\n                                                                                                 "    // l:63
"        if (X1 <= X2) {\n                                                                          "    // l:64
"            // no hor wrap\n                                                                       "    // l:65
"            if (x >= X1 && x < X2) {\n                                                             "    // l:66
"                // pixel in WININ\n                                                                "    // l:67
"                FragColor.x = WININ;\n                                                             "    // l:68
"                return;\n                                                                          "    // l:69
"            }\n                                                                                    "    // l:70
"        }\n                                                                                        "    // l:71
"        else {\n                                                                                   "    // l:72
"            // hor wrap\n                                                                          "    // l:73
"            if (x < X2 || x >= X1) {\n                                                             "    // l:74
"                // pixel in WININ\n                                                                "    // l:75
"                FragColor.x = WININ;\n                                                             "    // l:76
"                return;\n                                                                          "    // l:77
"            }\n                                                                                    "    // l:78
"        }\n                                                                                        "    // l:79
"    }\n                                                                                            "    // l:80
"\n                                                                                                 "    // l:81
"    FragColor.x = readIOreg(0x4au) & 0x3fu;  // WINOUT\n                                           "    // l:82
"}\n                                                                                                "    // l:83
"\n                                                                                                 "    // l:84
;


// WindowVertexShaderSource (from window_vertex.glsl, lines 2 to 29)
const char* WindowVertexShaderSource = 
"#version 430 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"layout (location = 0) in vec2 position;\n                                                          "    // l:3
"\n                                                                                                 "    // l:4
"uniform uint BG;\n                                                                                 "    // l:5
"\n                                                                                                 "    // l:6
"out vec2 screenCoord;\n                                                                            "    // l:7
"\n                                                                                                 "    // l:8
"void main() {\n                                                                                    "    // l:9
"    if (true) {\n                                                                                  "    // l:10
"        // background\n                                                                            "    // l:11
"        // convert y coordinate from scanline to screen coordinate\n                               "    // l:12
"        gl_Position = vec4(\n                                                                      "    // l:13
"            position.x,\n                                                                          "    // l:14
"            (2.0 * position.y) / float(160) - 1.0, 0, 1\n                                          "    // l:15
"        );\n                                                                                       "    // l:16
"\n                                                                                                 "    // l:17
"        screenCoord = vec2(\n                                                                      "    // l:18
"            float(240) * float((1.0 + position.x)) / 2.0,\n                                        "    // l:19
"            position.y\n                                                                           "    // l:20
"        );\n                                                                                       "    // l:21
"    }\n                                                                                            "    // l:22
"    else {\n                                                                                       "    // l:23
"        // object\n                                                                                "    // l:24
"    }\n                                                                                            "    // l:25
"}\n                                                                                                "    // l:26
"\n                                                                                                 "    // l:27
;

#endif  // GC__SHADER_H