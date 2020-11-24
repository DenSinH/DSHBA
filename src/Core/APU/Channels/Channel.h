#pragma once

#include "../../Scheduler/scheduler.h"

#include "default.h"
#include "helpers.h"
#include "const.h"

#include <cmath>

class Channel {
public:
    explicit Channel(s_scheduler* scheduler) {
        Tick = (s_event) {
            .callback = TickEvent,
            .caller = this,
        };

        // we don't need to store a reference to the scheduler in this class,
        // all events are recursive and nonstop anyway
        scheduler->AddEvent(&Tick);
        Scheduler = scheduler;
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

        UpdateEvent();
    }

    void UpdateEvent() {
        if (SoundOn()) {
            if (Tick.active) {
                // already was active
                i32 diff = (Tick.time - TriggerTime + Period) & s_scheduler::TimeMask;
                if (std::abs(diff) > 0x100) {
                    // only reschedule if time has actually changed
                    // we keep a bit of a resolution cause a 0x100 tick difference we can probably barely hear anyway
                    // and rescheduling events is expensive
                    Scheduler->RescheduleEvent(&Tick, Tick.time - diff);
                }
            }
            else {
                // event was activated
                TriggerTime = *Scheduler->timer;
                Scheduler->AddEventAfter(&Tick, Period);
            }
        }
        else if (Tick.active) {
            Scheduler->RemoveEvent(&Tick);
        }
    }

    ALWAYS_INLINE void SetPeriod(u32 new_val) {
        // make sure that this is called before UpdateEvent
        // explanation is below
        TruePeriod = Period = new_val | 1;  // make sure it's never 0
        BatchSize = 1;
        while (unlikely(Period < 512)) {
            Period <<= 1;
            BatchSize <<= 1;
        }
    }

protected:
    // channels with a period lower/higher than these bounds will not be marked as enabled
    static const u32 UpperPeriodBound = CLOCK_FREQUENCY / 20;
    static const u32 LowerPeriodBound = CLOCK_FREQUENCY / 22000;

    i32 LengthCounter = 0;
    u32 TruePeriod    = 128 * 2048;  // wanted for sweep channels
    u32 Period        = 128 * 2048;  // square channel default period, just some arbitrary value
    u32 Volume        = 0;

    bool LengthFlag    = false;
    bool Enabled       = false;

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

    /* batch up tick events
     * the GBA's CPU clock runs at 16853760Hz
     * the sample rate is ~33kHz
     * that makes for ~512 cycles per sample
     * if the period is less than 512 cycles, we want to batch up a few ticks so that we don't have to
     * do the channel event more times than necessary, when we don't hear it anyway
     * */
    u32 BatchSize = 1;
    s_event Tick;
    s_scheduler* Scheduler;
    i32 TriggerTime;

    static SCHEDULER_EVENT(TickEvent) {
        auto chan = (Channel*)caller;

        for (u32 i = 0; i < chan->BatchSize; i++) {
            chan->OnTick();
        }

        if (chan->SoundOn()) {
            chan->CurrentSample = chan->GetSample();
        }

        chan->TriggerTime = event->time;
        event->time += chan->Period;
        scheduler->AddEvent(event);
    }
};