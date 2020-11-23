#pragma once

#include "Channel.h"

#include <algorithm>

class EnvelopeChannel : public Channel {

    /*
     * PSG channel with volume envelope effect
     * */

public:
    explicit EnvelopeChannel(s_scheduler* scheduler) : Channel(scheduler) {}

    i32 EnvelopeTime = 0;
    bool EnvelopeUp = false;

    void DoEnvelope() {
        if (EnvelopeTime > 0) {
            EnvelopeTime--;
            Volume += EnvelopeUp ? 1 : -1;

            Volume = std::clamp(Volume, 0u, 16u);
        }
    }
};