#pragma once

#include "../Mem/MemoryHelpers.h"
#include "../Scheduler/scheduler.h"

#include "shaders/GX_constants.h"

#include "default.h"
#include "helpers.h"

#include <glad/glad.h>

#include <mutex>
#include <array>

class Mem;

class GBAPPU {

public:
    u32 Frame = 0;

    explicit GBAPPU(s_scheduler* scheduler, Mem* memory);
    ~GBAPPU() { };

    void VideoInit();
    struct s_framebuffer Render();

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
    GLuint BGProgram;
    GLuint Framebuffer;

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
    u32 NumberOfObjVerts = 0;
    u64 ObjAttr01Buffer[sizeof(OAMMEM)];
    GLuint ObjProgram;
    GLuint ObjIOLocation;
    GLuint ObjPALLocation;
    GLuint YClipStartLocation, YClipEndLocation;

    GLuint ObjVAO;
    GLuint ObjVBO;
    GLuint ObjEBO;

    void BufferScanline(u32 scanline);

    void InitFramebuffers();
    void InitBGProgram();
    void InitObjProgram();
    void InitBGBuffers();
    void InitObjBuffers();

    template<bool ObjWindow>
    void BufferObjects(u32 buffer, i32 scanline, i32 batch_size);

    void DrawScanlines(u32 scanline, u32 amount);
    void DrawObjects(u32 scanline, u32 amount);
};


template<bool ObjWindow>
void GBAPPU::BufferObjects(u32 buffer, i32 scanline, i32 batch_size) {
    NumberOfObjVerts = 0;
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
            // todo
            break;
        }

        y = OAMBuffer[buffer][scanline][i];
        if (y > VISIBLE_SCREEN_HEIGHT) {
            y -= 0x100;
        }

        height = ObjHeight[OAMBuffer[buffer][scanline][i + 1] >> 6][OAMBuffer[buffer][scanline][i + 3] >> 6];
        if (((y + height) < scanline) || (y >= (scanline + batch_size))) {
            // not in frame
            continue;
        }

        u64 Attr01 = *(u64*)&OAMBuffer[buffer][scanline][i];
        ObjAttr01Buffer[NumberOfObjVerts++] = Attr01;
        ObjAttr01Buffer[NumberOfObjVerts++] = Attr01;
        ObjAttr01Buffer[NumberOfObjVerts++] = Attr01;
        ObjAttr01Buffer[NumberOfObjVerts++] = Attr01;
    }
}