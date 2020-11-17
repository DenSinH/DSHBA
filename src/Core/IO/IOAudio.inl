
inline WRITE_CALLBACK(MMIO::WriteSquare0Sweep) {
    // todo: sweep
}

template<u8 x> WRITE_CALLBACK(MMIO::WriteSquareCNT_L) {
    APU->sq[x].LengthCounter = (64 - value & 0x001f);
    APU->sq[x].SetDuty((value >> 6) & 3);
    // todo: envelope
    APU->sq[x].Volume = value >> 12;
}

template<u8 x> WRITE_CALLBACK(MMIO::WriteSquareCNT_H) {
    // square wave channels tick 8 times as fast because of the pulse width setting,
    // so 128 / 8 = 16, 128 = ARM7TDMI.Frequency / 131072
    APU->sq[x].Period     = (2048 - (value & 0x07ff)) << 4;
    APU->sq[x].LengthFlag = (value & 0x4000) != 0;
    if (value & 0x8000) {
        APU->sq[x].Trigger();
    }
}

inline WRITE_CALLBACK(MMIO::WriteNoiseCNT_L) {
    APU->ns.LengthCounter = (64 - value & 0x001f);
    // todo: envelope
    APU->ns.Volume = value >> 12;
}

inline WRITE_CALLBACK(MMIO::WriteNoiseCNT_H) {
    u32 r = value & 0x0007;
    u32 s = (value & 0x00f0) >> 4;
    // ARM7TDMI.Frequency / 524288 = 32
    if (r == 0)
    {
        // interpret as 0.5 instead
        APU->ns.Period = 32 * 2 * (2 << s);
    }
    else
    {
        APU->ns.Period = 32 * (r * (2 << s));
    }

    APU->ns.CounterStepWidth = (value & 0x0008) != 0;
    APU->ns.LengthFlag = (value & 0x4000) > 0;
    if (value & 0x8000)  {
        APU->ns.Trigger();
    };
}
