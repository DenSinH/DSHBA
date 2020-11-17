
inline WRITE_CALLBACK(MMIO::WriteSquare0Sweep) {
    APU->sq[0].SweepNumber = value & 7;
    APU->sq[0].SweepUp = (value & 0x8) != 0;
    APU->sq[0].SweepPeriod = (value >> 4) & 7;
    APU->sq[0].SweepReload();
}

template<u8 x> WRITE_CALLBACK(MMIO::WriteSquareCNT_L) {
    APU->sq[x].LengthCounter = (64 - value & 0x001f);
    APU->sq[x].SetDuty((value >> 6) & 3);
    APU->sq[x].EnvelopeTime = (value >> 8) & 7;
    APU->sq[x].EnvelopeUp  = (value & 0x0800) != 0;
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