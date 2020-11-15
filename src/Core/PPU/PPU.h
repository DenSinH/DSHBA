#pragma once

#include "../Mem/MemoryHelpers.h"
#include "../Scheduler/scheduler.h"

#include "shaders/GX_constants.h"

#include "default.h"
#include "helpers.h"

#include <glad/glad.h>

#include <mutex>
#include <array>
#include <condition_variable>

class Mem;

class GBAPPU {

public:
    u32 Frame = 0;

    explicit GBAPPU(s_scheduler* scheduler, Mem* memory);
    ~GBAPPU() { };

    void VideoInit();
    void VideoDestroy();
    struct s_framebuffer Render();
    struct s_framebuffer RenderUntil(size_t ticks);

private:
    friend class MMIO;  // IO needs full control over the PPU
    friend class Initializer;

    Mem *Memory;
    u32 BufferFrame = 0;

    PALMEM PALBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};
    VRAMMEM VRAMBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};
    s_UpdateRange VRAMRanges[2][VISIBLE_SCREEN_HEIGHT] = {};
    OAMMEM OAMBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};

    u32 ReferenceLine2Buffer[2][VISIBLE_SCREEN_HEIGHT] = {};
    u32 ReferenceLine3Buffer[2][VISIBLE_SCREEN_HEIGHT] = {};

    u32 ScanlineVRAMBatchSizes[2][VISIBLE_SCREEN_HEIGHT] = {};
    u32 CurrentVRAMScanlineBatch = 0;
    u32 ScanlineOAMBatchSizes[2][VISIBLE_SCREEN_HEIGHT] = {};
    u32 CurrentOAMScanlineBatch = 0;

    LCDIO LCDIOBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};

    s_scheduler* Scheduler;

    std::mutex DrawMutex = std::mutex();

    // programs for each draw program
    GLuint WinBGProgram, WinObjProgram;
    GLuint WinBGVAO, WinObjVAO;
    GLuint WinFramebuffer;
    GLuint WinTexture, WinDepthBuffer;

    GLuint WinBGIOLocation, WinObjIOLocation;
    GLuint WinObjOAMLocation;

    GLuint WinObjYClipStartLocation, WinObjYClipEndLocation;

    GLuint BGProgram;
    GLuint Framebuffer;

    GLuint BGLocation;
    GLuint BGWindowLocation;
    GLuint IOTexture, BGIOLocation;
    GLuint ReferenceLine2Location, ReferenceLine3Location;
    GLuint PALTexture, BGPALLocation;
    GLuint VRAMSSBO;

    GLuint BGVAO;
    GLuint BGVBO;  // for drawing a batch of scanlines

    static const constexpr auto ObjIndices = [] {
        std::array<u16, ((sizeof(OAMMEM) * 5) >> 2)> table = {0};

        u16 index = 0;
        for (int i = 0; i < table.size(); i += 5) {
            table[i]     = index++;
            table[i + 1] = index++;
            table[i + 2] = index++;
            table[i + 3] = index++;
            table[i + 4] = 0xffff;  // restart index
        }

        return table;
    }();

    static const constexpr i32 ObjHeight[3][4] = {
            {8,  16, 32, 64},
            {8,   8, 16, 32},
            {16, 32, 32, 64},
    };

    // buffer attribute 0 and 1 (positions) to send to the vertex shader
    u64 ObjAttrBuffer[sizeof(OAMMEM)];
    GLuint ObjProgram;
    GLuint ObjIOLocation;
    GLuint ObjPALLocation;
    GLuint ObjWindowLocation;
    GLuint OAMTexture, ObjOAMLocation;
    GLuint ObjYClipStartLocation, ObjYClipEndLocation;

    GLuint ObjVAO;
    GLuint ObjVBO;
    GLuint ObjEBO;

    bool SyncToVideo = true;
    bool FrameSkip = false;

    /* utils for non-synced non-frameskipped video */
    bool FrameReady = false;
    std::mutex FrameWaitMutex = std::mutex();
    std::condition_variable FrameReadyVariable;

    /* utils for non-frameskipped video */
    bool FrameDrawn = true;  // this is important to be true initially
    std::condition_variable FrameDrawnVariable;

    void BufferScanline(u32 scanline);

    void InitFramebuffers();
    void InitBGProgram();
    void InitObjProgram();
    void InitWinBGProgram();
    void InitWinObjProgram();
    void InitBGBuffers();
    void InitObjBuffers();
    void InitWinBGBuffers();
    void InitWinObjBuffers();

    template<bool ObjWindow>
    u32 BufferObjects(u32 buffer, i32 scanline, i32 batch_size);

    void DrawWindows();
    void DrawBGWindow(int win_start, int win_end) const;
    void DrawObjWindow(int win_start, int win_end);
    void DrawScanlines(u32 scanline, u32 amount);
    void DrawObjects(u32 scanline, u32 amount);
};


template<bool ObjWindow>
u32 GBAPPU::BufferObjects(u32 buffer, i32 scanline, i32 batch_size) {
    u32 NumberOfObjVerts = 0;
    i32 y;
    u32 height;
    // buffer in reverse for object priority
    for (int i = sizeof(OAMMEM) - sizeof(u64); i >= 0; i -= sizeof(u64)) {
        if ((OAMBuffer[buffer][scanline][i + 1] & 0x03) == 0x02) {
            // rendering disabled
            continue;
        }

        if constexpr(!ObjWindow) {
            if ((OAMBuffer[buffer][scanline][i + 1] & 0x0c) == 0x08) {
                // part of object window, dont buffer now
                continue;
            }
        }
        else {
            if ((OAMBuffer[buffer][scanline][i + 1] & 0x0c) != 0x08) {
                // not part of object window, dont buffer now
                continue;
            }
        }

        y = OAMBuffer[buffer][scanline][i];
        if (y > VISIBLE_SCREEN_HEIGHT) {
            y -= 0x100;
        }

        height = ObjHeight[OAMBuffer[buffer][scanline][i + 1] >> 6][OAMBuffer[buffer][scanline][i + 3] >> 6];
        if ((OAMBuffer[buffer][scanline][i + 1] & 0x3) == 0x3) {
            // double rendering
            height <<= 1;
        }

        if (((y + height) < scanline) || (y >= (scanline + batch_size))) {
            // not in frame
            continue;
        }

        u64 Attrs = *(u64*)&OAMBuffer[buffer][scanline][i];
        ObjAttrBuffer[NumberOfObjVerts++] = Attrs;
        ObjAttrBuffer[NumberOfObjVerts++] = Attrs;
        ObjAttrBuffer[NumberOfObjVerts++] = Attrs;
        ObjAttrBuffer[NumberOfObjVerts++] = Attrs;
    }
    return NumberOfObjVerts;
}