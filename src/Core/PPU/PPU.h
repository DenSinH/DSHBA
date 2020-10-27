#pragma once

#include "../Mem/Mem.h"
#include "../Scheduler/scheduler.h"

#include "shaders/GX_constants.h"

#include "default.h"

#include <mutex>

class GBAPPU {

public:
    u32 Frame = 0;

    explicit GBAPPU(s_scheduler* scheduler, Mem *memory);
    ~GBAPPU() { };

    void VideoInit();
    struct s_framebuffer Render();

private:
    friend class Initializer;

    Mem *Memory;

    u32 BufferScanlineCount = 0;
    u32 BufferFrame = 0;

    PALMEM PALBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};
    VRAMMEM VRAMBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};
    s_UpdateRange VRAMRanges[2][VISIBLE_SCREEN_HEIGHT] = {};
    VRAMMEM OAMBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};

    u32 ScanlineBatchSizes[2][VISIBLE_SCREEN_HEIGHT] = {};
    u32 CurrentScanlineBatch = 0;

    LCDIO LCDIOBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};

    s_scheduler* Scheduler;
    s_event BufferScanline;

    std::mutex DrawMutex = std::mutex();

    // programs for each draw program
    unsigned int Program;
    unsigned int Framebuffer;

    unsigned int IOTexture, IOLocation;
    unsigned int OAMTexture, OAMLocation;
    unsigned int PALTexture, PALLocation;

    // unsigned int PALBinding, OAMBinding, IOBinding;
    unsigned int VRAMSSBO;
    unsigned int VAO;
    unsigned int VBO;  // for drawing a line

    static SCHEDULER_EVENT(BufferScanlineEvent);

    void InitFramebuffers();
    void InitPrograms();
    void InitBuffers();
    void DrawScanlines(u32 scanline, u32 amount);
};