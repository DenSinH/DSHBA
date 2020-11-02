#pragma once

#include "../Mem/MemoryHelpers.h"
#include "../Scheduler/scheduler.h"

#include "shaders/GX_constants.h"

#include "default.h"

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
    VRAMMEM OAMBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};

    u32 ScanlineBatchSizes[2][VISIBLE_SCREEN_HEIGHT] = {};
    u32 CurrentScanlineBatch = 0;

    LCDIO LCDIOBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};

    s_scheduler* Scheduler;

    std::mutex DrawMutex = std::mutex();

    // programs for each draw program
    GLuint BGProgram;
    GLuint Framebuffer;

    GLuint IOTexture, BGIOLocation;
    GLuint OAMTexture, BGOAMLocation;
    GLuint PALTexture, BGPALLocation;
    GLuint VRAMSSBO;

    GLuint BGVAO;
    GLuint BGVBO;  // for drawing a batch of scanlines

    static const constexpr auto ObjIndices = [] {
        std::array<u16, ((128 * 5) >> 2)> table = {0};

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

    // buffer attribute 0 and 1 (positions) to send to the vertex shader
    u32 NumberOfObjVerts = 0;
    u32 ObjAttr01Buffer[sizeof(OAMMEM)];
    GLuint ObjProgram;
    GLuint ObjIOLocation;
    GLuint ObjOAMLocation;
    GLuint ObjPALLocation;

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
    void BufferObjects(u32 buffer);

    void DrawScanlines(u32 scanline, u32 amount);
    void DrawObjects();
};


template<bool ObjWindow>
void GBAPPU::BufferObjects(u32 buffer) {
    NumberOfObjVerts = 0;
    for (int i = 0; i < (sizeof(OAMMEM) >> 3); i++) {
        // search halfway through the frame
        u32 Attr01 = *(u32*)&OAMBuffer[buffer][VISIBLE_SCREEN_HEIGHT >> 1][i << 3];

        if ((Attr01 & 0x0300) == 0x0200) {
            // rendering disabled
            continue;
        }

        if constexpr(!ObjWindow) {
            if ((Attr01 & 0x0c00) == 0x0800) {
                // part of object window, dont buffer now
                continue;
            }
        }
        else {
            // todo
            break;
        }

        ObjAttr01Buffer[NumberOfObjVerts++] = Attr01;
        ObjAttr01Buffer[NumberOfObjVerts++] = Attr01;
        ObjAttr01Buffer[NumberOfObjVerts++] = Attr01;
        ObjAttr01Buffer[NumberOfObjVerts++] = Attr01;
    }
}