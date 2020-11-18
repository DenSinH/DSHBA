#pragma once

#include "default.h"

#include <queue>

class MMIO;

class FIFOChannel {

public:
    explicit FIFOChannel(std::function<void(u32)> fifo_callback, u32 fifo_address) {
        FIFOCallback = fifo_callback;
        FIFOAddress = fifo_address;
    }

    i16 CurrentSample;

    ALWAYS_INLINE void Enqueue(i8 data) {
        Queue.push(data);
    }

    void TimerOverflow() {
        if (Queue.size()) {
            CurrentSample = ((i16)Queue.front()) << 8 | ((u8)Queue.front() << 1);
            Queue.pop();
        }

        if (Queue.size() < 16) {
            FIFOCallback(FIFOAddress);
        }
    }

private:
    friend class MMIO;

    std::queue<i8> Queue = {};

    std::function<void(u32)> FIFOCallback;
    u32 FIFOAddress;
};