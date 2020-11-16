#pragma once

#include "Channel.h"

class Square : public Channel {

public:
    explicit Square(s_scheduler* scheduler) : Channel(scheduler) {
        Period = 128 * 100;
    }

private:
    static constexpr const u8 DutyCycles[4]  = {
            0x80,  // 12.5%
            0xc0,  // 25%
            0xf0,  // 50%
            0xfc,  // 75%
    };

    u8 Index = 0;
    u8 Duty  = DutyCycles[0];

protected:

    i16 GetSample() override {
        if (Duty & (1 << Index)) {
            return 0x7fff;
        }
        else {
            return -0x7fff;
        }
    }

    void OnTick() override {
        Index = (Index + 1) & 7;
    }
};