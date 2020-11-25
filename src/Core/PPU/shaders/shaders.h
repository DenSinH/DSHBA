#ifndef GC__SHADER_H
#define GC__SHADER_H

// BlitFragmentShaderSource (from blit_fragment.glsl, lines 2 to 30)
const char* BlitFragmentShaderSource = 
"#version 330 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"in vec2 texCoord;\n                                                                                "    // l:3
"\n                                                                                                 "    // l:4
"uniform sampler2D TopLayer;\n                                                                      "    // l:5
"uniform sampler2D BottomLayer;\n                                                                   "    // l:6
"\n                                                                                                 "    // l:7
"out vec4 FragColor;\n                                                                              "    // l:8
"\n                                                                                                 "    // l:9
"void main() {\n                                                                                    "    // l:10
"    vec4 top = texture(TopLayer, texCoord);\n                                                      "    // l:11
"    vec4 bottom = texture(BottomLayer, texCoord);\n                                                "    // l:12
"\n                                                                                                 "    // l:13
"    // default: pick top\n                                                                         "    // l:14
"    FragColor = vec4(\n                                                                            "    // l:15
"        top.rgb, 1\n                                                                               "    // l:16
"    );\n                                                                                           "    // l:17
"    if ((bottom.a != -1) && (bottom.a <= 0)) {\n                                                   "    // l:18
"        // there was a bottom layer in the bottom framebuffer\n                                    "    // l:19
"        if (top.a >= 0) {\n                                                                        "    // l:20
"            FragColor = vec4(\n                                                                    "    // l:21
"                // correct for how we store bottom alpha\n                                         "    // l:22
"                top.rgb * top.a - 2 * bottom.rgb * (bottom.a + 0.25), 1\n                          "    // l:23
"            );\n                                                                                   "    // l:24
"        }\n                                                                                        "    // l:25
"    }\n                                                                                            "    // l:26
"}\n                                                                                                "    // l:27
"\n                                                                                                 "    // l:28
;


// BlitVertexShaderSource (from blit_vertex.glsl, lines 2 to 20)
const char* BlitVertexShaderSource = 
"#version 330 core\n                                                                                "    // l:1
"\n                                                                                                 "    // l:2
"layout (location = 0) in vec2 position;\n                                                          "    // l:3
"layout (location = 1) in vec2 inTexCoord;\n                                                        "    // l:4
"\n                                                                                                 "    // l:5
"out vec2 texCoord;  // needed for fragment_helpers\n                                               "    // l:6
"\n                                                                                                 "    // l:7
"void main() {\n                                                                                    "    // l:8
"    texCoord = inTexCoord;\n                                                                       "    // l:9
"\n                                                                                                 "    // l:10
"    gl_Position = vec4(\n                                                                          "    // l:11
"        position.x,\n                                                                              "    // l:12
"        position.y,\n                                                                              "    // l:13
"        0,\n                                                                                       "    // l:14
"        1\n                                                                                        "    // l:15
"    );\n                                                                                           "    // l:16
"}\n                                                                                                "    // l:17
"\n                                                                                                 "    // l:18
;


// FragmentShaderSource (from fragment.glsl, lines 2 to 301)
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
"void CheckBottom(uint layer, uint window);\n                                                       "    // l:13
"vec4 AlphaCorrect(vec4 color, uint layer, uint window);\n                                          "    // l:14
"\n                                                                                                 "    // l:15
"uint readVRAM8(uint address);\n                                                                    "    // l:16
"uint readVRAM16(uint address);\n                                                                   "    // l:17
"\n                                                                                                 "    // l:18
"uint readVRAM32(uint address);\n                                                                   "    // l:19
"uint readIOreg(uint address);\n                                                                    "    // l:20
"vec4 readPALentry(uint index);\n                                                                   "    // l:21
"\n                                                                                                 "    // l:22
"uint getWindow(uint x, uint y);\n                                                                  "    // l:23
"\n                                                                                                 "    // l:24
"float getDepth(uint BGCNT) {\n                                                                     "    // l:25
"    return ((2.0 * float(BGCNT & 3u)) / 8.0) + (float(1u + BG) / 32.0);\n                          "    // l:26
"}\n                                                                                                "    // l:27
"\n                                                                                                 "    // l:28
"uint VRAMIndex(uint Tx, uint Ty, uint Size) {\n                                                    "    // l:29
"    uint temp = ((Ty & 0x1fu) << 6);\n                                                             "    // l:30
"    temp |= temp | ((Tx & 0x1fu) << 1);\n                                                          "    // l:31
"    switch (Size) {\n                                                                              "    // l:32
"        case 0u:  // 32x32\n                                                                       "    // l:33
"            break;\n                                                                               "    // l:34
"        case 1u:  // 64x32\n                                                                       "    // l:35
"            if ((Tx & 0x20u) != 0u) {\n                                                            "    // l:36
"                temp |= 0x800u;\n                                                                  "    // l:37
"            }\n                                                                                    "    // l:38
"            break;\n                                                                               "    // l:39
"        case 2u:  // 32x64\n                                                                       "    // l:40
"            if ((Ty & 0x20u) != 0u) {\n                                                            "    // l:41
"                temp |= 0x800u;\n                                                                  "    // l:42
"            }\n                                                                                    "    // l:43
"            break;\n                                                                               "    // l:44
"        case 3u:  // 64x64\n                                                                       "    // l:45
"            if ((Ty & 0x20u) != 0u) {\n                                                            "    // l:46
"                temp |= 0x1000u;\n                                                                 "    // l:47
"            }\n                                                                                    "    // l:48
"            if ((Tx & 0x20u) != 0u) {\n                                                            "    // l:49
"                temp |= 0x800u;\n                                                                  "    // l:50
"            }\n                                                                                    "    // l:51
"            break;\n                                                                               "    // l:52
"        default:\n                                                                                 "    // l:53
"            // invalid, should not happen\n                                                        "    // l:54
"            return 0u;\n                                                                           "    // l:55
"    }\n                                                                                            "    // l:56
"    return temp;\n                                                                                 "    // l:57
"}\n                                                                                                "    // l:58
"\n                                                                                                 "    // l:59
"vec4 regularScreenEntryPixel(uint dx, uint dy, uint ScreenEntry, uint CBB, bool ColorMode) {\n     "    // l:60
"    uint PaletteBank = ScreenEntry >> 12;  // 16 bits, we need top 4\n                             "    // l:61
"    if ((ScreenEntry & 0x0800u) != 0u) {\n                                                         "    // l:62
"        // VFlip\n                                                                                 "    // l:63
"        dy = 7u - dy;\n                                                                            "    // l:64
"    }\n                                                                                            "    // l:65
"\n                                                                                                 "    // l:66
"    if ((ScreenEntry & 0x0400u) != 0u) {\n                                                         "    // l:67
"        // HFlip\n                                                                                 "    // l:68
"        dx = 7u - dx;\n                                                                            "    // l:69
"    }\n                                                                                            "    // l:70
"\n                                                                                                 "    // l:71
"    uint TID     = ScreenEntry & 0x3ffu;\n                                                         "    // l:72
"    uint Address = CBB << 14;\n                                                                    "    // l:73
"\n                                                                                                 "    // l:74
"    if (!ColorMode) {\n                                                                            "    // l:75
"        // 4bpp\n                                                                                  "    // l:76
"        Address += TID << 5; // beginning of tile\n                                                "    // l:77
"        Address += dy << 2;  // beginning of sliver\n                                              "    // l:78
"\n                                                                                                 "    // l:79
"        Address += dx >> 1;  // offset into sliver\n                                               "    // l:80
"        uint VRAMEntry = readVRAM8(Address);\n                                                     "    // l:81
"        if ((dx & 1u) != 0u) {\n                                                                   "    // l:82
"            VRAMEntry >>= 4;     // odd x, upper nibble\n                                          "    // l:83
"        }\n                                                                                        "    // l:84
"        else {\n                                                                                   "    // l:85
"            VRAMEntry &= 0xfu;  // even x, lower nibble\n                                          "    // l:86
"        }\n                                                                                        "    // l:87
"\n                                                                                                 "    // l:88
"        if (VRAMEntry != 0u) {\n                                                                   "    // l:89
"            return vec4(readPALentry((PaletteBank << 4) + VRAMEntry).rgb, 1);\n                    "    // l:90
"        }\n                                                                                        "    // l:91
"    }\n                                                                                            "    // l:92
"    else {\n                                                                                       "    // l:93
"        // 8bpp\n                                                                                  "    // l:94
"        Address += TID << 6; // beginning of tile\n                                                "    // l:95
"        Address += dy << 3;  // beginning of sliver\n                                              "    // l:96
"\n                                                                                                 "    // l:97
"        Address += dx;       // offset into sliver\n                                               "    // l:98
"        uint VRAMEntry = readVRAM8(Address);\n                                                     "    // l:99
"\n                                                                                                 "    // l:100
"        if (VRAMEntry != 0u) {\n                                                                   "    // l:101
"            return vec4(readPALentry(VRAMEntry).rgb, 1);\n                                         "    // l:102
"        }\n                                                                                        "    // l:103
"    }\n                                                                                            "    // l:104
"\n                                                                                                 "    // l:105
"    // transparent\n                                                                               "    // l:106
"    discard;\n                                                                                     "    // l:107
"}\n                                                                                                "    // l:108
"\n                                                                                                 "    // l:109
"vec4 regularBGPixel(uint BGCNT, uint x, uint y) {\n                                                "    // l:110
"    uint HOFS, VOFS;\n                                                                             "    // l:111
"    uint CBB, SBB, Size;\n                                                                         "    // l:112
"    bool ColorMode;\n                                                                              "    // l:113
"\n                                                                                                 "    // l:114
"    HOFS  = readIOreg(0x10u + (BG << 2)) & 0x1ffu;\n                                               "    // l:115
"    VOFS  = readIOreg(0x12u + (BG << 2)) & 0x1ffu;\n                                               "    // l:116
"\n                                                                                                 "    // l:117
"    CBB       = (BGCNT >> 2) & 3u;\n                                                               "    // l:118
"    ColorMode = (BGCNT & 0x0080u) == 0x0080u;  // 0: 4bpp, 1: 8bpp\n                               "    // l:119
"    SBB       = (BGCNT >> 8) & 0x1fu;\n                                                            "    // l:120
"    Size      = (BGCNT >> 14) & 3u;\n                                                              "    // l:121
"\n                                                                                                 "    // l:122
"    uint x_eff = (x + HOFS) & 0xffffu;\n                                                           "    // l:123
"    uint y_eff = (y + VOFS) & 0xffffu;\n                                                           "    // l:124
"\n                                                                                                 "    // l:125
"    // mosaic effect\n                                                                             "    // l:126
"    if ((BGCNT & 0x0040u) != 0u) {\n                                                               "    // l:127
"        uint MOSAIC = readIOreg(0x4cu);\n                                                          "    // l:128
"        x_eff -= x_eff % ((MOSAIC & 0xfu) + 1u);\n                                                 "    // l:129
"        y_eff -= y_eff % (((MOSAIC & 0xf0u) >> 4) + 1u);\n                                         "    // l:130
"    }\n                                                                                            "    // l:131
"\n                                                                                                 "    // l:132
"    uint ScreenEntryIndex = VRAMIndex(x_eff >> 3u, y_eff >> 3u, Size);\n                           "    // l:133
"    ScreenEntryIndex += (SBB << 11u);\n                                                            "    // l:134
"    uint ScreenEntry = readVRAM16(ScreenEntryIndex);  // always halfword aligned\n                 "    // l:135
"\n                                                                                                 "    // l:136
"    return regularScreenEntryPixel(x_eff & 7u, y_eff & 7u, ScreenEntry, CBB, ColorMode);\n         "    // l:137
"}\n                                                                                                "    // l:138
"\n                                                                                                 "    // l:139
"const uint AffineBGSizeTable[4] = uint[](\n                                                        "    // l:140
"    128u, 256u, 512u, 1024u\n                                                                      "    // l:141
");\n                                                                                               "    // l:142
"\n                                                                                                 "    // l:143
"vec4 affineBGPixel(uint BGCNT, vec2 screen_pos) {\n                                                "    // l:144
"    uint x = uint(screen_pos.x);\n                                                                 "    // l:145
"    uint y = uint(screen_pos.y);\n                                                                 "    // l:146
"\n                                                                                                 "    // l:147
"    uint ReferenceLine;\n                                                                          "    // l:148
"    uint BGX_raw, BGY_raw;\n                                                                       "    // l:149
"    int PA, PB, PC, PD;\n                                                                          "    // l:150
"    if (BG == 2u) {\n                                                                              "    // l:151
"        ReferenceLine = ReferenceLine2[y];\n                                                       "    // l:152
"\n                                                                                                 "    // l:153
"        BGX_raw  = readIOreg(0x28u);\n                                                             "    // l:154
"        BGX_raw |= readIOreg(0x2au) << 16;\n                                                       "    // l:155
"        BGY_raw  = readIOreg(0x2cu);\n                                                             "    // l:156
"        BGY_raw |= readIOreg(0x2eu) << 16;\n                                                       "    // l:157
"        PA = int(readIOreg(0x20u)) << 16;\n                                                        "    // l:158
"        PB = int(readIOreg(0x22u)) << 16;\n                                                        "    // l:159
"        PC = int(readIOreg(0x24u)) << 16;\n                                                        "    // l:160
"        PD = int(readIOreg(0x26u)) << 16;\n                                                        "    // l:161
"    }\n                                                                                            "    // l:162
"    else {\n                                                                                       "    // l:163
"        ReferenceLine = ReferenceLine3[y];\n                                                       "    // l:164
"\n                                                                                                 "    // l:165
"        BGX_raw  = readIOreg(0x38u);\n                                                             "    // l:166
"        BGX_raw |= readIOreg(0x3au) << 16;\n                                                       "    // l:167
"        BGY_raw  = readIOreg(0x3cu);\n                                                             "    // l:168
"        BGY_raw |= readIOreg(0x3eu) << 16;\n                                                       "    // l:169
"        PA = int(readIOreg(0x30u)) << 16;\n                                                        "    // l:170
"        PB = int(readIOreg(0x32u)) << 16;\n                                                        "    // l:171
"        PC = int(readIOreg(0x34u)) << 16;\n                                                        "    // l:172
"        PD = int(readIOreg(0x36u)) << 16;\n                                                        "    // l:173
"    }\n                                                                                            "    // l:174
"\n                                                                                                 "    // l:175
"    // convert to signed\n                                                                         "    // l:176
"    int BGX = int(BGX_raw) << 4;\n                                                                 "    // l:177
"    int BGY = int(BGY_raw) << 4;\n                                                                 "    // l:178
"    BGX >>= 4;\n                                                                                   "    // l:179
"    BGY >>= 4;\n                                                                                   "    // l:180
"\n                                                                                                 "    // l:181
"    // was already shifted left\n                                                                  "    // l:182
"    PA >>= 16;\n                                                                                   "    // l:183
"    PB >>= 16;\n                                                                                   "    // l:184
"    PC >>= 16;\n                                                                                   "    // l:185
"    PD >>= 16;\n                                                                                   "    // l:186
"\n                                                                                                 "    // l:187
"    uint CBB, SBB, Size;\n                                                                         "    // l:188
"    bool ColorMode;\n                                                                              "    // l:189
"\n                                                                                                 "    // l:190
"    CBB       = (BGCNT >> 2) & 3u;\n                                                               "    // l:191
"    SBB       = (BGCNT >> 8) & 0x1fu;\n                                                            "    // l:192
"    Size      = AffineBGSizeTable[(BGCNT >> 14) & 3u];\n                                           "    // l:193
"\n                                                                                                 "    // l:194
"    mat2x2 RotationScaling = mat2x2(\n                                                             "    // l:195
"        float(PA), float(PC),  // first column\n                                                   "    // l:196
"        float(PB), float(PD)   // second column\n                                                  "    // l:197
"    );\n                                                                                           "    // l:198
"\n                                                                                                 "    // l:199
"    vec2 pos  = screen_pos - vec2(0, ReferenceLine);\n                                             "    // l:200
"    int x_eff = int(BGX + dot(vec2(PA, PB), pos));\n                                               "    // l:201
"    int y_eff = int(BGY + dot(vec2(PC, PD), pos));\n                                               "    // l:202
"\n                                                                                                 "    // l:203
"    // correct for fixed point math\n                                                              "    // l:204
"    x_eff >>= 8;\n                                                                                 "    // l:205
"    y_eff >>= 8;\n                                                                                 "    // l:206
"\n                                                                                                 "    // l:207
"    if ((x_eff < 0) || (x_eff > int(Size)) || (y_eff < 0) || (y_eff > int(Size))) {\n              "    // l:208
"        if ((BGCNT & 0x2000u) == 0u) {\n                                                           "    // l:209
"            // no display area overflow\n                                                          "    // l:210
"            discard;\n                                                                             "    // l:211
"        }\n                                                                                        "    // l:212
"\n                                                                                                 "    // l:213
"        // wrapping\n                                                                              "    // l:214
"        x_eff &= int(Size) - 1;\n                                                                  "    // l:215
"        y_eff &= int(Size) - 1;\n                                                                  "    // l:216
"    }\n                                                                                            "    // l:217
"\n                                                                                                 "    // l:218
"    // mosaic effect\n                                                                             "    // l:219
"    if ((BGCNT & 0x0040u) != 0u) {\n                                                               "    // l:220
"        uint MOSAIC = readIOreg(0x4cu);\n                                                          "    // l:221
"        x_eff -= x_eff % int((MOSAIC & 0xfu) + 1u);\n                                              "    // l:222
"        y_eff -= y_eff % int(((MOSAIC & 0xf0u) >> 4) + 1u);\n                                      "    // l:223
"    }\n                                                                                            "    // l:224
"\n                                                                                                 "    // l:225
"    uint TIDAddress = (SBB << 11u);  // base\n                                                     "    // l:226
"    TIDAddress += ((uint(y_eff) >> 3) * (Size >> 3)) | (uint(x_eff) >> 3);  // tile\n              "    // l:227
"    uint TID = readVRAM8(TIDAddress);\n                                                            "    // l:228
"\n                                                                                                 "    // l:229
"    uint PixelAddress = (CBB << 14) | (TID << 6) | ((uint(y_eff) & 7u) << 3) | (uint(x_eff) & 7u);\n"    // l:230
"    uint VRAMEntry = readVRAM8(PixelAddress);\n                                                    "    // l:231
"\n                                                                                                 "    // l:232
"    // transparent\n                                                                               "    // l:233
"    if (VRAMEntry == 0u) {\n                                                                       "    // l:234
"        discard;\n                                                                                 "    // l:235
"    }\n                                                                                            "    // l:236
"\n                                                                                                 "    // l:237
"    return vec4(readPALentry(VRAMEntry).rgb, 1);\n                                                 "    // l:238
"}\n                                                                                                "    // l:239
"\n                                                                                                 "    // l:240
"vec4 mode0(uint, uint);\n                                                                          "    // l:241
"vec4 mode1(uint, uint, vec2);\n                                                                    "    // l:242
"vec4 mode2(uint, uint, vec2);\n                                                                    "    // l:243
"vec4 mode3(uint, uint);\n                                                                          "    // l:244
"vec4 mode4(uint, uint);\n                                                                          "    // l:245
"\n                                                                                                 "    // l:246
"void main() {\n                                                                                    "    // l:247
"    uint x = uint(screenCoord.x);\n                                                                "    // l:248
"    uint y = uint(screenCoord.y);\n                                                                "    // l:249
"\n                                                                                                 "    // l:250
"    uint window = getWindow(x, y);\n                                                               "    // l:251
"    uint BLDCNT = readIOreg(0x50u);\n                                                              "    // l:252
"\n                                                                                                 "    // l:253
"    if (BG >= 4u) {\n                                                                              "    // l:254
"        CheckBottom(5u, window);\n                                                                 "    // l:255
"\n                                                                                                 "    // l:256
"        // backdrop, highest frag depth\n                                                          "    // l:257
"        gl_FragDepth = 1;\n                                                                        "    // l:258
"        FragColor = ColorCorrect(vec4(readPALentry(0u).rgb, 1.0));\n                               "    // l:259
"        FragColor = AlphaCorrect(FragColor, 5u, window);\n                                         "    // l:260
"        return;\n                                                                                  "    // l:261
"    }\n                                                                                            "    // l:262
"\n                                                                                                 "    // l:263
"    // check if we are rendering on the bottom layer, and if we even need to render this fragment\n"    // l:264
"    CheckBottom(BG, window);\n                                                                     "    // l:265
"\n                                                                                                 "    // l:266
"    // disabled by window\n                                                                        "    // l:267
"    if ((window & (1u << BG)) == 0u) {\n                                                           "    // l:268
"        discard;\n                                                                                 "    // l:269
"    }\n                                                                                            "    // l:270
"\n                                                                                                 "    // l:271
"    uint DISPCNT = readIOreg(0u);\n                                                                "    // l:272
"\n                                                                                                 "    // l:273
"    vec4 outColor;\n                                                                               "    // l:274
"    switch(DISPCNT & 7u) {\n                                                                       "    // l:275
"        case 0u:\n                                                                                 "    // l:276
"            outColor = mode0(x, y);\n                                                              "    // l:277
"            break;\n                                                                               "    // l:278
"        case 1u:\n                                                                                 "    // l:279
"            outColor = mode1(x, y, screenCoord);\n                                                 "    // l:280
"            break;\n                                                                               "    // l:281
"        case 2u:\n                                                                                 "    // l:282
"            outColor = mode2(x, y, screenCoord);\n                                                 "    // l:283
"            break;\n                                                                               "    // l:284
"        case 3u:\n                                                                                 "    // l:285
"            outColor = mode3(x, y);\n                                                              "    // l:286
"            break;\n                                                                               "    // l:287
"        case 4u:\n                                                                                 "    // l:288
"            outColor = mode4(x, y);\n                                                              "    // l:289
"            break;\n                                                                               "    // l:290
"        default:\n                                                                                 "    // l:291
"            outColor = vec4(float(x) / float(240u), float(y) / float(160u), 1, 1);\n               "    // l:292
"            break;\n                                                                               "    // l:293
"    }\n                                                                                            "    // l:294
"\n                                                                                                 "    // l:295
"    FragColor = ColorCorrect(outColor);\n                                                          "    // l:296
"    FragColor = AlphaCorrect(FragColor, BG, window);\n                                             "    // l:297
"}\n                                                                                                "    // l:298
"\n                                                                                                 "    // l:299
;


// FragmentHelperSource (from fragment_helpers.glsl, lines 2 to 158)
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
"uniform bool Bottom;\n                                                                             "    // l:12
"\n                                                                                                 "    // l:13
"uniform int PALBufferIndex[160u];\n                                                                "    // l:14
"\n                                                                                                 "    // l:15
"uint readIOreg(uint address);\n                                                                    "    // l:16
"\n                                                                                                 "    // l:17
"// algorithm from https://byuu.net/video/color-emulation/\n                                        "    // l:18
"const float lcdGamma = 4.0;\n                                                                      "    // l:19
"const float outGamma = 2.2;\n                                                                      "    // l:20
"const mat3x3 CorrectionMatrix = mat3x3(\n                                                          "    // l:21
"        255.0,  10.0,  50.0,\n                                                                     "    // l:22
"         50.0, 230.0,  10.0,\n                                                                     "    // l:23
"          0.0,  30.0, 220.0\n                                                                      "    // l:24
") / 255.0;\n                                                                                       "    // l:25
"\n                                                                                                 "    // l:26
"vec4 ColorCorrect(vec4 color) {\n                                                                  "    // l:27
"    vec3 lrgb = pow(color.rgb, vec3(lcdGamma));\n                                                  "    // l:28
"    vec3 rgb = pow(CorrectionMatrix * lrgb, vec3(1.0 / outGamma)) * (255.0 / 280.0);\n             "    // l:29
"    return vec4(rgb, color.a);\n                                                                   "    // l:30
"}\n                                                                                                "    // l:31
"\n                                                                                                 "    // l:32
"void CheckBottom(uint layer, uint window) {\n                                                      "    // l:33
"    if (Bottom) {\n                                                                                "    // l:34
"        uint BLDCNT = readIOreg(0x50u);\n                                                          "    // l:35
"        if (((BLDCNT & 0x00c0u) != 0x0040u)) {\n                                                   "    // l:36
"            // not interesting, not normal alpha blending\n                                        "    // l:37
"            discard;\n                                                                             "    // l:38
"        }\n                                                                                        "    // l:39
"\n                                                                                                 "    // l:40
"        if ((window & 0x20u) == 0u) {\n                                                            "    // l:41
"            // blending disabled in window, don't render on bottom frame\n                         "    // l:42
"            discard;\n                                                                             "    // l:43
"        }\n                                                                                        "    // l:44
"    }\n                                                                                            "    // l:45
"}\n                                                                                                "    // l:46
"\n                                                                                                 "    // l:47
"vec4 AlphaCorrect(vec4 color, uint layer, uint window) {\n                                         "    // l:48
"    // BG0-3, 4 for Obj, 5 for BD\n                                                                "    // l:49
"    if ((window & 0x20u) == 0u) {\n                                                                "    // l:50
"        // blending disabled in window\n                                                           "    // l:51
"        return vec4(color.rgb, -1);\n                                                              "    // l:52
"    }\n                                                                                            "    // l:53
"\n                                                                                                 "    // l:54
"    uint BLDCNT = readIOreg(0x50u);\n                                                              "    // l:55
"    uint BLDY = clamp(readIOreg(0x54u) & 0x1fu, 0u, 16u);\n                                        "    // l:56
"\n                                                                                                 "    // l:57
"    switch (BLDCNT & 0x00c0u) {\n                                                                  "    // l:58
"        case 0x0000u:\n                                                                            "    // l:59
"            // blending disabled\n                                                                 "    // l:60
"            return vec4(color.rgb, -1);\n                                                          "    // l:61
"        case 0x0040u:\n                                                                            "    // l:62
"            // normal blending, do this after (most complicated)\n                                 "    // l:63
"            break;\n                                                                               "    // l:64
"        case 0x0080u:\n                                                                            "    // l:65
"        {\n                                                                                        "    // l:66
"            // blend A with white\n                                                                "    // l:67
"            if ((BLDCNT & (1u << layer)) != 0u) {\n                                                "    // l:68
"                // layer is top layer\n                                                            "    // l:69
"                return vec4(mix(color.rgb, vec3(1), float(BLDY) / 16.0), -1.0);\n                  "    // l:70
"            }\n                                                                                    "    // l:71
"            // bottom layer, not blended\n                                                         "    // l:72
"            return vec4(color.rgb, -1);\n                                                          "    // l:73
"        }\n                                                                                        "    // l:74
"        case 0x00c0u:\n                                                                            "    // l:75
"        {\n                                                                                        "    // l:76
"            // blend A with black\n                                                                "    // l:77
"            if ((BLDCNT & (1u << layer)) != 0u) {\n                                                "    // l:78
"                // layer is top layer\n                                                            "    // l:79
"                return vec4(mix(color.rgb, vec3(0), float(BLDY) / 16.0), -1.0);\n                  "    // l:80
"            }\n                                                                                    "    // l:81
"            // bottom layer, not blended\n                                                         "    // l:82
"            return vec4(color.rgb, -1);\n                                                          "    // l:83
"        }\n                                                                                        "    // l:84
"    }\n                                                                                            "    // l:85
"\n                                                                                                 "    // l:86
"    // value was not normal blending / fade\n                                                      "    // l:87
"    uint BLDALPHA = readIOreg(0x52u);\n                                                            "    // l:88
"    uint BLD_EVA = clamp(BLDALPHA & 0x1fu, 0u, 16u);\n                                             "    // l:89
"    uint BLD_EVB = clamp((BLDALPHA >> 8u) & 0x1fu, 0u, 16u);\n                                     "    // l:90
"\n                                                                                                 "    // l:91
"    if ((BLDCNT & (1u << layer)) != 0u) {\n                                                        "    // l:92
"        // top layer\n                                                                             "    // l:93
"        if (!Bottom) {\n                                                                           "    // l:94
"            return vec4(color.rgb, float(BLD_EVA) / 16.0);\n                                       "    // l:95
"        }\n                                                                                        "    // l:96
"        else {\n                                                                                   "    // l:97
"            discard;\n                                                                             "    // l:98
"        }\n                                                                                        "    // l:99
"    }\n                                                                                            "    // l:100
"    // bottom layer\n                                                                              "    // l:101
"    if ((BLDCNT & (0x100u << layer)) != 0u) {\n                                                    "    // l:102
"        // set alpha value to -half of the actual value\n                                          "    // l:103
"        // -1 means: final color\n                                                                 "    // l:104
"        // negative: bottom\n                                                                      "    // l:105
"        // positive: top\n                                                                         "    // l:106
"        return vec4(color.rgb, -0.25 - (float(BLD_EVB) / 32.0));\n                                 "    // l:107
"    }\n                                                                                            "    // l:108
"\n                                                                                                 "    // l:109
"    // neither\n                                                                                   "    // l:110
"    return vec4(color.rgb, -1);\n                                                                  "    // l:111
"}\n                                                                                                "    // l:112
"\n                                                                                                 "    // l:113
"uint readVRAM8(uint address) {\n                                                                   "    // l:114
"    return texelFetch(\n                                                                           "    // l:115
"        VRAM, ivec2(address & 0x7fu, address >> 7u), 0\n                                           "    // l:116
"    ).x;\n                                                                                         "    // l:117
"}\n                                                                                                "    // l:118
"\n                                                                                                 "    // l:119
"uint readVRAM16(uint address) {\n                                                                  "    // l:120
"    address &= ~1u;\n                                                                              "    // l:121
"    uint lsb = readVRAM8(address);\n                                                               "    // l:122
"    return lsb | (readVRAM8(address + 1u) << 8u);\n                                                "    // l:123
"}\n                                                                                                "    // l:124
"\n                                                                                                 "    // l:125
"uint readVRAM32(uint address) {\n                                                                  "    // l:126
"    address &= ~3u;\n                                                                              "    // l:127
"    uint lsh = readVRAM16(address);\n                                                              "    // l:128
"    return lsh | (readVRAM16(address + 2u) << 16u);\n                                              "    // l:129
"}\n                                                                                                "    // l:130
"\n                                                                                                 "    // l:131
"uint readIOreg(uint address) {\n                                                                   "    // l:132
"    return texelFetch(\n                                                                           "    // l:133
"        IO, ivec2(address >> 1u, uint(OnScreenPos.y)), 0\n                                         "    // l:134
"    ).x;\n                                                                                         "    // l:135
"}\n                                                                                                "    // l:136
"\n                                                                                                 "    // l:137
"ivec4 readOAMentry(uint index) {\n                                                                 "    // l:138
"    return texelFetch(\n                                                                           "    // l:139
"        OAM, int(index), 0\n                                                                       "    // l:140
"    );\n                                                                                           "    // l:141
"}\n                                                                                                "    // l:142
"\n                                                                                                 "    // l:143
"vec4 readPALentry(uint index) {\n                                                                  "    // l:144
"    // Conveniently, since PAL stores the converted colors already, getting a color from an index is as simple as this:\n"    // l:145
"    return texelFetch(\n                                                                           "    // l:146
"        PAL, ivec2(index, PALBufferIndex[uint(OnScreenPos.y)]), 0\n                                "    // l:147
"    );\n                                                                                           "    // l:148
"}\n                                                                                                "    // l:149
"\n                                                                                                 "    // l:150
"uint getWindow(uint x, uint y) {\n                                                                 "    // l:151
"    return texelFetch(\n                                                                           "    // l:152
"        Window, ivec2(x, 160u - y), 0\n                                                            "    // l:153
"    ).r;\n                                                                                         "    // l:154
"}\n                                                                                                "    // l:155
"\n                                                                                                 "    // l:156
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


// ObjectFragmentShaderSource (from object_fragment.glsl, lines 5 to 285)
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
"vec4 AlphaCorrect(vec4 color, uint layer, uint window);\n                                          "    // l:23
"\n                                                                                                 "    // l:24
"uint readVRAM8(uint address);\n                                                                    "    // l:25
"uint readVRAM16(uint address);\n                                                                   "    // l:26
"uint readVRAM32(uint address);\n                                                                   "    // l:27
"\n                                                                                                 "    // l:28
"uint readIOreg(uint address);\n                                                                    "    // l:29
"ivec4 readOAMentry(uint index);\n                                                                  "    // l:30
"vec4 readPALentry(uint index);\n                                                                   "    // l:31
"\n                                                                                                 "    // l:32
"uint getWindow(uint x, uint y);\n                                                                  "    // l:33
"\n                                                                                                 "    // l:34
"vec4 RegularObject(bool OAM2DMapping) {\n                                                          "    // l:35
"    uint TID = OBJ.attr2 & 0x03ffu;\n                                                              "    // l:36
"\n                                                                                                 "    // l:37
"    uint dx = uint(InObjPos.x);\n                                                                  "    // l:38
"    uint dy = uint(InObjPos.y);\n                                                                  "    // l:39
"\n                                                                                                 "    // l:40
"    // mosaic effect\n                                                                             "    // l:41
"    if ((OBJ.attr0 & 0x1000u) != 0u) {\n                                                           "    // l:42
"        uint MOSAIC = readIOreg(0x4cu);\n                                                          "    // l:43
"        dx -= dx % (((MOSAIC & 0xf00u) >> 8) + 1u);\n                                              "    // l:44
"        dy -= dy % (((MOSAIC & 0xf000u) >> 12) + 1u);\n                                            "    // l:45
"    }\n                                                                                            "    // l:46
"\n                                                                                                 "    // l:47
"    uint PixelAddress;\n                                                                           "    // l:48
"    if ((OBJ.attr0 & 0x2000u) == 0x0000u) {\n                                                      "    // l:49
"        uint PaletteBank = OBJ.attr2 >> 12;\n                                                      "    // l:50
"        PixelAddress = TID << 5;\n                                                                 "    // l:51
"\n                                                                                                 "    // l:52
"        // get base address for line of tiles (vertically)\n                                       "    // l:53
"        if (OAM2DMapping) {\n                                                                      "    // l:54
"            PixelAddress += ObjWidth * (dy >> 3) << 2;\n                                           "    // l:55
"        }\n                                                                                        "    // l:56
"        else {\n                                                                                   "    // l:57
"            PixelAddress += 32u * 0x20u * (dy >> 3);\n                                             "    // l:58
"        }\n                                                                                        "    // l:59
"        PixelAddress += (dy & 7u) << 2; // offset within tile for sliver\n                         "    // l:60
"\n                                                                                                 "    // l:61
"        // Sprites VRAM base address is 0x10000\n                                                  "    // l:62
"        PixelAddress = (PixelAddress & 0x7fffu) | 0x10000u;\n                                      "    // l:63
"\n                                                                                                 "    // l:64
"        // horizontal offset:\n                                                                    "    // l:65
"        PixelAddress += (dx >> 3) << 5;    // of tile\n                                            "    // l:66
"        PixelAddress += ((dx & 7u) >> 1);  // in tile\n                                            "    // l:67
"\n                                                                                                 "    // l:68
"        uint VRAMEntry = readVRAM8(PixelAddress);\n                                                "    // l:69
"        if ((dx & 1u) != 0u) {\n                                                                   "    // l:70
"            // upper nibble\n                                                                      "    // l:71
"            VRAMEntry >>= 4;\n                                                                     "    // l:72
"        }\n                                                                                        "    // l:73
"        else {\n                                                                                   "    // l:74
"            VRAMEntry &= 0x0fu;\n                                                                  "    // l:75
"        }\n                                                                                        "    // l:76
"\n                                                                                                 "    // l:77
"        if (VRAMEntry != 0u) {\n                                                                   "    // l:78
"            return vec4(readPALentry(0x100u + (PaletteBank << 4) + VRAMEntry).rgb, 1);\n           "    // l:79
"        }\n                                                                                        "    // l:80
"        else {\n                                                                                   "    // l:81
"            // transparent\n                                                                       "    // l:82
"            discard;\n                                                                             "    // l:83
"        }\n                                                                                        "    // l:84
"    }\n                                                                                            "    // l:85
"    else {\n                                                                                       "    // l:86
"        // 8bpp\n                                                                                  "    // l:87
"        PixelAddress = TID << 5;\n                                                                 "    // l:88
"\n                                                                                                 "    // l:89
"        if (OAM2DMapping) {\n                                                                      "    // l:90
"            PixelAddress += ObjWidth * (dy & ~7u);\n                                               "    // l:91
"        }\n                                                                                        "    // l:92
"        else {\n                                                                                   "    // l:93
"            PixelAddress += 32u * 0x20u * (dy >> 3);\n                                             "    // l:94
"        }\n                                                                                        "    // l:95
"        PixelAddress += (dy & 7u) << 3;\n                                                          "    // l:96
"\n                                                                                                 "    // l:97
"        // Sprites VRAM base address is 0x10000\n                                                  "    // l:98
"        PixelAddress = (PixelAddress & 0x7fffu) | 0x10000u;\n                                      "    // l:99
"\n                                                                                                 "    // l:100
"        // horizontal offset:\n                                                                    "    // l:101
"        PixelAddress += (dx >> 3) << 6;\n                                                          "    // l:102
"        PixelAddress += dx & 7u;\n                                                                 "    // l:103
"\n                                                                                                 "    // l:104
"        uint VRAMEntry = readVRAM8(PixelAddress);\n                                                "    // l:105
"\n                                                                                                 "    // l:106
"        if (VRAMEntry != 0u) {\n                                                                   "    // l:107
"            return vec4(readPALentry(0x100u + VRAMEntry).rgb, 1);\n                                "    // l:108
"        }\n                                                                                        "    // l:109
"        else {\n                                                                                   "    // l:110
"            // transparent\n                                                                       "    // l:111
"            discard;\n                                                                             "    // l:112
"        }\n                                                                                        "    // l:113
"    }\n                                                                                            "    // l:114
"}\n                                                                                                "    // l:115
"\n                                                                                                 "    // l:116
"bool InsideBox(vec2 v, vec2 bottomLeft, vec2 topRight) {\n                                         "    // l:117
"    vec2 s = step(bottomLeft, v) - step(topRight, v);\n                                            "    // l:118
"    return (s.x * s.y) != 0.0;\n                                                                   "    // l:119
"}\n                                                                                                "    // l:120
"\n                                                                                                 "    // l:121
"vec4 AffineObject(bool OAM2DMapping) {\n                                                           "    // l:122
"    uint TID = OBJ.attr2 & 0x03ffu;\n                                                              "    // l:123
"\n                                                                                                 "    // l:124
"    uint AffineIndex = (OBJ.attr1 & 0x3e00u) >> 9;\n                                               "    // l:125
"    AffineIndex <<= 2;  // goes in groups of 4\n                                                   "    // l:126
"\n                                                                                                 "    // l:127
"    // scaling parameters\n                                                                        "    // l:128
"    int PA = readOAMentry(AffineIndex).attr3;\n                                                    "    // l:129
"    int PB = readOAMentry(AffineIndex + 1u).attr3;\n                                               "    // l:130
"    int PC = readOAMentry(AffineIndex + 2u).attr3;\n                                               "    // l:131
"    int PD = readOAMentry(AffineIndex + 3u).attr3;\n                                               "    // l:132
"\n                                                                                                 "    // l:133
"    // reference point\n                                                                           "    // l:134
"    vec2 p0 = vec2(\n                                                                              "    // l:135
"        float(ObjWidth  >> 1),\n                                                                   "    // l:136
"        float(ObjHeight >> 1)\n                                                                    "    // l:137
"    );\n                                                                                           "    // l:138
"\n                                                                                                 "    // l:139
"    vec2 p1;\n                                                                                     "    // l:140
"    if ((OBJ.attr0 & 0x0300u) == 0x0300u) {\n                                                      "    // l:141
"        // double rendering\n                                                                      "    // l:142
"        p1 = 2 * p0;\n                                                                             "    // l:143
"    }\n                                                                                            "    // l:144
"    else {\n                                                                                       "    // l:145
"        p1 = p0;\n                                                                                 "    // l:146
"    }\n                                                                                            "    // l:147
"\n                                                                                                 "    // l:148
"    mat2x2 rotscale = mat2x2(\n                                                                    "    // l:149
"        float(PA), float(PC),\n                                                                    "    // l:150
"        float(PB), float(PD)\n                                                                     "    // l:151
"    ) / 256.0;  // fixed point stuff\n                                                             "    // l:152
"\n                                                                                                 "    // l:153
"    ivec2 pos = ivec2(rotscale * (InObjPos - p1) + p0);\n                                          "    // l:154
"    if (!InsideBox(pos, vec2(0, 0), vec2(ObjWidth, ObjHeight))) {\n                                "    // l:155
"        // out of bounds\n                                                                         "    // l:156
"        discard;\n                                                                                 "    // l:157
"    }\n                                                                                            "    // l:158
"\n                                                                                                 "    // l:159
"    // mosaic effect\n                                                                             "    // l:160
"    if ((OBJ.attr0 & 0x1000u) != 0u) {\n                                                           "    // l:161
"        uint MOSAIC = readIOreg(0x4cu);\n                                                          "    // l:162
"        pos.x -= pos.x % int(((MOSAIC & 0xf00u) >> 8) + 1u);\n                                     "    // l:163
"        pos.y -= pos.y % int(((MOSAIC & 0xf000u) >> 12) + 1u);\n                                   "    // l:164
"    }\n                                                                                            "    // l:165
"\n                                                                                                 "    // l:166
"    // get actual pixel\n                                                                          "    // l:167
"    uint PixelAddress = 0x10000u;  // OBJ VRAM starts at 0x10000 in VRAM\n                         "    // l:168
"    PixelAddress += TID << 5;\n                                                                    "    // l:169
"    if (OAM2DMapping) {\n                                                                          "    // l:170
"        PixelAddress += ObjWidth * uint(pos.y & ~7) >> 1;\n                                        "    // l:171
"    }\n                                                                                            "    // l:172
"    else {\n                                                                                       "    // l:173
"        PixelAddress += 32u * 0x20u * uint(pos.y >> 3);\n                                          "    // l:174
"    }\n                                                                                            "    // l:175
"\n                                                                                                 "    // l:176
"    // the rest is very similar to regular sprites:\n                                              "    // l:177
"    if ((OBJ.attr0 & 0x2000u) == 0x0000u) {\n                                                      "    // l:178
"        uint PaletteBank = OBJ.attr2 >> 12;\n                                                      "    // l:179
"        PixelAddress += uint(pos.y & 7) << 2; // offset within tile for sliver\n                   "    // l:180
"\n                                                                                                 "    // l:181
"        // horizontal offset:\n                                                                    "    // l:182
"        PixelAddress += uint(pos.x >> 3) << 5;    // of tile\n                                     "    // l:183
"        PixelAddress += uint(pos.x & 7) >> 1;  // in tile\n                                        "    // l:184
"\n                                                                                                 "    // l:185
"        uint VRAMEntry = readVRAM8(PixelAddress);\n                                                "    // l:186
"        if ((pos.x & 1) != 0) {\n                                                                  "    // l:187
"            // upper nibble\n                                                                      "    // l:188
"            VRAMEntry >>= 4;\n                                                                     "    // l:189
"        }\n                                                                                        "    // l:190
"        else {\n                                                                                   "    // l:191
"            VRAMEntry &= 0x0fu;\n                                                                  "    // l:192
"        }\n                                                                                        "    // l:193
"\n                                                                                                 "    // l:194
"        if (VRAMEntry != 0u) {\n                                                                   "    // l:195
"            return vec4(readPALentry(0x100u + (PaletteBank << 4) + VRAMEntry).rgb, 1);\n           "    // l:196
"        }\n                                                                                        "    // l:197
"        else {\n                                                                                   "    // l:198
"            // transparent\n                                                                       "    // l:199
"            discard;\n                                                                             "    // l:200
"        }\n                                                                                        "    // l:201
"    }\n                                                                                            "    // l:202
"    else {\n                                                                                       "    // l:203
"        PixelAddress += (uint(pos.y) & 7u) << 3; // offset within tile for sliver\n                "    // l:204
"\n                                                                                                 "    // l:205
"        // horizontal offset:\n                                                                    "    // l:206
"        PixelAddress += uint(pos.x >> 3) << 6;  // of tile\n                                       "    // l:207
"        PixelAddress += uint(pos.x & 7);        // in tile\n                                       "    // l:208
"\n                                                                                                 "    // l:209
"        uint VRAMEntry = readVRAM8(PixelAddress);\n                                                "    // l:210
"\n                                                                                                 "    // l:211
"        if (VRAMEntry != 0u) {\n                                                                   "    // l:212
"            return vec4(readPALentry(0x100u + VRAMEntry).rgb, 1);\n                                "    // l:213
"        }\n                                                                                        "    // l:214
"        else {\n                                                                                   "    // l:215
"            // transparent\n                                                                       "    // l:216
"            discard;\n                                                                             "    // l:217
"        }\n                                                                                        "    // l:218
"    }\n                                                                                            "    // l:219
"}\n                                                                                                "    // l:220
"\n                                                                                                 "    // l:221
"void main() {\n                                                                                    "    // l:222
"    if (OnScreenPos.x < 0) {\n                                                                     "    // l:223
"        discard;\n                                                                                 "    // l:224
"    }\n                                                                                            "    // l:225
"    if (OnScreenPos.x > 240u) {\n                                                                  "    // l:226
"        discard;\n                                                                                 "    // l:227
"    }\n                                                                                            "    // l:228
"\n                                                                                                 "    // l:229
"    if (OnScreenPos.y < float(YClipStart)) {\n                                                     "    // l:230
"        discard;\n                                                                                 "    // l:231
"    }\n                                                                                            "    // l:232
"    if (OnScreenPos.y > float(YClipEnd)) {\n                                                       "    // l:233
"        discard;\n                                                                                 "    // l:234
"    }\n                                                                                            "    // l:235
"\n                                                                                                 "    // l:236
"    uint DISPCNT = readIOreg(0x00u);\n                                                             "    // l:237
"#ifndef OBJ_WINDOW\n                                                                               "    // l:238
"    if ((DISPCNT & 0x1000u) == 0u) {\n                                                             "    // l:239
"        // objects disabled in this scanline\n                                                     "    // l:240
"        discard;\n                                                                                 "    // l:241
"    }\n                                                                                            "    // l:242
"    uint window = getWindow(uint(OnScreenPos.x), uint(OnScreenPos.y));\n                           "    // l:243
"    if ((window & 0x10u) == 0u) {\n                                                                "    // l:244
"        // disabled by window\n                                                                    "    // l:245
"        discard;\n                                                                                 "    // l:246
"    }\n                                                                                            "    // l:247
"#else\n                                                                                            "    // l:248
"    if ((DISPCNT & 0x8000u) == 0u) {\n                                                             "    // l:249
"        // object window disabled in this scanline\n                                               "    // l:250
"        discard;\n                                                                                 "    // l:251
"    }\n                                                                                            "    // l:252
"#endif\n                                                                                           "    // l:253
"\n                                                                                                 "    // l:254
"    bool OAM2DMapping = (DISPCNT & (0x0040u)) != 0u;\n                                             "    // l:255
"\n                                                                                                 "    // l:256
"    vec4 Color;\n                                                                                  "    // l:257
"    if (!Affine) {\n                                                                               "    // l:258
"        Color = RegularObject(OAM2DMapping);\n                                                     "    // l:259
"    }\n                                                                                            "    // l:260
"    else{\n                                                                                        "    // l:261
"        Color = AffineObject(OAM2DMapping);\n                                                      "    // l:262
"    }\n                                                                                            "    // l:263
"\n                                                                                                 "    // l:264
"#ifndef OBJ_WINDOW\n                                                                               "    // l:265
"    FragColor = ColorCorrect(Color);\n                                                             "    // l:266
"    if ((OBJ.attr0 & 0x0c00u) == 0x0400u) {\n                                                      "    // l:267
"        FragColor = AlphaCorrect(FragColor, 4u, window);\n                                         "    // l:268
"    }\n                                                                                            "    // l:269
"    else {\n                                                                                       "    // l:270
"        FragColor = vec4(FragColor.rgb, -1);\n                                                     "    // l:271
"    }\n                                                                                            "    // l:272
"#else\n                                                                                            "    // l:273
"    // RegularObject/AffineObject will only return if it is nontransparent\n                       "    // l:274
"    uint WINOBJ = (readIOreg(0x4au) >> 8) & 0x3fu;\n                                               "    // l:275
"\n                                                                                                 "    // l:276
"    FragColor.r = WINOBJ;\n                                                                        "    // l:277
"#endif\n                                                                                           "    // l:278
"}\n                                                                                                "    // l:279
"\n                                                                                                 "    // l:280
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