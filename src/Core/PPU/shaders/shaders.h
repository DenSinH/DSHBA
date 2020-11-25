#ifndef GC__SHADER_H
#define GC__SHADER_H

// FragmentShaderSource (from fragment.glsl, lines 2 to 289)
const char* FragmentShaderSource = 
"#version 330 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"in vec2 screenCoord;\n                                                                             "    // l:3
"\n                                                                                                 "    // l:4
"out vec4 FragColor;\n                                                                              "    // l:5
"uniform uint ReferenceLine2[160u];\n                                                               "    // l:6
"uniform uint ReferenceLine3[160u];\n                                                               "    // l:7
"\n                                                                                                 "    // l:8
"// BG 0 - 3 or 4 for backdrop\n                                                                    "    // l:9
"uniform uint BG;\n                                                                                 "    // l:10
"\n                                                                                                 "    // l:11
"vec4 ColorCorrect(vec4 color);\n                                                                   "    // l:12
"\n                                                                                                 "    // l:13
"uint readVRAM8(uint address);\n                                                                    "    // l:14
"uint readVRAM16(uint address);\n                                                                   "    // l:15
"\n                                                                                                 "    // l:16
"uint readVRAM32(uint address);\n                                                                   "    // l:17
"uint readIOreg(uint address);\n                                                                    "    // l:18
"vec4 readPALentry(uint index);\n                                                                   "    // l:19
"\n                                                                                                 "    // l:20
"uint getWindow(uint x, uint y);\n                                                                  "    // l:21
"\n                                                                                                 "    // l:22
"float getDepth(uint BGCNT) {\n                                                                     "    // l:23
"    return ((2.0 * float(BGCNT & 3u)) / 8.0) + (float(1u + BG) / 32.0);\n                          "    // l:24
"}\n                                                                                                "    // l:25
"\n                                                                                                 "    // l:26
"uint VRAMIndex(uint Tx, uint Ty, uint Size) {\n                                                    "    // l:27
"    uint temp = ((Ty & 0x1fu) << 6);\n                                                             "    // l:28
"    temp |= temp | ((Tx & 0x1fu) << 1);\n                                                          "    // l:29
"    switch (Size) {\n                                                                              "    // l:30
"        case 0u:  // 32x32\n                                                                       "    // l:31
"            break;\n                                                                               "    // l:32
"        case 1u:  // 64x32\n                                                                       "    // l:33
"            if ((Tx & 0x20u) != 0u) {\n                                                            "    // l:34
"                temp |= 0x800u;\n                                                                  "    // l:35
"            }\n                                                                                    "    // l:36
"            break;\n                                                                               "    // l:37
"        case 2u:  // 32x64\n                                                                       "    // l:38
"            if ((Ty & 0x20u) != 0u) {\n                                                            "    // l:39
"                temp |= 0x800u;\n                                                                  "    // l:40
"            }\n                                                                                    "    // l:41
"            break;\n                                                                               "    // l:42
"        case 3u:  // 64x64\n                                                                       "    // l:43
"            if ((Ty & 0x20u) != 0u) {\n                                                            "    // l:44
"                temp |= 0x1000u;\n                                                                 "    // l:45
"            }\n                                                                                    "    // l:46
"            if ((Tx & 0x20u) != 0u) {\n                                                            "    // l:47
"                temp |= 0x800u;\n                                                                  "    // l:48
"            }\n                                                                                    "    // l:49
"            break;\n                                                                               "    // l:50
"        default:\n                                                                                 "    // l:51
"            // invalid, should not happen\n                                                        "    // l:52
"            return 0u;\n                                                                           "    // l:53
"    }\n                                                                                            "    // l:54
"    return temp;\n                                                                                 "    // l:55
"}\n                                                                                                "    // l:56
"\n                                                                                                 "    // l:57
"vec4 regularScreenEntryPixel(uint dx, uint dy, uint ScreenEntry, uint CBB, bool ColorMode) {\n     "    // l:58
"    uint PaletteBank = ScreenEntry >> 12;  // 16 bits, we need top 4\n                             "    // l:59
"    if ((ScreenEntry & 0x0800u) != 0u) {\n                                                         "    // l:60
"        // VFlip\n                                                                                 "    // l:61
"        dy = 7u - dy;\n                                                                            "    // l:62
"    }\n                                                                                            "    // l:63
"\n                                                                                                 "    // l:64
"    if ((ScreenEntry & 0x0400u) != 0u) {\n                                                         "    // l:65
"        // HFlip\n                                                                                 "    // l:66
"        dx = 7u - dx;\n                                                                            "    // l:67
"    }\n                                                                                            "    // l:68
"\n                                                                                                 "    // l:69
"    uint TID     = ScreenEntry & 0x3ffu;\n                                                         "    // l:70
"    uint Address = CBB << 14;\n                                                                    "    // l:71
"\n                                                                                                 "    // l:72
"    if (!ColorMode) {\n                                                                            "    // l:73
"        // 4bpp\n                                                                                  "    // l:74
"        Address += TID << 5; // beginning of tile\n                                                "    // l:75
"        Address += dy << 2;  // beginning of sliver\n                                              "    // l:76
"\n                                                                                                 "    // l:77
"        Address += dx >> 1;  // offset into sliver\n                                               "    // l:78
"        uint VRAMEntry = readVRAM8(Address);\n                                                     "    // l:79
"        if ((dx & 1u) != 0u) {\n                                                                   "    // l:80
"            VRAMEntry >>= 4;     // odd x, upper nibble\n                                          "    // l:81
"        }\n                                                                                        "    // l:82
"        else {\n                                                                                   "    // l:83
"            VRAMEntry &= 0xfu;  // even x, lower nibble\n                                          "    // l:84
"        }\n                                                                                        "    // l:85
"\n                                                                                                 "    // l:86
"        if (VRAMEntry != 0u) {\n                                                                   "    // l:87
"            return vec4(readPALentry((PaletteBank << 4) + VRAMEntry).rgb, 1);\n                    "    // l:88
"        }\n                                                                                        "    // l:89
"    }\n                                                                                            "    // l:90
"    else {\n                                                                                       "    // l:91
"        // 8bpp\n                                                                                  "    // l:92
"        Address += TID << 6; // beginning of tile\n                                                "    // l:93
"        Address += dy << 3;  // beginning of sliver\n                                              "    // l:94
"\n                                                                                                 "    // l:95
"        Address += dx;       // offset into sliver\n                                               "    // l:96
"        uint VRAMEntry = readVRAM8(Address);\n                                                     "    // l:97
"\n                                                                                                 "    // l:98
"        if (VRAMEntry != 0u) {\n                                                                   "    // l:99
"            return vec4(readPALentry(VRAMEntry).rgb, 1);\n                                         "    // l:100
"        }\n                                                                                        "    // l:101
"    }\n                                                                                            "    // l:102
"\n                                                                                                 "    // l:103
"    // transparent\n                                                                               "    // l:104
"    discard;\n                                                                                     "    // l:105
"}\n                                                                                                "    // l:106
"\n                                                                                                 "    // l:107
"vec4 regularBGPixel(uint BGCNT, uint x, uint y) {\n                                                "    // l:108
"    uint HOFS, VOFS;\n                                                                             "    // l:109
"    uint CBB, SBB, Size;\n                                                                         "    // l:110
"    bool ColorMode;\n                                                                              "    // l:111
"\n                                                                                                 "    // l:112
"    HOFS  = readIOreg(0x10u + (BG << 2)) & 0x1ffu;\n                                               "    // l:113
"    VOFS  = readIOreg(0x12u + (BG << 2)) & 0x1ffu;\n                                               "    // l:114
"\n                                                                                                 "    // l:115
"    CBB       = (BGCNT >> 2) & 3u;\n                                                               "    // l:116
"    ColorMode = (BGCNT & 0x0080u) == 0x0080u;  // 0: 4bpp, 1: 8bpp\n                               "    // l:117
"    SBB       = (BGCNT >> 8) & 0x1fu;\n                                                            "    // l:118
"    Size      = (BGCNT >> 14) & 3u;\n                                                              "    // l:119
"\n                                                                                                 "    // l:120
"    uint x_eff = (x + HOFS) & 0xffffu;\n                                                           "    // l:121
"    uint y_eff = (y + VOFS) & 0xffffu;\n                                                           "    // l:122
"\n                                                                                                 "    // l:123
"    // mosaic effect\n                                                                             "    // l:124
"    if ((BGCNT & 0x0040u) != 0u) {\n                                                               "    // l:125
"        uint MOSAIC = readIOreg(0x4cu);\n                                                          "    // l:126
"        x_eff -= x_eff % ((MOSAIC & 0xfu) + 1u);\n                                                 "    // l:127
"        y_eff -= y_eff % (((MOSAIC & 0xf0u) >> 4) + 1u);\n                                         "    // l:128
"    }\n                                                                                            "    // l:129
"\n                                                                                                 "    // l:130
"    uint ScreenEntryIndex = VRAMIndex(x_eff >> 3u, y_eff >> 3u, Size);\n                           "    // l:131
"    ScreenEntryIndex += (SBB << 11u);\n                                                            "    // l:132
"    uint ScreenEntry = readVRAM16(ScreenEntryIndex);  // always halfword aligned\n                 "    // l:133
"\n                                                                                                 "    // l:134
"    return regularScreenEntryPixel(x_eff & 7u, y_eff & 7u, ScreenEntry, CBB, ColorMode);\n         "    // l:135
"}\n                                                                                                "    // l:136
"\n                                                                                                 "    // l:137
"const uint AffineBGSizeTable[4] = uint[](\n                                                        "    // l:138
"    128u, 256u, 512u, 1024u\n                                                                      "    // l:139
");\n                                                                                               "    // l:140
"\n                                                                                                 "    // l:141
"vec4 affineBGPixel(uint BGCNT, vec2 screen_pos) {\n                                                "    // l:142
"    uint x = uint(screen_pos.x);\n                                                                 "    // l:143
"    uint y = uint(screen_pos.y);\n                                                                 "    // l:144
"\n                                                                                                 "    // l:145
"    uint ReferenceLine;\n                                                                          "    // l:146
"    uint BGX_raw, BGY_raw;\n                                                                       "    // l:147
"    int PA, PB, PC, PD;\n                                                                          "    // l:148
"    if (BG == 2u) {\n                                                                              "    // l:149
"        ReferenceLine = ReferenceLine2[y];\n                                                       "    // l:150
"\n                                                                                                 "    // l:151
"        BGX_raw  = readIOreg(0x28u);\n                                                             "    // l:152
"        BGX_raw |= readIOreg(0x2au) << 16;\n                                                       "    // l:153
"        BGY_raw  = readIOreg(0x2cu);\n                                                             "    // l:154
"        BGY_raw |= readIOreg(0x2eu) << 16;\n                                                       "    // l:155
"        PA = int(readIOreg(0x20u)) << 16;\n                                                        "    // l:156
"        PB = int(readIOreg(0x22u)) << 16;\n                                                        "    // l:157
"        PC = int(readIOreg(0x24u)) << 16;\n                                                        "    // l:158
"        PD = int(readIOreg(0x26u)) << 16;\n                                                        "    // l:159
"    }\n                                                                                            "    // l:160
"    else {\n                                                                                       "    // l:161
"        ReferenceLine = ReferenceLine3[y];\n                                                       "    // l:162
"\n                                                                                                 "    // l:163
"        BGX_raw  = readIOreg(0x38u);\n                                                             "    // l:164
"        BGX_raw |= readIOreg(0x3au) << 16;\n                                                       "    // l:165
"        BGY_raw  = readIOreg(0x3cu);\n                                                             "    // l:166
"        BGY_raw |= readIOreg(0x3eu) << 16;\n                                                       "    // l:167
"        PA = int(readIOreg(0x30u)) << 16;\n                                                        "    // l:168
"        PB = int(readIOreg(0x32u)) << 16;\n                                                        "    // l:169
"        PC = int(readIOreg(0x34u)) << 16;\n                                                        "    // l:170
"        PD = int(readIOreg(0x36u)) << 16;\n                                                        "    // l:171
"    }\n                                                                                            "    // l:172
"\n                                                                                                 "    // l:173
"    // convert to signed\n                                                                         "    // l:174
"    int BGX = int(BGX_raw) << 4;\n                                                                 "    // l:175
"    int BGY = int(BGY_raw) << 4;\n                                                                 "    // l:176
"    BGX >>= 4;\n                                                                                   "    // l:177
"    BGY >>= 4;\n                                                                                   "    // l:178
"\n                                                                                                 "    // l:179
"    // was already shifted left\n                                                                  "    // l:180
"    PA >>= 16;\n                                                                                   "    // l:181
"    PB >>= 16;\n                                                                                   "    // l:182
"    PC >>= 16;\n                                                                                   "    // l:183
"    PD >>= 16;\n                                                                                   "    // l:184
"\n                                                                                                 "    // l:185
"    uint CBB, SBB, Size;\n                                                                         "    // l:186
"    bool ColorMode;\n                                                                              "    // l:187
"\n                                                                                                 "    // l:188
"    CBB       = (BGCNT >> 2) & 3u;\n                                                               "    // l:189
"    SBB       = (BGCNT >> 8) & 0x1fu;\n                                                            "    // l:190
"    Size      = AffineBGSizeTable[(BGCNT >> 14) & 3u];\n                                           "    // l:191
"\n                                                                                                 "    // l:192
"    mat2x2 RotationScaling = mat2x2(\n                                                             "    // l:193
"        float(PA), float(PC),  // first column\n                                                   "    // l:194
"        float(PB), float(PD)   // second column\n                                                  "    // l:195
"    );\n                                                                                           "    // l:196
"\n                                                                                                 "    // l:197
"    vec2 pos  = screen_pos - vec2(0, ReferenceLine);\n                                             "    // l:198
"    int x_eff = int(BGX + dot(vec2(PA, PB), pos));\n                                               "    // l:199
"    int y_eff = int(BGY + dot(vec2(PC, PD), pos));\n                                               "    // l:200
"\n                                                                                                 "    // l:201
"    // correct for fixed point math\n                                                              "    // l:202
"    x_eff >>= 8;\n                                                                                 "    // l:203
"    y_eff >>= 8;\n                                                                                 "    // l:204
"\n                                                                                                 "    // l:205
"    if ((x_eff < 0) || (x_eff > int(Size)) || (y_eff < 0) || (y_eff > int(Size))) {\n              "    // l:206
"        if ((BGCNT & 0x2000u) == 0u) {\n                                                           "    // l:207
"            // no display area overflow\n                                                          "    // l:208
"            discard;\n                                                                             "    // l:209
"        }\n                                                                                        "    // l:210
"\n                                                                                                 "    // l:211
"        // wrapping\n                                                                              "    // l:212
"        x_eff &= int(Size) - 1;\n                                                                  "    // l:213
"        y_eff &= int(Size) - 1;\n                                                                  "    // l:214
"    }\n                                                                                            "    // l:215
"\n                                                                                                 "    // l:216
"    // mosaic effect\n                                                                             "    // l:217
"    if ((BGCNT & 0x0040u) != 0u) {\n                                                               "    // l:218
"        uint MOSAIC = readIOreg(0x4cu);\n                                                          "    // l:219
"        x_eff -= x_eff % int((MOSAIC & 0xfu) + 1u);\n                                              "    // l:220
"        y_eff -= y_eff % int(((MOSAIC & 0xf0u) >> 4) + 1u);\n                                      "    // l:221
"    }\n                                                                                            "    // l:222
"\n                                                                                                 "    // l:223
"    uint TIDAddress = (SBB << 11u);  // base\n                                                     "    // l:224
"    TIDAddress += ((uint(y_eff) >> 3) * (Size >> 3)) | (uint(x_eff) >> 3);  // tile\n              "    // l:225
"    uint TID = readVRAM8(TIDAddress);\n                                                            "    // l:226
"\n                                                                                                 "    // l:227
"    uint PixelAddress = (CBB << 14) | (TID << 6) | ((uint(y_eff) & 7u) << 3) | (uint(x_eff) & 7u);\n"    // l:228
"    uint VRAMEntry = readVRAM8(PixelAddress);\n                                                    "    // l:229
"\n                                                                                                 "    // l:230
"    // transparent\n                                                                               "    // l:231
"    if (VRAMEntry == 0u) {\n                                                                       "    // l:232
"        discard;\n                                                                                 "    // l:233
"    }\n                                                                                            "    // l:234
"\n                                                                                                 "    // l:235
"    return vec4(readPALentry(VRAMEntry).rgb, 1);\n                                                 "    // l:236
"}\n                                                                                                "    // l:237
"\n                                                                                                 "    // l:238
"vec4 mode0(uint, uint);\n                                                                          "    // l:239
"vec4 mode1(uint, uint, vec2);\n                                                                    "    // l:240
"vec4 mode2(uint, uint, vec2);\n                                                                    "    // l:241
"vec4 mode3(uint, uint);\n                                                                          "    // l:242
"vec4 mode4(uint, uint);\n                                                                          "    // l:243
"\n                                                                                                 "    // l:244
"void main() {\n                                                                                    "    // l:245
"    if (BG >= 4u) {\n                                                                              "    // l:246
"        // backdrop, highest frag depth\n                                                          "    // l:247
"        gl_FragDepth = 1;\n                                                                        "    // l:248
"        FragColor = ColorCorrect(vec4(readPALentry(0u).rgb, 1.0));\n                               "    // l:249
"        return;\n                                                                                  "    // l:250
"    }\n                                                                                            "    // l:251
"\n                                                                                                 "    // l:252
"    uint x = uint(screenCoord.x);\n                                                                "    // l:253
"    uint y = uint(screenCoord.y);\n                                                                "    // l:254
"\n                                                                                                 "    // l:255
"    // disabled by window\n                                                                        "    // l:256
"    if ((getWindow(x, y) & (1u << BG)) == 0u) {\n                                                  "    // l:257
"        discard;\n                                                                                 "    // l:258
"    }\n                                                                                            "    // l:259
"\n                                                                                                 "    // l:260
"    uint DISPCNT = readIOreg(0u);\n                                                                "    // l:261
"\n                                                                                                 "    // l:262
"    vec4 outColor;\n                                                                               "    // l:263
"    switch(DISPCNT & 7u) {\n                                                                       "    // l:264
"        case 0u:\n                                                                                 "    // l:265
"            outColor = mode0(x, y);\n                                                              "    // l:266
"            break;\n                                                                               "    // l:267
"        case 1u:\n                                                                                 "    // l:268
"            outColor = mode1(x, y, screenCoord);\n                                                 "    // l:269
"            break;\n                                                                               "    // l:270
"        case 2u:\n                                                                                 "    // l:271
"            outColor = mode2(x, y, screenCoord);\n                                                 "    // l:272
"            break;\n                                                                               "    // l:273
"        case 3u:\n                                                                                 "    // l:274
"            outColor = mode3(x, y);\n                                                              "    // l:275
"            break;\n                                                                               "    // l:276
"        case 4u:\n                                                                                 "    // l:277
"            outColor = mode4(x, y);\n                                                              "    // l:278
"            break;\n                                                                               "    // l:279
"        default:\n                                                                                 "    // l:280
"            outColor = vec4(float(x) / float(240u), float(y) / float(160u), 1, 1);\n               "    // l:281
"            break;\n                                                                               "    // l:282
"    }\n                                                                                            "    // l:283
"\n                                                                                                 "    // l:284
"    FragColor = ColorCorrect(outColor);\n                                                          "    // l:285
"}\n                                                                                                "    // l:286
"\n                                                                                                 "    // l:287
;


// FragmentHelperSource (from fragment_helpers.glsl, lines 2 to 73)
const char* FragmentHelperSource = 
"#version 330 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"/* GENERAL */\n                                                                                    "    // l:3
"in vec2 OnScreenPos;\n                                                                             "    // l:4
"\n                                                                                                 "    // l:5
"uniform sampler2D PAL;\n                                                                           "    // l:6
"uniform usampler2D VRAM;\n                                                                         "    // l:7
"uniform usampler2D IO;\n                                                                           "    // l:8
"uniform isampler1D OAM;\n                                                                          "    // l:9
"uniform usampler2D Window;\n                                                                       "    // l:10
"\n                                                                                                 "    // l:11
"uniform int PALBufferIndex[160u];\n                                                                "    // l:12
"\n                                                                                                 "    // l:13
"// algorithm from https://byuu.net/video/color-emulation/\n                                        "    // l:14
"const float lcdGamma = 4.0;\n                                                                      "    // l:15
"const float outGamma = 2.2;\n                                                                      "    // l:16
"const mat3x3 CorrectionMatrix = mat3x3(\n                                                          "    // l:17
"        255.0,  10.0,  50.0,\n                                                                     "    // l:18
"         50.0, 230.0,  10.0,\n                                                                     "    // l:19
"          0.0,  30.0, 220.0\n                                                                      "    // l:20
") / 255.0;\n                                                                                       "    // l:21
"\n                                                                                                 "    // l:22
"vec4 ColorCorrect(vec4 color) {\n                                                                  "    // l:23
"    vec3 lrgb = pow(color.rgb, vec3(lcdGamma));\n                                                  "    // l:24
"    vec3 rgb = pow(CorrectionMatrix * lrgb, vec3(1.0 / outGamma)) * (255.0 / 280.0);\n             "    // l:25
"    return vec4(rgb, color.a);\n                                                                   "    // l:26
"}\n                                                                                                "    // l:27
"\n                                                                                                 "    // l:28
"uint readVRAM8(uint address) {\n                                                                   "    // l:29
"    return texelFetch(\n                                                                           "    // l:30
"        VRAM, ivec2(address & 0x7fu, address >> 7u), 0\n                                           "    // l:31
"    ).x;\n                                                                                         "    // l:32
"}\n                                                                                                "    // l:33
"\n                                                                                                 "    // l:34
"uint readVRAM16(uint address) {\n                                                                  "    // l:35
"    address &= ~1u;\n                                                                              "    // l:36
"    uint lsb = readVRAM8(address);\n                                                               "    // l:37
"    return lsb | (readVRAM8(address + 1u) << 8u);\n                                                "    // l:38
"}\n                                                                                                "    // l:39
"\n                                                                                                 "    // l:40
"uint readVRAM32(uint address) {\n                                                                  "    // l:41
"    address &= ~3u;\n                                                                              "    // l:42
"    uint lsh = readVRAM16(address);\n                                                              "    // l:43
"    return lsh | (readVRAM16(address + 2u) << 16u);\n                                              "    // l:44
"}\n                                                                                                "    // l:45
"\n                                                                                                 "    // l:46
"uint readIOreg(uint address) {\n                                                                   "    // l:47
"    return texelFetch(\n                                                                           "    // l:48
"        IO, ivec2(address >> 1u, uint(OnScreenPos.y)), 0\n                                         "    // l:49
"    ).x;\n                                                                                         "    // l:50
"}\n                                                                                                "    // l:51
"\n                                                                                                 "    // l:52
"ivec4 readOAMentry(uint index) {\n                                                                 "    // l:53
"    return texelFetch(\n                                                                           "    // l:54
"        OAM, int(index), 0\n                                                                       "    // l:55
"    );\n                                                                                           "    // l:56
"}\n                                                                                                "    // l:57
"\n                                                                                                 "    // l:58
"vec4 readPALentry(uint index) {\n                                                                  "    // l:59
"    // Conveniently, since PAL stores the converted colors already, getting a color from an index is as simple as this:\n"    // l:60
"    return texelFetch(\n                                                                           "    // l:61
"        PAL, ivec2(index, PALBufferIndex[uint(OnScreenPos.y)]), 0\n                                "    // l:62
"    );\n                                                                                           "    // l:63
"}\n                                                                                                "    // l:64
"\n                                                                                                 "    // l:65
"uint getWindow(uint x, uint y) {\n                                                                 "    // l:66
"    return texelFetch(\n                                                                           "    // l:67
"        Window, ivec2(x, 160u - y), 0\n                                                            "    // l:68
"    ).r;\n                                                                                         "    // l:69
"}\n                                                                                                "    // l:70
"\n                                                                                                 "    // l:71
;


// FragmentShaderMode0Source (from fragment_mode0.glsl, lines 2 to 38)
const char* FragmentShaderMode0Source = 
"#version 330 core\n                                                                                "    // l:1
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
"    if ((DISPCNT & (0x0100u << BG)) == 0u) {\n                                                     "    // l:21
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
"#version 330 core\n                                                                                "    // l:1
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
"    if (BG == 3u) {\n                                                                              "    // l:17
"        // BG 3 is not drawn\n                                                                     "    // l:18
"        discard;\n                                                                                 "    // l:19
"    }\n                                                                                            "    // l:20
"\n                                                                                                 "    // l:21
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:22
"\n                                                                                                 "    // l:23
"    uint BGCNT = readIOreg(0x08u + (BG << 1));\n                                                   "    // l:24
"\n                                                                                                 "    // l:25
"    vec4 Color;\n                                                                                  "    // l:26
"    if ((DISPCNT & (0x0100u << BG)) == 0u) {\n                                                     "    // l:27
"        discard;  // background disabled\n                                                         "    // l:28
"    }\n                                                                                            "    // l:29
"\n                                                                                                 "    // l:30
"\n                                                                                                 "    // l:31
"    if (BG < 2u) {\n                                                                               "    // l:32
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
"#version 330 core\n                                                                                "    // l:1
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
"    if (BG < 2u) {\n                                                                               "    // l:17
"        // only BG2 and 3 enabled\n                                                                "    // l:18
"        discard;\n                                                                                 "    // l:19
"    }\n                                                                                            "    // l:20
"\n                                                                                                 "    // l:21
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:22
"\n                                                                                                 "    // l:23
"    uint BGCNT = readIOreg(0x08u + (BG << 1));\n                                                   "    // l:24
"\n                                                                                                 "    // l:25
"    vec4 Color;\n                                                                                  "    // l:26
"    if ((DISPCNT & (0x0100u << BG)) == 0u) {\n                                                     "    // l:27
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
"#version 330 core\n                                                                                "    // l:1
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
"    if (BG != 2u) {\n                                                                              "    // l:15
"        // only BG2 is drawn\n                                                                     "    // l:16
"        discard;\n                                                                                 "    // l:17
"    }\n                                                                                            "    // l:18
"\n                                                                                                 "    // l:19
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:20
"\n                                                                                                 "    // l:21
"    if ((DISPCNT & 0x0400u) == 0u) {\n                                                             "    // l:22
"        // background 2 is disabled\n                                                              "    // l:23
"        discard;\n                                                                                 "    // l:24
"    }\n                                                                                            "    // l:25
"\n                                                                                                 "    // l:26
"    uint VRAMAddr = (240u * y + x) << 1;  // 16bpp\n                                               "    // l:27
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
"#version 330 core\n                                                                                "    // l:1
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
"    if (BG != 2u) {\n                                                                              "    // l:14
"        // only BG2 is drawn\n                                                                     "    // l:15
"        discard;\n                                                                                 "    // l:16
"    }\n                                                                                            "    // l:17
"\n                                                                                                 "    // l:18
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:19
"\n                                                                                                 "    // l:20
"    if ((DISPCNT & 0x0400u) == 0u) {\n                                                             "    // l:21
"        // background 2 is disabled\n                                                              "    // l:22
"        discard;\n                                                                                 "    // l:23
"    }\n                                                                                            "    // l:24
"\n                                                                                                 "    // l:25
"    // offset is specified in DISPCNT\n                                                            "    // l:26
"    uint Offset = 0u;\n                                                                            "    // l:27
"    if ((DISPCNT & 0x0010u) != 0u) {\n                                                             "    // l:28
"        // offset\n                                                                                "    // l:29
"        Offset = 0xa000u;\n                                                                        "    // l:30
"    }\n                                                                                            "    // l:31
"\n                                                                                                 "    // l:32
"    uint VRAMAddr = (240u * y + x);\n                                                              "    // l:33
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


// ObjectFragmentShaderSource (from object_fragment.glsl, lines 5 to 277)
const char* ObjectFragmentShaderSource = 
"#define attr0 x\n                                                                                  "    // l:1
"#define attr1 y\n                                                                                  "    // l:2
"#define attr2 z\n                                                                                  "    // l:3
"#define attr3 w\n                                                                                  "    // l:4
"\n                                                                                                 "    // l:5
"in vec2 InObjPos;\n                                                                                "    // l:6
"in vec2 OnScreenPos;\n                                                                             "    // l:7
"flat in uvec4 OBJ;\n                                                                               "    // l:8
"flat in uint ObjWidth;\n                                                                           "    // l:9
"flat in uint ObjHeight;\n                                                                          "    // l:10
"\n                                                                                                 "    // l:11
"uniform bool Affine;\n                                                                             "    // l:12
"uniform uint YClipStart;\n                                                                         "    // l:13
"uniform uint YClipEnd;\n                                                                           "    // l:14
"\n                                                                                                 "    // l:15
"#ifdef OBJ_WINDOW\n                                                                                "    // l:16
"    out uvec4 FragColor;\n                                                                         "    // l:17
"#else\n                                                                                            "    // l:18
"    out vec4 FragColor;\n                                                                          "    // l:19
"#endif\n                                                                                           "    // l:20
"\n                                                                                                 "    // l:21
"vec4 ColorCorrect(vec4 color);\n                                                                   "    // l:22
"\n                                                                                                 "    // l:23
"uint readVRAM8(uint address);\n                                                                    "    // l:24
"uint readVRAM16(uint address);\n                                                                   "    // l:25
"uint readVRAM32(uint address);\n                                                                   "    // l:26
"\n                                                                                                 "    // l:27
"uint readIOreg(uint address);\n                                                                    "    // l:28
"ivec4 readOAMentry(uint index);\n                                                                  "    // l:29
"vec4 readPALentry(uint index);\n                                                                   "    // l:30
"\n                                                                                                 "    // l:31
"uint getWindow(uint x, uint y);\n                                                                  "    // l:32
"\n                                                                                                 "    // l:33
"vec4 RegularObject(bool OAM2DMapping) {\n                                                          "    // l:34
"    uint TID = OBJ.attr2 & 0x03ffu;\n                                                              "    // l:35
"\n                                                                                                 "    // l:36
"    uint dx = uint(InObjPos.x);\n                                                                  "    // l:37
"    uint dy = uint(InObjPos.y);\n                                                                  "    // l:38
"\n                                                                                                 "    // l:39
"    // mosaic effect\n                                                                             "    // l:40
"    if ((OBJ.attr0 & 0x1000u) != 0u) {\n                                                           "    // l:41
"        uint MOSAIC = readIOreg(0x4cu);\n                                                          "    // l:42
"        dx -= dx % (((MOSAIC & 0xf00u) >> 8) + 1u);\n                                              "    // l:43
"        dy -= dy % (((MOSAIC & 0xf000u) >> 12) + 1u);\n                                            "    // l:44
"    }\n                                                                                            "    // l:45
"\n                                                                                                 "    // l:46
"    uint PixelAddress;\n                                                                           "    // l:47
"    if ((OBJ.attr0 & 0x2000u) == 0x0000u) {\n                                                      "    // l:48
"        uint PaletteBank = OBJ.attr2 >> 12;\n                                                      "    // l:49
"        PixelAddress = TID << 5;\n                                                                 "    // l:50
"\n                                                                                                 "    // l:51
"        // get base address for line of tiles (vertically)\n                                       "    // l:52
"        if (OAM2DMapping) {\n                                                                      "    // l:53
"            PixelAddress += ObjWidth * (dy >> 3) << 2;\n                                           "    // l:54
"        }\n                                                                                        "    // l:55
"        else {\n                                                                                   "    // l:56
"            PixelAddress += 32u * 0x20u * (dy >> 3);\n                                             "    // l:57
"        }\n                                                                                        "    // l:58
"        PixelAddress += (dy & 7u) << 2; // offset within tile for sliver\n                         "    // l:59
"\n                                                                                                 "    // l:60
"        // Sprites VRAM base address is 0x10000\n                                                  "    // l:61
"        PixelAddress = (PixelAddress & 0x7fffu) | 0x10000u;\n                                      "    // l:62
"\n                                                                                                 "    // l:63
"        // horizontal offset:\n                                                                    "    // l:64
"        PixelAddress += (dx >> 3) << 5;    // of tile\n                                            "    // l:65
"        PixelAddress += ((dx & 7u) >> 1);  // in tile\n                                            "    // l:66
"\n                                                                                                 "    // l:67
"        uint VRAMEntry = readVRAM8(PixelAddress);\n                                                "    // l:68
"        if ((dx & 1u) != 0u) {\n                                                                   "    // l:69
"            // upper nibble\n                                                                      "    // l:70
"            VRAMEntry >>= 4;\n                                                                     "    // l:71
"        }\n                                                                                        "    // l:72
"        else {\n                                                                                   "    // l:73
"            VRAMEntry &= 0x0fu;\n                                                                  "    // l:74
"        }\n                                                                                        "    // l:75
"\n                                                                                                 "    // l:76
"        if (VRAMEntry != 0u) {\n                                                                   "    // l:77
"            return vec4(readPALentry(0x100u + (PaletteBank << 4) + VRAMEntry).rgb, 1);\n           "    // l:78
"        }\n                                                                                        "    // l:79
"        else {\n                                                                                   "    // l:80
"            // transparent\n                                                                       "    // l:81
"            discard;\n                                                                             "    // l:82
"        }\n                                                                                        "    // l:83
"    }\n                                                                                            "    // l:84
"    else {\n                                                                                       "    // l:85
"        // 8bpp\n                                                                                  "    // l:86
"        PixelAddress = TID << 5;\n                                                                 "    // l:87
"\n                                                                                                 "    // l:88
"        if (OAM2DMapping) {\n                                                                      "    // l:89
"            PixelAddress += ObjWidth * (dy & ~7u);\n                                               "    // l:90
"        }\n                                                                                        "    // l:91
"        else {\n                                                                                   "    // l:92
"            PixelAddress += 32u * 0x20u * (dy >> 3);\n                                             "    // l:93
"        }\n                                                                                        "    // l:94
"        PixelAddress += (dy & 7u) << 3;\n                                                          "    // l:95
"\n                                                                                                 "    // l:96
"        // Sprites VRAM base address is 0x10000\n                                                  "    // l:97
"        PixelAddress = (PixelAddress & 0x7fffu) | 0x10000u;\n                                      "    // l:98
"\n                                                                                                 "    // l:99
"        // horizontal offset:\n                                                                    "    // l:100
"        PixelAddress += (dx >> 3) << 6;\n                                                          "    // l:101
"        PixelAddress += dx & 7u;\n                                                                 "    // l:102
"\n                                                                                                 "    // l:103
"        uint VRAMEntry = readVRAM8(PixelAddress);\n                                                "    // l:104
"\n                                                                                                 "    // l:105
"        if (VRAMEntry != 0u) {\n                                                                   "    // l:106
"            return vec4(readPALentry(0x100u + VRAMEntry).rgb, 1);\n                                "    // l:107
"        }\n                                                                                        "    // l:108
"        else {\n                                                                                   "    // l:109
"            // transparent\n                                                                       "    // l:110
"            discard;\n                                                                             "    // l:111
"        }\n                                                                                        "    // l:112
"    }\n                                                                                            "    // l:113
"}\n                                                                                                "    // l:114
"\n                                                                                                 "    // l:115
"bool InsideBox(vec2 v, vec2 bottomLeft, vec2 topRight) {\n                                         "    // l:116
"    vec2 s = step(bottomLeft, v) - step(topRight, v);\n                                            "    // l:117
"    return (s.x * s.y) != 0.0;\n                                                                   "    // l:118
"}\n                                                                                                "    // l:119
"\n                                                                                                 "    // l:120
"vec4 AffineObject(bool OAM2DMapping) {\n                                                           "    // l:121
"    uint TID = OBJ.attr2 & 0x03ffu;\n                                                              "    // l:122
"\n                                                                                                 "    // l:123
"    uint AffineIndex = (OBJ.attr1 & 0x3e00u) >> 9;\n                                               "    // l:124
"    AffineIndex <<= 2;  // goes in groups of 4\n                                                   "    // l:125
"\n                                                                                                 "    // l:126
"    // scaling parameters\n                                                                        "    // l:127
"    int PA = readOAMentry(AffineIndex).attr3;\n                                                    "    // l:128
"    int PB = readOAMentry(AffineIndex + 1u).attr3;\n                                               "    // l:129
"    int PC = readOAMentry(AffineIndex + 2u).attr3;\n                                               "    // l:130
"    int PD = readOAMentry(AffineIndex + 3u).attr3;\n                                               "    // l:131
"\n                                                                                                 "    // l:132
"    // reference point\n                                                                           "    // l:133
"    vec2 p0 = vec2(\n                                                                              "    // l:134
"        float(ObjWidth  >> 1),\n                                                                   "    // l:135
"        float(ObjHeight >> 1)\n                                                                    "    // l:136
"    );\n                                                                                           "    // l:137
"\n                                                                                                 "    // l:138
"    vec2 p1;\n                                                                                     "    // l:139
"    if ((OBJ.attr0 & 0x0300u) == 0x0300u) {\n                                                      "    // l:140
"        // double rendering\n                                                                      "    // l:141
"        p1 = 2 * p0;\n                                                                             "    // l:142
"    }\n                                                                                            "    // l:143
"    else {\n                                                                                       "    // l:144
"        p1 = p0;\n                                                                                 "    // l:145
"    }\n                                                                                            "    // l:146
"\n                                                                                                 "    // l:147
"    mat2x2 rotscale = mat2x2(\n                                                                    "    // l:148
"        float(PA), float(PC),\n                                                                    "    // l:149
"        float(PB), float(PD)\n                                                                     "    // l:150
"    ) / 256.0;  // fixed point stuff\n                                                             "    // l:151
"\n                                                                                                 "    // l:152
"    ivec2 pos = ivec2(rotscale * (InObjPos - p1) + p0);\n                                          "    // l:153
"    if (!InsideBox(pos, vec2(0, 0), vec2(ObjWidth, ObjHeight))) {\n                                "    // l:154
"        // out of bounds\n                                                                         "    // l:155
"        discard;\n                                                                                 "    // l:156
"    }\n                                                                                            "    // l:157
"\n                                                                                                 "    // l:158
"    // mosaic effect\n                                                                             "    // l:159
"    if ((OBJ.attr0 & 0x1000u) != 0u) {\n                                                           "    // l:160
"        uint MOSAIC = readIOreg(0x4cu);\n                                                          "    // l:161
"        pos.x -= pos.x % int(((MOSAIC & 0xf00u) >> 8) + 1u);\n                                     "    // l:162
"        pos.y -= pos.y % int(((MOSAIC & 0xf000u) >> 12) + 1u);\n                                   "    // l:163
"    }\n                                                                                            "    // l:164
"\n                                                                                                 "    // l:165
"    // get actual pixel\n                                                                          "    // l:166
"    uint PixelAddress = 0x10000u;  // OBJ VRAM starts at 0x10000 in VRAM\n                         "    // l:167
"    PixelAddress += TID << 5;\n                                                                    "    // l:168
"    if (OAM2DMapping) {\n                                                                          "    // l:169
"        PixelAddress += ObjWidth * uint(pos.y & ~7) >> 1;\n                                        "    // l:170
"    }\n                                                                                            "    // l:171
"    else {\n                                                                                       "    // l:172
"        PixelAddress += 32u * 0x20u * uint(pos.y >> 3);\n                                          "    // l:173
"    }\n                                                                                            "    // l:174
"\n                                                                                                 "    // l:175
"    // the rest is very similar to regular sprites:\n                                              "    // l:176
"    if ((OBJ.attr0 & 0x2000u) == 0x0000u) {\n                                                      "    // l:177
"        uint PaletteBank = OBJ.attr2 >> 12;\n                                                      "    // l:178
"        PixelAddress += uint(pos.y & 7) << 2; // offset within tile for sliver\n                   "    // l:179
"\n                                                                                                 "    // l:180
"        // horizontal offset:\n                                                                    "    // l:181
"        PixelAddress += uint(pos.x >> 3) << 5;    // of tile\n                                     "    // l:182
"        PixelAddress += uint(pos.x & 7) >> 1;  // in tile\n                                        "    // l:183
"\n                                                                                                 "    // l:184
"        uint VRAMEntry = readVRAM8(PixelAddress);\n                                                "    // l:185
"        if ((pos.x & 1) != 0) {\n                                                                  "    // l:186
"            // upper nibble\n                                                                      "    // l:187
"            VRAMEntry >>= 4;\n                                                                     "    // l:188
"        }\n                                                                                        "    // l:189
"        else {\n                                                                                   "    // l:190
"            VRAMEntry &= 0x0fu;\n                                                                  "    // l:191
"        }\n                                                                                        "    // l:192
"\n                                                                                                 "    // l:193
"        if (VRAMEntry != 0u) {\n                                                                   "    // l:194
"            return vec4(readPALentry(0x100u + (PaletteBank << 4) + VRAMEntry).rgb, 1);\n           "    // l:195
"        }\n                                                                                        "    // l:196
"        else {\n                                                                                   "    // l:197
"            // transparent\n                                                                       "    // l:198
"            discard;\n                                                                             "    // l:199
"        }\n                                                                                        "    // l:200
"    }\n                                                                                            "    // l:201
"    else {\n                                                                                       "    // l:202
"        PixelAddress += (uint(pos.y) & 7u) << 3; // offset within tile for sliver\n                "    // l:203
"\n                                                                                                 "    // l:204
"        // horizontal offset:\n                                                                    "    // l:205
"        PixelAddress += uint(pos.x >> 3) << 6;  // of tile\n                                       "    // l:206
"        PixelAddress += uint(pos.x & 7);        // in tile\n                                       "    // l:207
"\n                                                                                                 "    // l:208
"        uint VRAMEntry = readVRAM8(PixelAddress);\n                                                "    // l:209
"\n                                                                                                 "    // l:210
"        if (VRAMEntry != 0u) {\n                                                                   "    // l:211
"            return vec4(readPALentry(0x100u + VRAMEntry).rgb, 1);\n                                "    // l:212
"        }\n                                                                                        "    // l:213
"        else {\n                                                                                   "    // l:214
"            // transparent\n                                                                       "    // l:215
"            discard;\n                                                                             "    // l:216
"        }\n                                                                                        "    // l:217
"    }\n                                                                                            "    // l:218
"}\n                                                                                                "    // l:219
"\n                                                                                                 "    // l:220
"void main() {\n                                                                                    "    // l:221
"    if (OnScreenPos.x < 0) {\n                                                                     "    // l:222
"        discard;\n                                                                                 "    // l:223
"    }\n                                                                                            "    // l:224
"    if (OnScreenPos.x > 240u) {\n                                                                  "    // l:225
"        discard;\n                                                                                 "    // l:226
"    }\n                                                                                            "    // l:227
"\n                                                                                                 "    // l:228
"    if (OnScreenPos.y < float(YClipStart)) {\n                                                     "    // l:229
"        discard;\n                                                                                 "    // l:230
"    }\n                                                                                            "    // l:231
"    if (OnScreenPos.y > float(YClipEnd)) {\n                                                       "    // l:232
"        discard;\n                                                                                 "    // l:233
"    }\n                                                                                            "    // l:234
"\n                                                                                                 "    // l:235
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:236
"#ifndef OBJ_WINDOW\n                                                                               "    // l:237
"    if ((DISPCNT & 0x1000u) == 0u) {\n                                                             "    // l:238
"        // objects disabled in this scanline\n                                                     "    // l:239
"        discard;\n                                                                                 "    // l:240
"    }\n                                                                                            "    // l:241
"    if ((getWindow(uint(OnScreenPos.x), uint(OnScreenPos.y)) & 0x10u) == 0u) {\n                   "    // l:242
"        // disabled by window\n                                                                    "    // l:243
"        discard;\n                                                                                 "    // l:244
"    }\n                                                                                            "    // l:245
"#else\n                                                                                            "    // l:246
"    if ((DISPCNT & 0x8000u) == 0u) {\n                                                             "    // l:247
"        // object window disabled in this scanline\n                                               "    // l:248
"        discard;\n                                                                                 "    // l:249
"    }\n                                                                                            "    // l:250
"#endif\n                                                                                           "    // l:251
"\n                                                                                                 "    // l:252
"    bool OAM2DMapping = (DISPCNT & (0x0040u)) != 0u;\n                                             "    // l:253
"\n                                                                                                 "    // l:254
"    vec4 Color;\n                                                                                  "    // l:255
"    if (!Affine) {\n                                                                               "    // l:256
"        Color = RegularObject(OAM2DMapping);\n                                                     "    // l:257
"    }\n                                                                                            "    // l:258
"    else{\n                                                                                        "    // l:259
"        Color = AffineObject(OAM2DMapping);\n                                                      "    // l:260
"    }\n                                                                                            "    // l:261
"\n                                                                                                 "    // l:262
"#ifndef OBJ_WINDOW\n                                                                               "    // l:263
"    FragColor = ColorCorrect(Color);\n                                                             "    // l:264
"#else\n                                                                                            "    // l:265
"    // RegularObject/AffineObject will only return if it is nontransparent\n                       "    // l:266
"    uint WINOBJ = (readIOreg(0x4au) >> 8) & 0x3fu;\n                                               "    // l:267
"\n                                                                                                 "    // l:268
"    FragColor.r = WINOBJ;\n                                                                        "    // l:269
"#endif\n                                                                                           "    // l:270
"}\n                                                                                                "    // l:271
"\n                                                                                                 "    // l:272
;


// ObjectVertexShaderSource (from object_vertex.glsl, lines 5 to 124)
const char* ObjectVertexShaderSource = 
"#define attr0 x\n                                                                                  "    // l:1
"#define attr1 y\n                                                                                  "    // l:2
"#define attr2 z\n                                                                                  "    // l:3
"#define attr3 w\n                                                                                  "    // l:4
"\n                                                                                                 "    // l:5
"uniform bool Affine;\n                                                                             "    // l:6
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
"const s_ObjSize ObjSizeTable[12] = s_ObjSize[](\n                                                  "    // l:21
"    s_ObjSize(8u, 8u),  s_ObjSize(16u, 16u), s_ObjSize(32u, 32u), s_ObjSize(64u, 64u),\n           "    // l:22
"    s_ObjSize(16u, 8u), s_ObjSize(32u, 8u),  s_ObjSize(32u, 16u), s_ObjSize(64u, 32u),\n           "    // l:23
"    s_ObjSize(8u, 16u), s_ObjSize(8u, 32u),  s_ObjSize(16u, 32u), s_ObjSize(32u, 62u)\n            "    // l:24
");\n                                                                                               "    // l:25
"\n                                                                                                 "    // l:26
"struct s_Position {\n                                                                              "    // l:27
"    bool right;\n                                                                                  "    // l:28
"    bool low;\n                                                                                    "    // l:29
"};\n                                                                                               "    // l:30
"\n                                                                                                 "    // l:31
"const s_Position PositionTable[4] = s_Position[](\n                                                "    // l:32
"    s_Position(false, false), s_Position(true, false), s_Position(true, true), s_Position(false, true)\n"    // l:33
");\n                                                                                               "    // l:34
"\n                                                                                                 "    // l:35
"void main() {\n                                                                                    "    // l:36
"    OBJ = InOBJ;\n                                                                                 "    // l:37
"    s_Position Position = PositionTable[gl_VertexID & 3];\n                                        "    // l:38
"\n                                                                                                 "    // l:39
"    uint Shape = OBJ.attr0 >> 14;\n                                                                "    // l:40
"    uint Size  = OBJ.attr1 >> 14;\n                                                                "    // l:41
"\n                                                                                                 "    // l:42
"    s_ObjSize ObjSize = ObjSizeTable[(Shape * 4u) + Size];\n                                       "    // l:43
"    ObjWidth = ObjSize.width;\n                                                                    "    // l:44
"    ObjHeight = ObjSize.height;\n                                                                  "    // l:45
"\n                                                                                                 "    // l:46
"    ivec2 ScreenPos = ivec2(OBJ.attr1 & 0x1ffu, OBJ.attr0 & 0xffu);\n                              "    // l:47
"\n                                                                                                 "    // l:48
"    // correct position for screen wrapping\n                                                      "    // l:49
"    if (ScreenPos.x > int(240u)) {\n                                                               "    // l:50
"        ScreenPos.x -= 0x200;\n                                                                    "    // l:51
"    }\n                                                                                            "    // l:52
"\n                                                                                                 "    // l:53
"    if (ScreenPos.y > int(160u)) {\n                                                               "    // l:54
"        ScreenPos.y -= 0x100;\n                                                                    "    // l:55
"    }\n                                                                                            "    // l:56
"\n                                                                                                 "    // l:57
"    InObjPos = uvec2(0, 0);\n                                                                      "    // l:58
"    if (Position.right) {\n                                                                        "    // l:59
"        InObjPos.x  += ObjWidth;\n                                                                 "    // l:60
"        ScreenPos.x += int(ObjWidth);\n                                                            "    // l:61
"\n                                                                                                 "    // l:62
"        if (Affine) {\n                                                                            "    // l:63
"            if ((OBJ.attr0 & 0x0300u) == 0x0300u) {\n                                              "    // l:64
"                // double rendering\n                                                              "    // l:65
"                InObjPos.x  += ObjWidth;\n                                                         "    // l:66
"                ScreenPos.x += int(ObjWidth);\n                                                    "    // l:67
"            }\n                                                                                    "    // l:68
"        }\n                                                                                        "    // l:69
"    }\n                                                                                            "    // l:70
"\n                                                                                                 "    // l:71
"    if (Position.low) {\n                                                                          "    // l:72
"        InObjPos.y  += ObjHeight;\n                                                                "    // l:73
"        ScreenPos.y += int(ObjHeight);\n                                                           "    // l:74
"\n                                                                                                 "    // l:75
"        if (Affine) {\n                                                                            "    // l:76
"            if ((OBJ.attr0 & 0x0300u) == 0x0300u) {\n                                              "    // l:77
"                // double rendering\n                                                              "    // l:78
"                InObjPos.y  += ObjHeight;\n                                                        "    // l:79
"                ScreenPos.y += int(ObjHeight);\n                                                   "    // l:80
"            }\n                                                                                    "    // l:81
"        }\n                                                                                        "    // l:82
"    }\n                                                                                            "    // l:83
"\n                                                                                                 "    // l:84
"    // flipping only applies to regular sprites\n                                                  "    // l:85
"    if (!Affine) {\n                                                                               "    // l:86
"        if ((OBJ.attr1 & 0x2000u) != 0u) {\n                                                       "    // l:87
"            // VFlip\n                                                                             "    // l:88
"            InObjPos.y = ObjHeight - InObjPos.y;\n                                                 "    // l:89
"        }\n                                                                                        "    // l:90
"\n                                                                                                 "    // l:91
"        if ((OBJ.attr1 & 0x1000u) != 0u) {\n                                                       "    // l:92
"            // HFlip\n                                                                             "    // l:93
"            InObjPos.x = ObjWidth - InObjPos.x;\n                                                  "    // l:94
"        }\n                                                                                        "    // l:95
"    }\n                                                                                            "    // l:96
"\n                                                                                                 "    // l:97
"    OnScreenPos = vec2(ScreenPos);\n                                                               "    // l:98
"\n                                                                                                 "    // l:99
"#ifndef OBJ_WINDOW\n                                                                               "    // l:100
"    // depth is the same everywhere in the object anyway\n                                         "    // l:101
"    uint Priority = (OBJ.attr2 & 0x0c00u) >> 10;\n                                                 "    // l:102
"\n                                                                                                 "    // l:103
"    gl_Position = vec4(\n                                                                          "    // l:104
"        -1.0 + 2.0 * OnScreenPos.x / float(240u),\n                                                "    // l:105
"        1 - 2.0 * OnScreenPos.y / float(160u),\n                                                   "    // l:106
"        -1 + float(Priority) / 2.0,  // /2.0 because openGL clips between -1 and 1 (-1 is in front)\n"    // l:107
"        1\n                                                                                        "    // l:108
"    );\n                                                                                           "    // l:109
"#else\n                                                                                            "    // l:110
"    gl_Position = vec4(\n                                                                          "    // l:111
"        -1.0 + 2.0 * OnScreenPos.x / float(240u),\n                                                "    // l:112
"        1 - 2.0 * OnScreenPos.y / float(160u),\n                                                   "    // l:113
"        0.5,  // between WIN1 and WINOUT\n                                                         "    // l:114
"        1\n                                                                                        "    // l:115
"    );\n                                                                                           "    // l:116
"#endif\n                                                                                           "    // l:117
"}\n                                                                                                "    // l:118
"\n                                                                                                 "    // l:119
;


// VertexShaderSource (from vertex.glsl, lines 2 to 24)
const char* VertexShaderSource = 
"#version 330 core\n                                                                                "    // l:1
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
"        1 - (2.0 * position.y) / float(160u), 0, 1\n                                               "    // l:12
"    );\n                                                                                           "    // l:13
"\n                                                                                                 "    // l:14
"    screenCoord = vec2(\n                                                                          "    // l:15
"        float(240u) * float((1.0 + position.x)) / 2.0,\n                                           "    // l:16
"        position.y\n                                                                               "    // l:17
"    );\n                                                                                           "    // l:18
"\n                                                                                                 "    // l:19
"    OnScreenPos = screenCoord;\n                                                                   "    // l:20
"}\n                                                                                                "    // l:21
"\n                                                                                                 "    // l:22
;


// WindowFragmentShaderSource (from window_fragment.glsl, lines 2 to 87)
const char* WindowFragmentShaderSource = 
"#version 330 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"in vec2 screenCoord;\n                                                                             "    // l:3
"\n                                                                                                 "    // l:4
"out uvec4 FragColor;\n                                                                             "    // l:5
"out float gl_FragDepth;\n                                                                          "    // l:6
"\n                                                                                                 "    // l:7
"uint readVRAM8(uint address);\n                                                                    "    // l:8
"uint readVRAM16(uint address);\n                                                                   "    // l:9
"uint readVRAM32(uint address);\n                                                                   "    // l:10
"\n                                                                                                 "    // l:11
"uint readIOreg(uint address);\n                                                                    "    // l:12
"vec4 readPALentry(uint index);\n                                                                   "    // l:13
"\n                                                                                                 "    // l:14
"void main() {\n                                                                                    "    // l:15
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:16
"\n                                                                                                 "    // l:17
"    if ((DISPCNT & 0xe000u) == 0u) {\n                                                             "    // l:18
"        // windows are disabled, enable all windows\n                                              "    // l:19
"        // we should have caught this before rendering, but eh, I guess we'll check again...\n     "    // l:20
"        FragColor.x = 0x3fu;\n                                                                     "    // l:21
"        gl_FragDepth = 1.0;\n                                                                      "    // l:22
"        return;\n                                                                                  "    // l:23
"    }\n                                                                                            "    // l:24
"\n                                                                                                 "    // l:25
"    uint x = uint(screenCoord.x);\n                                                                "    // l:26
"    uint y = uint(screenCoord.y);\n                                                                "    // l:27
"\n                                                                                                 "    // l:28
"    // window 0 has higher priority\n                                                              "    // l:29
"    for (uint window = 0u; window < 2u; window++) {\n                                              "    // l:30
"        if ((DISPCNT & (0x2000u << window)) == 0u) {\n                                             "    // l:31
"            // window disabled\n                                                                   "    // l:32
"            continue;\n                                                                            "    // l:33
"        }\n                                                                                        "    // l:34
"\n                                                                                                 "    // l:35
"        uint WINH = readIOreg(0x40u + 2u * window);\n                                              "    // l:36
"        uint WINV = readIOreg(0x44u + 2u * window);\n                                              "    // l:37
"        uint WININ = (readIOreg(0x48u) >> (window * 8u)) & 0x3fu;\n                                "    // l:38
"\n                                                                                                 "    // l:39
"        uint X1 = WINH >> 8;\n                                                                     "    // l:40
"        uint X2 = WINH & 0xffu;\n                                                                  "    // l:41
"        if (X2 > 240u) {\n                                                                         "    // l:42
"            X2 = 240u;\n                                                                           "    // l:43
"        }\n                                                                                        "    // l:44
"\n                                                                                                 "    // l:45
"        uint Y1 = WINV >> 8;\n                                                                     "    // l:46
"        uint Y2 = WINV & 0xffu;\n                                                                  "    // l:47
"\n                                                                                                 "    // l:48
"        if (Y1 <= Y2) {\n                                                                          "    // l:49
"            // no vert wrap and out of bounds, continue\n                                          "    // l:50
"            if (y < Y1 || y > Y2) {\n                                                              "    // l:51
"                continue;\n                                                                        "    // l:52
"            }\n                                                                                    "    // l:53
"        }\n                                                                                        "    // l:54
"        else {\n                                                                                   "    // l:55
"            // vert wrap and \"in bounds\":\n                                                      "    // l:56
"            if ((y < Y1) && (y > Y2)) {\n                                                          "    // l:57
"                continue;\n                                                                        "    // l:58
"            }\n                                                                                    "    // l:59
"        }\n                                                                                        "    // l:60
"\n                                                                                                 "    // l:61
"        if (X1 <= X2) {\n                                                                          "    // l:62
"            // no hor wrap\n                                                                       "    // l:63
"            if (x >= X1 && x < X2) {\n                                                             "    // l:64
"                // pixel in WININ\n                                                                "    // l:65
"                FragColor.x = WININ;\n                                                             "    // l:66
"                gl_FragDepth = 0.0;\n                                                              "    // l:67
"                return;\n                                                                          "    // l:68
"            }\n                                                                                    "    // l:69
"        }\n                                                                                        "    // l:70
"        else {\n                                                                                   "    // l:71
"            // hor wrap\n                                                                          "    // l:72
"            if (x < X2 || x >= X1) {\n                                                             "    // l:73
"                // pixel in WININ\n                                                                "    // l:74
"                FragColor.x = WININ;\n                                                             "    // l:75
"                gl_FragDepth = 0.0;\n                                                              "    // l:76
"                return;\n                                                                          "    // l:77
"            }\n                                                                                    "    // l:78
"        }\n                                                                                        "    // l:79
"    }\n                                                                                            "    // l:80
"\n                                                                                                 "    // l:81
"    FragColor.x = readIOreg(0x4au) & 0x3fu;  // WINOUT\n                                           "    // l:82
"    gl_FragDepth = 1.0;\n                                                                          "    // l:83
"}\n                                                                                                "    // l:84
"\n                                                                                                 "    // l:85
;

#endif  // GC__SHADER_H