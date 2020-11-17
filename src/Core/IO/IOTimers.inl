
static constexpr u8 PrescalerSelection[4] {
    0, 6, 8, 10
};

template<u8 x>
void MMIO::OverflowTimer() {
    log_tmr("Overflow timer %d", x);
    ASSUME(x < 4);

    Timers[x].Counter = Timers[x].Register.CNT_L;  // reload counter
    Timers[x].TriggerTime = *Scheduler->timer;  // reset triggertime

    if constexpr(x < 2) {
        // trigger bound FIFO channels
        if (Timers[x].FIFOA) {
            Timers[x].FIFOA->TimerOverflow();
        }

        if (Timers[x].FIFOB) {
            Timers[x].FIFOB->TimerOverflow();
        }
    }

    if constexpr(x < 3) {
        // tick next timer if in count-up mode
        if (Timers[x + 1].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::CountUp)) {
            Timers[x + 1].Counter++;
            if (unlikely(Timers[x + 1].Counter) > 0x10000) {
                // overflow next timer
                OverflowTimer<x + 1>();
            }
        }
    }

    // trigger interrupt
    if (Timers[x].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::IRQ)) {
        log_tmr("Doing timer IRQ for timer %x", x);
        TriggerInterrupt(static_cast<u16>(Interrupt::Timer0) << x);
    }

    // reschedule event, timer 0 is never in count-up mode
    if (x == 0 || !(Timers[x].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::CountUp))) {
        if (likely(!Timers[x].Overflow.active)) {
            Timers[x].Overflow.time += (0x10000 - Timers[x].Register.CNT_L) << Timers[x].PrescalerShift;
            Scheduler->AddEvent(&Timers[x].Overflow);
        }
        else {
            log_warn("Timer event in scheduler on overflow");
            Scheduler->RescheduleEvent(&Timers[x].Overflow,
                             Timers[x].Overflow.time + ((0x10000 - Timers[x].Register.CNT_L) << Timers[x].PrescalerShift));
        }
    }
}

template<u8 x>
SCHEDULER_EVENT(MMIO::TimerOverflowEvent) {
    auto IO = (MMIO*)caller;

    IO->OverflowTimer<x>();
}

template<u8 x>
WRITE_CALLBACK(MMIO::WriteTMxCNT_L) {
    log_tmr("Write TM%dCNT_L: %x", x, value);
    Timers[x].Register.CNT_L = value;

    if (!(Timers->Register.CNT_H & static_cast<u16>(TMCNT_HFlags::Enabled))) {
        // frozen timer
        Timers[x].Counter = value;
    }
}

template<u8 x>
READ_PRECALL(MMIO::ReadTMxCNT_L) {
    if constexpr (x == 0) {
        // always direct
        Timers[x].FlushDirect(*Scheduler->timer);
    }
    else {
        // maybe count-up
        if (!(Timers[x].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::CountUp))) {
            Timers[x].FlushDirect(*Scheduler->timer);
        }
    }
    return (u16)Timers[x].Counter;
}

template<u8 x>
WRITE_CALLBACK(MMIO::WriteTMxCNT_H) {
    log_tmr("Write TM%dCNT_H: %x", x, value);
    /*
     * On writes to TMCNT_H we have to "flush" a timer to it's current value
     * */

    const bool was_enabled = (Timers[x].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::Enabled)) != 0;

    if (was_enabled) {
        // flush timer
        if constexpr (x == 0) {
            // always direct
            Timers[x].FlushDirect(*Scheduler->timer);
        }
        else {
            // maybe count-up
            if (!(Timers[x].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::CountUp))) {
                Timers[x].FlushDirect(*Scheduler->timer);
            }
        }
    }
    else if (value & static_cast<u16>(TMCNT_HFlags::Enabled)) {
        // timer got enabled
        // trigger timing still needs to be set
        Timers[x].Counter = Timers[x].Register.CNT_L;
        Timers[x].TriggerTime = *Scheduler->timer;
    }
    else {
        // nothing interesting happens
        return;
    }

    // update register data
    Timers[x].Register.CNT_H = value;
    Timers[x].PrescalerShift = PrescalerSelection[value & static_cast<u16>(TMCNT_HFlags::Prescaler)];

    // schedule overflow event
    if (x == 0 || !(Timers[x].Register.CNT_H & static_cast<u16>(TMCNT_HFlags::CountUp))) {
        // for direct timers: schedule/reschedule event
        if (unlikely(Timers[x].Overflow.active)) {
            Scheduler->RemoveEvent(&Timers[x].Overflow);
        }

        // delta time
        u64 new_time = (0x10000 - Timers[x].Counter) << Timers[x].PrescalerShift;
        // absolute time
        u64 current_time = *Scheduler->timer;
        new_time = current_time + (current_time - Timers[x].TriggerTime) + new_time;
        Timers[x].Overflow.time = new_time;

        Scheduler->AddEvent(&Timers[x].Overflow);
    }
    else if (unlikely(Timers[x].Overflow.active)) {
        Scheduler->RemoveEvent(&Timers[x].Overflow);
    }
}
