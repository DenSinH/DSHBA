#pragma once

#include "../../Scheduler/scheduler.h"

#include "default.h"
#include "helpers.h"
#include "const.h"

class Channel {
public:
    explicit Channel(s_scheduler* scheduler) {
        Tick = (s_event) {
            .callback = TickEvent,
            .caller = this,
            .time = Period
        };

        // we don't need to store a reference to the scheduler in this class,
        // all events are recursive and nonstop anyway
        scheduler->AddEvent(&Tick);
    }

    i16 CurrentSample = 0;

    void TickLengthCounter() {
        if (LengthCounter > 0) {
            LengthCounter--;
        }
    }

    virtual void Trigger() {
        Enabled = true;
        if (LengthCounter == 0) {
            LengthCounter = 64;
        }
    }

protected:
    // channels with a period lower/higher than these bounds will not be marked as enabled
    static const u32 UpperPeriodBound = CLOCK_FREQUENCY / 20;
    static const u32 LowerPeriodBound = CLOCK_FREQUENCY / 22000;

    u32  Period        = 128 * 2048;  // square channel default period, just some arbitrary value
    u32  Volume        = 0;

    virtual void OnTick() {};
    virtual i16 GetSample() { return 0; };

    [[nodiscard]] virtual ALWAYS_INLINE bool SoundOn() const {
        if (!Enabled) {
            return false;
        }

        if (LengthFlag && (LengthCounter == 0)) {
            return false;
        }

        if (!Volume) {
            return false;
        }

        return true;
    }

private:
    friend class Initializer;
    friend class MMIO;

    i32  LengthCounter = 0;
    bool LengthFlag    = false;
    bool Enabled       = false;

    s_event Tick;
    static SCHEDULER_EVENT(TickEvent) {
        auto chan = (Channel*)caller;

        chan->OnTick();

        if (chan->SoundOn()) {
            chan->CurrentSample = chan->GetSample();
        }

        event->time += chan->Period;
        scheduler->AddEvent(event);
    }
};