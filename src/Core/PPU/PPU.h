#pragma once

#include "../Mem/Mem.h"
#include "../Scheduler/scheduler.h"

#include "default.h"

#include <thread>
#include <mutex>

class GBAPPU {

public:
    explicit GBAPPU(s_scheduler* scheduler, Mem *memory);
    ~GBAPPU() {
        ReleaseAll();
    };

    void VideoInit();
    struct s_framebuffer Render(u32 timeout);
    void ReleaseAll() {
        // We want to release all wait events
        // only one we are waiting for now is the VMEMBufferAvailable
        VMEMBufferAvailable[BufferBufferIndex] = true;
        NewDrawAvailable.notify_all();
        NewBufferAvailable.notify_all();
    }

private:
    friend class Initializer;

    Mem *Memory;

    u32 BufferScanlineCount = 0, DrawScanlineCount = 0;

    s_VMEM VMEMBuffer[MAX_BUFFERED_SCANLINES] = {};
    volatile bool VMEMBufferAvailable[MAX_BUFFERED_SCANLINES] = {};
    u32 DrawBufferIndex = 0, BufferBufferIndex = 0;
    std::mutex AvailabilityMutex;
    std::condition_variable NewBufferAvailable, NewDrawAvailable;

    void WaitForBufferAvailable() {
        std::unique_lock<std::mutex> lock(AvailabilityMutex);
        NewBufferAvailable.wait(lock, [&]{
            return VMEMBufferAvailable[BufferBufferIndex];
        });
    }

    bool WaitForDrawCommand(int ms) {
        // returns true on successful wait (timeout not expired)
        std::unique_lock<std::mutex> lock(AvailabilityMutex);
        return NewDrawAvailable.wait_for(
                lock,
                std::chrono::milliseconds(ms),
                [&]{
                    bool buffer_available = !VMEMBufferAvailable[DrawBufferIndex];
                    return buffer_available;
                }
        );
    }

    s_scheduler* Scheduler;
    s_event BufferScanline;

    u32 CurrentFramebuffer = 0;

    // programs for each draw program
    unsigned int Program;
    unsigned int Framebuffer[2];

    unsigned int PALUBO, OAMUBO, IOUBO;
    unsigned int PALBinding, OAMBinding, IOBinding;
    unsigned int VRAMSSBO;
    unsigned int VAO;
    unsigned int VBO;  // for drawing a line

    static SCHEDULER_EVENT(BufferScanlineEvent);

    void InitFramebuffers();
    void InitPrograms();
    void InitBuffers();
    void FrameSwap();
    void DrawScanLine(s_VMEM* VMEM);
};