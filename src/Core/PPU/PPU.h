#pragma once

#include "../Mem/Mem.h"
#include "../Scheduler/scheduler.h"

#include "shaders/GX_constants.h"

#include "default.h"

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

    s_VMEM VMEMBuffer[2][VISIBLE_SCREEN_HEIGHT] = {};

    s_scheduler* Scheduler;
    s_event BufferScanline;

    // programs for each draw program
    unsigned int Program;
    unsigned int Framebuffer;

    unsigned int PALUBO, OAMUBO, IOUBO;
    // unsigned int PALBinding, OAMBinding, IOBinding;
    unsigned int VRAMSSBO;
    unsigned int VAO;
    unsigned int VBO;  // for drawing a line

    static SCHEDULER_EVENT(BufferScanlineEvent);

    void InitFramebuffers();
    void InitPrograms();
    void InitBuffers();
    void DrawScanLine(s_VMEM* VMEM, u32 scanline) const;
};