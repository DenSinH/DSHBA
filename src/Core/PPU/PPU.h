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
    s_UpdateRange PALRanges[2][VISIBLE_SCREEN_HEIGHT] = {};
    VRAMMEM VRAMBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};
    s_UpdateRange VRAMRanges[2][VISIBLE_SCREEN_HEIGHT] = {};
    VRAMMEM OAMBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};
    s_UpdateRange OAMRanges[2][VISIBLE_SCREEN_HEIGHT] = {};

    LCDIO LCDIOBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};

    s_scheduler* Scheduler;
    s_event BufferScanline;

    std::mutex DrawMutex = std::mutex();

    // programs for each draw program
    unsigned int Program;
    unsigned int Framebuffer;

    unsigned int PALSSBO, OAMSSBO;
    unsigned int IOLocation;

    // unsigned int PALBinding, OAMBinding, IOBinding;
    unsigned int VRAMSSBO;
    unsigned int VAO;
    unsigned int VBO;  // for drawing a line

    static SCHEDULER_EVENT(BufferScanlineEvent);

    void InitFramebuffers();
    void InitPrograms();
    void InitBuffers();
    void DrawScanLine(u32 scanline);
};