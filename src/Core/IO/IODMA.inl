
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

#ifdef DIRECT_IO_DATA_COPY
        // DMACNT_H already copied over
        memcpy(
                &DMAData[x],
                &Registers[static_cast<u32>(IORegister::DMA0SAD) + x * 0xc],
                sizeof(s_DMAData) - sizeof(u16)
        );
#else
        DMAData[x].SAD   = ReadArray<u32>(Registers, static_cast<u32>(IORegister::DMA0SAD)   + x * 0xc);
        DMAData[x].DAD   = ReadArray<u32>(Registers, static_cast<u32>(IORegister::DMA0DAD)   + x * 0xc);
        DMAData[x].CNT_L = ReadArray<u16>(Registers, static_cast<u32>(IORegister::DMA0CNT_L) + x * 0xc);
#endif

        if ((value & static_cast<u16>(DMACNT_HFlags::StartTiming)) == static_cast<u16>(DMACNT_HFlags::StartImmediate)) {
            // no need to have shadow copies for immediate DMAs
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

    IO->RunDMAChannel(x);
}