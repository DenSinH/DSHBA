
template<u8 x>
WRITE_CALLBACK(MMIO::WriteDMAxCNT_H) {
    bool was_enabled = (DMAData[x].CNT_H & static_cast<u16>(DMACNT_HFlags::Enable)) != 0;
    DMAData[x].CNT_H = value;
    log_dma("Wrote to DMA%dCNT_H: %04x", x, value);

    if (!was_enabled && (value & static_cast<u16>(DMACNT_HFlags::Enable))) {
        // DMA enable, store DMASAD/DAD/CNT_L registers in shadow registers
        log_dma("Settings: SAD: %08x, DAD: %08x, CNT_L: %04x",
                ReadArray<u32>(Registers, static_cast<u32>(IORegister::DMA0SAD) + x * 0xc),
                ReadArray<u32>(Registers, static_cast<u32>(IORegister::DMA0DAD) + x * 0xc),
                ReadArray<u16>(Registers, static_cast<u32>(IORegister::DMA0CNT_L) + x * 0xc)
        );

        DMAData[x].SAD   = ReadArray<u32>(Registers, static_cast<u32>(IORegister::DMA0SAD)   + x * 0xc);
        DMAData[x].DAD   = ReadArray<u32>(Registers, static_cast<u32>(IORegister::DMA0DAD)   + x * 0xc);
        DMAData[x].CNT_L = ReadArray<u16>(Registers, static_cast<u32>(IORegister::DMA0CNT_L) + x * 0xc);

        if constexpr(x == 1 || x == 2) {
            // possible audio DMA
            if ((value & static_cast<u16>(DMACNT_HFlags::Repeat)) &&
                    ((value & static_cast<u16>(DMACNT_HFlags::StartTiming)) == static_cast<u16>(DMACNT_HFlags::StartSpecial))) {
                // destination address must be correct
                if ((DMAData[x].DAD & 0xffff'fff0) == 0x0400'00a0) {

                    // Audio DMA
                    // always 4 words (32 bit), DMA transfer type and CNT_L are ignored
                    DMAData[x].CNT_L = 4;
                    DMAData[x].CNT_H |= static_cast<u16>(DMACNT_HFlags::WordSized);

                    // destination address will not be incremented
                    DMAData[x].CNT_H &= ~static_cast<u16>(DMACNT_HFlags::DestAddrControl);
                    DMAData[x].CNT_H |= static_cast<u16>(DMACNT_HFlags::DestFixed);
                    DMAData[x].Audio = true;
                    log_dma("Audio DMA %x, %x -> %x", x, DMAData[x].SAD, DMAData[x].DAD);
                }
                else {
                    DMAData[x].Audio = false;
                }
            }
            else {
                DMAData[x].Audio = false;
            }
        }

        if ((value & static_cast<u16>(DMACNT_HFlags::StartTiming)) == static_cast<u16>(DMACNT_HFlags::StartImmediate)) {
            // immediate DMAs, just trigger here
            log_dma("DMA started");
            // trigger, but don't mark as enabled
            TriggerDMAChannel(x);
        }
        else {
            // mark as enabled
            DMAEnabled[x] = true;
        }
    }
}

template<u8 x>
READ_PRECALL(MMIO::ReadDMAxCNT_H) {
    return DMAData[x].CNT_H;
}

template<u8 x>
SCHEDULER_EVENT(MMIO::DMAStartEvent) {
    auto IO = (MMIO*)caller;

    return IO->RunDMAChannel(x);  // interrupts might happen during DMA
}