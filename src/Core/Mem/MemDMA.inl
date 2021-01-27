
#ifdef FAST_DMA
template<typename T, bool intermittent_events>
void Mem::FastDMA(s_DMAData* DMA) {
    /*
     * Safe direct DMAs with both incrementing addresses
     * */
    u32 cycles_per_access = GetAccessTime<T>(static_cast<MemoryRegion>(DMA->DAD >> 24));
    cycles_per_access += GetAccessTime<T>(static_cast<MemoryRegion>(DMA->SAD >> 24));

    // align addresses
    u8* dest   = GetPtr(DMA->DAD & ~(sizeof(T) - 1));
    u8* src    = GetPtr(DMA->SAD & ~(sizeof(T) - 1));
    const u32 length = DMA->CNT_L ? DMA->CNT_L : DMA->CNT_L_MAX;
    log_dma("Fast DMA %x -> %x (len: %x, control: %04x), size %d", DMA->SAD, DMA->DAD, length, DMA->CNT_H, sizeof(T));

    if constexpr(!intermittent_events) {
        // direct memcpy
        log_dma("Fast DMA (%x bytes, first value: %x), no intermissions", length * sizeof(T), *(T*)src);
        memcpy(
                dest,
                src,
                length * sizeof(T)
        );
        (*timer) += length * cycles_per_access;
    }
    else {
        log_dma("Fast DMA with intermissions");
        // intermittent memcpy
        u32 units_left = length;
        while (units_left != 0) {
            u16 accesses_until_next_event = (Scheduler->PeekEvent() / cycles_per_access) + 1; // + 1 to get over the limit
            accesses_until_next_event = MIN(units_left, accesses_until_next_event);

            log_dma("DMAing chunk of length %x", accesses_until_next_event);
            memcpy(
                    dest,
                    src,
                    accesses_until_next_event * sizeof(T)
            );
            (*timer) += accesses_until_next_event * cycles_per_access;
            Scheduler->DoEvents();

            units_left -= accesses_until_next_event;
            dest += accesses_until_next_event * sizeof(T);
            src  += accesses_until_next_event * sizeof(T);
        }
    }

    // get last transferred value as DMA latched value
    if constexpr(std::is_same_v<T, u16>) {
        DMALatch = *(u16*)(dest - sizeof(u16));
        DMALatch |= DMALatch << 16;
    }
    else {
        DMALatch = *(u32*)(dest - sizeof(u32));
    }

    log_dma("DMA latch (fast %dbit): %x", sizeof(T) << 3, DMALatch);
    // update DMA data
    // handle writeback/reload in IO class
    DMA->DAD += length * sizeof(T);
    DMA->SAD += length * sizeof(T);
}

template<typename T, bool intermittent_events>
void Mem::MediumDMA(s_DMAData* DMA) {
    /*
     * Safe direct DMAs with varying src/dest address control
     * */
    u32 cycles_per_access = GetAccessTime<T>(static_cast<MemoryRegion>(DMA->DAD >> 24));
    cycles_per_access += GetAccessTime<T>(static_cast<MemoryRegion>(DMA->SAD >> 24));

    // align addresses
    u8* dest   = GetPtr(DMA->DAD & ~(sizeof(T) - 1));
    u8* src    = GetPtr(DMA->SAD & ~(sizeof(T) - 1));
    const u32 length = DMA->CNT_L ? DMA->CNT_L : DMA->CNT_L_MAX;

    i32 delta_dad = DeltaXAD<T>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl));
    i32 delta_sad;
    if (DMA->SAD < 0x0800'0000) {
        delta_sad = DeltaXAD<T>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::SrcAddrControl));
    }
    else {
        delta_sad = sizeof(T);
    }

    log_dma("Medium DMA %x -> %x (len: %x, control: %04x), size %d", DMA->SAD, DMA->DAD, length, DMA->CNT_H, sizeof(T));
    log_dma("ddad = %x, dsad = %x", delta_dad, delta_sad);
    if constexpr(!intermittent_events) {
        // direct memcpy
        log_dma("Medium DMA, no intermissions");
        for (size_t i = 0; i < length; i++) {
            *(T*)dest = *(T*)src;

            dest += delta_dad;
            src  += delta_sad;
        }
        (*timer) += length * cycles_per_access;
    }
    else {
        log_dma("Medium DMA with intermissions");
        // intermittent memcpy
        u32 units_left = length;
        while (units_left != 0) {
            u16 accesses_until_next_event = (Scheduler->PeekEvent() / cycles_per_access) + 1; // + 1 to get over the limit
            accesses_until_next_event = MIN(units_left, accesses_until_next_event);

            log_dma("DMAing chunk of length %x", accesses_until_next_event);
            for (size_t i = 0; i < accesses_until_next_event; i++) {
                *(T*)dest = *(T*)src;

                dest += delta_dad;
                src  += delta_sad;
            }
            (*timer) += accesses_until_next_event * cycles_per_access;
            Scheduler->DoEvents();

            units_left -= accesses_until_next_event;
        }
    }

    // get last transferred value as DMA latched value
    if constexpr(std::is_same_v<T, u16>) {
        DMALatch = *(u16*)(dest - delta_dad);
        DMALatch |= DMALatch << 16;
    }
    else {
        DMALatch = *(u32*)(dest - delta_dad);
    }
    log_dma("DMA latch (med %dbit): %x", sizeof(T) << 3, DMALatch);

    // update DMA data
    // handle writeback/reload in IO class
    DMA->DAD += length * delta_dad;
    DMA->SAD += length * delta_sad;
    log_dma("Post medium SAD %x, DAD %x", DMA->SAD, DMA->DAD);
}
#endif

template<typename T, bool intermittent_events>
void Mem::SlowDMA(s_DMAData* DMA) {
    // address alignment happens in read/write handlers
    u32 dad = DMA->DAD;
    u32 sad = DMA->SAD;
    u32 length = DMA->CNT_L ? DMA->CNT_L : DMA->CNT_L_MAX;
    const u16 control = DMA->CNT_H;

    i32 delta_dad = DeltaXAD<T>(control & static_cast<u16>(DMACNT_HFlags::DestAddrControl));

    i32 delta_sad;
    if (DMA->SAD < 0x0800'0000) {
        delta_sad = DeltaXAD<T>(control & static_cast<u16>(DMACNT_HFlags::SrcAddrControl));
    }
    else {
        delta_sad = sizeof(T);
    }

    log_dma("Slow DMA %x -> %x (len: %x, control: %04x)", sad, dad, DMA->CNT_L, control);

    u32 latch;
    for (u32 i = 0; i < length; i++) {
        // we don't want to reflush
        // if a game overwrites the area PC is in with DMA, the result is probably fairly unpredictable anyway
        latch = Read<T, true, true>(sad);
        Write<T, true, false>(dad, latch);

        if constexpr(intermittent_events) {
            if (unlikely(Scheduler->ShouldDoEvents())) {
                Scheduler->DoEvents();
            }
        }

        dad += delta_dad;
        sad += delta_sad;
    }

    // get last transferred word as DMA latch
    // for slow DMAs we have to be careful with reading the latched value
    if constexpr(std::is_same_v<T, u16>) {
        DMALatch = latch;
        DMALatch |= DMALatch << 16;
    }
    else {
        DMALatch = latch;
    }
    log_dma("DMA latch (slow %dbit): %x", sizeof(T) << 3, DMALatch);

    // handle writeback/reload in IO class
    DMA->DAD = dad;
    DMA->SAD = sad;
}

template<typename T, bool intermittent_events>
void Mem::DoDMA(s_DMAData* DMA) {
#ifdef FAST_DMA
    u32 length = DMA->CNT_L ? DMA->CNT_L : DMA->CNT_L_MAX;

    DMAAction dma_action = AllowFastDMA<T>(DMA->DAD, DMA->SAD, length, DMA->CNT_H);
#endif

    if (unlikely(Type == BackupType::EEPROM)) {
        // unspecified EEPROM
        if (DMA->DAD >= 0x0800'0000) {
            // DMA to EEPROM
            if (likely(!((EEPROM *) Backup)->BusWidth)) {
                if (DMA->CNT_L > 9) {
                    ((EEPROM *) Backup)->SetBusWidth(EEPROM_64K_BUS_WIDTH);
                    Type = BackupType::EEPROM_64;
                } else {
                    ((EEPROM *) Backup)->SetBusWidth(EEPROM_4K_BUS_WIDTH);
                    Type = BackupType::EEPROM_4;
                }
            } else {
                // this should not happen, lets just prevent it from happening again
                if (((EEPROM *) Backup)->BusWidth == EEPROM_4K_BUS_WIDTH) {
                    Type = BackupType::EEPROM_4;
                } else {
                    Type = BackupType::EEPROM_64;
                }
            }
        }
    }
#ifdef FAST_DMA
    else if (static_cast<u8>(Type) & static_cast<u8>(BackupType::EEPROM_bit)) {
        if (DMA->DAD >= 0x0800'0000 || DMA->SAD >= 0x0800'0000) {
            // DMA to EEPROM
            // log_mem("Potential EEPROM DMA: %x -> %x", DMA->SAD, DMA->DAD);
            dma_action = DMAAction::Slow;
        }
    }

    if (DMA->DAD > (0x0800'0000 + ROMSize)) {
        dma_action = DMAAction::Slow;  // ROM OOB DMA
    }
#endif

#ifdef FAST_DMA
    switch(dma_action) {
        case DMAAction::Medium:
            // Invalidate VRAM, we do this before the transfer because the transfer updates DMA*
            switch (static_cast<MemoryRegion>(DMA->DAD >> 24)) {
                case MemoryRegion::iWRAM: {
                    u32 low  = MaskVRAMAddress(DMA->DAD);
                    u32 high = MaskVRAMAddress(DMA->DAD + length * DeltaXAD<T>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl)));

                    if (unlikely(high < low)) {
                        // decrementing DMA
                        std::swap(high, low);
                    }

                    // todo: move this
                    constexpr size_t InstructionCacheSize = 256;
                    for (u32 addr = low & ~((InstructionCacheSize << 1) - 1); addr < high; addr += InstructionCacheSize << 1) {
                        iWRAMWrite(addr);
                    }
                }
                case MemoryRegion::VRAM:
                {
                    u32 low  = MaskVRAMAddress(DMA->DAD);
                    u32 high = MaskVRAMAddress(DMA->DAD + length * DeltaXAD<T>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl)));

                    if (unlikely(high < low)) {
                        // decrementing DMA
                        std::swap(high, low);
                    }

                    if ((high + sizeof(T)) > VRAMUpdate.max) {
                        VRAMUpdate.max = high + sizeof(T);
                    }
                    if (low < VRAMUpdate.min) {
                        VRAMUpdate.min = low;
                    }
                    break;
                }
                case MemoryRegion::OAM:
                    DirtyOAM = true;
                    break;
                default:
                    break;
            }

            MediumDMA<T, intermittent_events>(DMA);
            break;
        case DMAAction::Fast:
            // invalidate VRAM and OAM, we do this before the transfer because the transfer updates DMA*
            // for fast DMAs, we know both address controls are increasing
            switch (static_cast<MemoryRegion>(DMA->DAD >> 24)) {
                case MemoryRegion::iWRAM: {
                    u32 low  = MaskVRAMAddress(DMA->DAD);
                    u32 high = MaskVRAMAddress(DMA->DAD + length * DeltaXAD<T>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl)));

                    // todo: move this
                    constexpr size_t InstructionCacheSize = 256;
                    for (u32 addr = low & ~((InstructionCacheSize << 1) - 1); addr < high; addr += InstructionCacheSize << 1) {
                        iWRAMWrite(addr);
                    }
                }
                case MemoryRegion::VRAM:
                {
                    u32 low  = MaskVRAMAddress(DMA->DAD);
                    u32 high = MaskVRAMAddress(DMA->DAD + length * sizeof(T));

                    if ((high + sizeof(T)) > VRAMUpdate.max) {
                        VRAMUpdate.max = high + sizeof(T);
                    }
                    if (low < VRAMUpdate.min) {
                        VRAMUpdate.min = low;
                    }
                    break;
                }
                case MemoryRegion::OAM:
                    DirtyOAM = true;
                    break;
                default:
                    break;
            }

            FastDMA<T, intermittent_events>(DMA);
            return;
        case DMAAction::Slow:
            SlowDMA<T, intermittent_events>(DMA);
            return;
        case DMAAction::Skip: {
            log_dma("Skipping DMA");
            u32 cycles_per_access = GetAccessTime<T>(static_cast<MemoryRegion>(DMA->DAD >> 24));
            cycles_per_access += GetAccessTime<T>(static_cast<MemoryRegion>(DMA->SAD >> 24));

            (*timer) += cycles_per_access * length;

            // handle writeback/reload in IO class
            DMA->DAD += length * DeltaXAD<T>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl));
            DMA->SAD += length * DeltaXAD<T>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::SrcAddrControl));;
        }
        default:
            break;
    }
#else
    SlowDMA<T, intermittent_events>(DMA);
#endif
}