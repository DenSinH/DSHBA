
static constexpr u8 PrescalerSelection[4] {
    0, 6, 8, 10
};

template<u8 x> void MMIO::OverflowTimer() {
    log_tmr("Overflow timer %d", x);
    ASSUME(x < 4);

    Timer[x].Counter = Timer[x].Register.CNT_L;  // reload counter
    Timer[x].TriggerTime = *Scheduler->timer;    // reset triggertime

    if constexpr(x < 3) {
        // tick next timer if in count-up mode
        if (Timer[x + 1].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::CountUp)) {
            Timer[x + 1].Counter++;
            if (unlikely(Timer[x + 1].Counter) > 0x10000) {
                // overflow next timer
                OverflowTimer<x + 1>();
            }
        }
    }

    // trigger interrupt
    if (Timer->Register.CNT_H & static_cast<u16>(TMCNT_HFlags::IRQ)) {
        TriggerInterrupt(static_cast<u16>(Interrupt::Timer0) << x);
    }

    // reschedule event, timer 0 is never in count-up mode
    if (x == 0 || !(Timer[x].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::CountUp))) {
        if (likely(!Timer[x].Overflow.active)) {
            Timer[x].Overflow.time += (0x10000 - Timer[x].Register.CNT_L) << Timer[x].PrescalerShift;
            add_event(Scheduler, &Timer[x].Overflow);
        }
        else {
            reschedule_event(Scheduler, &Timer[x].Overflow,
                             Timer[x].Overflow.time + ((0x10000 - Timer[x].Register.CNT_L) << Timer[x].PrescalerShift));
        }
    }
}

template<u8 x> SCHEDULER_EVENT(MMIO::TimerOverflowEvent) {
    auto IO = (MMIO*)caller;

    IO->OverflowTimer<x>();
}

template<u8 x> WRITE_CALLBACK(MMIO::WriteTMxCNT_L) {
    log_tmr("Write TM%dCNT_L: %x", x, value);
    Timer[x].Register.CNT_L = value;

    if (!(Timer->Register.CNT_H & static_cast<u16>(TMCNT_HFlags::Enabled))) {
        // frozen timer
        Timer[x].Counter = value;
    }
}

template<u8 x> READ_PRECALL(MMIO::ReadTMxCNT_L) {
    if constexpr (x == 0) {
        // always direct
        Timer[x].FlushDirect(*Scheduler->timer);
    }
    else {
        // maybe count-up
        if (!(Timer[x].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::CountUp))) {
            Timer[x].FlushDirect(*Scheduler->timer);
        }
    }
    return (u16)Timer[x].Counter;
}

template<u8 x> WRITE_CALLBACK(MMIO::WriteTMxCNT_H) {
    log_tmr("Write TM%dCNT_H: %x", x, value);
    /*
     * On writes to TMCNT_H we have to "flush" a timer to it's current value
     * */

    const bool was_enabled = (Timer[x].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::Enabled)) != 0;

    if (was_enabled) {
        // flush timer
        if constexpr (x == 0) {
            // always direct
            Timer[x].FlushDirect(*Scheduler->timer);
        }
        else {
            // maybe count-up
            if (!(Timer[x].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::CountUp))) {
                Timer[x].FlushDirect(*Scheduler->timer);
            }
        }
    }
    else if (value & static_cast<u16>(TMCNT_HFlags::Enabled)) {
        // timer got enabled
        // trigger timing still needs to be set
        Timer[x].Counter = Timer[x].Register.CNT_L;
        Timer[x].TriggerTime = *Scheduler->timer;
    }
    else {
        // nothing interesting happens
        return;
    }

    // update register data
    Timer[x].Register.CNT_H = value;
    Timer[x].PrescalerShift = PrescalerSelection[value & static_cast<u16>(TMCNT_HFlags::Prescaler)];

    // schedule overflow event
    if (x == 0 || !(Timer[x].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::CountUp))) {
        // for direct timers: schedule/reschedule event
        if (unlikely(Timer[x].Overflow.active)) {
            remove_event(Scheduler, &Timer[x].Overflow);
        }

        // delta time
        u64 new_time = (0x10000 - Timer[x].Counter) << Timer[x].PrescalerShift;
        // absolute time
        new_time = (*Scheduler->timer) + (*Scheduler->timer - Timer[x].TriggerTime) + new_time;
        Timer[x].Overflow.time = new_time;

        add_event(Scheduler, &Timer[x].Overflow);
    }
}
