#include "../IO/IOFlags.h"

#include <type_traits>
#include <algorithm>

template<typename T>
static constexpr i32 DeltaXAD(u16 flag) {
    switch (static_cast<DMACNT_HFlags>(flag)) {
        case DMACNT_HFlags::DestIncrementReload:
        case DMACNT_HFlags::DestIncrement:
        // case DMACNT_HFlags::SrcIncrement:  also 0, same as DestIncrement
            return sizeof(T);
        case DMACNT_HFlags::DestDecrement:
        case DMACNT_HFlags::SrcDecrement:
            return -sizeof(T);
        case DMACNT_HFlags::DestFixed:
        case DMACNT_HFlags::SrcFixed:
            return 0;
        default: UNREACHABLE
            log_fatal("Invalid flag passed to DeltaXAD");
    }
}

template<typename T, bool intermittent_events>
void Mem::FastDMA(s_DMAData* DMA) {
    /*
     * Safe direct DMAs with both incrementing addresses
     * */
    u32 cycles_per_access = 1;  // todo

    // align addresses
    u8* dest   = GetPtr(DMA->DAD & ~(sizeof(T) - 1));
    u8* src    = GetPtr(DMA->SAD & ~(sizeof(T) - 1));
    u32 length = DMA->CNT_L ? DMA->CNT_L : DMA->CNT_L_MAX;

    log_dma("Fast DMA %x -> %x (len: %x, control: %04x)", DMA->SAD, DMA->DAD, length, DMA->CNT_H);

    if constexpr(!intermittent_events) {
        // direct memcpy
        log_dma("Fast DMA, no intermissions");
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
        while (length != 0) {
            u16 accesses_until_next_event = (peek_event(Scheduler) / cycles_per_access) + 1; // + 1 to get over the limit
            accesses_until_next_event = MIN(length, accesses_until_next_event);

            log_dma("DMAing chunk of length %x", accesses_until_next_event);
            memcpy(
                    dest,
                    src,
                    accesses_until_next_event * sizeof(T)
            );
            (*timer) += accesses_until_next_event * cycles_per_access;
            do_events(Scheduler);

            length -= accesses_until_next_event;
            dest += accesses_until_next_event * sizeof(T);
            src  += accesses_until_next_event * sizeof(T);
        }
    }

    // update DMA data
    if ((DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl)) != static_cast<u16>(DMACNT_HFlags::DestIncrementReload)) {
        DMA->DAD += length * sizeof(T);
    }
    DMA->SAD += length * sizeof(T);
}

template<typename T, bool intermittent_events>
void Mem::MediumDMA(s_DMAData* DMA) {
    /*
     * Safe direct DMAs with varying src/dest address control
     * */
    u32 cycles_per_access = 1;  // todo

    // align addresses
    u8* dest   = GetPtr(DMA->DAD & ~(sizeof(T) - 1));
    u8* src    = GetPtr(DMA->SAD & ~(sizeof(T) - 1));
    u32 length = DMA->CNT_L ? DMA->CNT_L : DMA->CNT_L_MAX;

    i32 delta_dad = DeltaXAD<T>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl));
    i32 delta_sad = DeltaXAD<T>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::SrcAddrControl));

    log_dma("Medium DMA %x -> %x (len: %x, control: %04x)", DMA->SAD, DMA->DAD, length, DMA->CNT_H);
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
        while (length != 0) {
            u16 accesses_until_next_event = (peek_event(Scheduler) / cycles_per_access) + 1; // + 1 to get over the limit
            accesses_until_next_event = MIN(length, accesses_until_next_event);

            log_dma("DMAing chunk of length %x", accesses_until_next_event);
            for (size_t i = 0; i < accesses_until_next_event; i++) {
                *(T*)dest = *(T*)src;

                dest += delta_dad;
                src  += delta_sad;
            }
            (*timer) += accesses_until_next_event * cycles_per_access;
            do_events(Scheduler);

            length -= accesses_until_next_event;
        }
    }

    // update DMA data
    if ((DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl)) != static_cast<u16>(DMACNT_HFlags::DestIncrementReload)) {
        DMA->DAD += length * delta_dad;
    }
    DMA->SAD += length * delta_sad;
}

template<typename T, bool intermittent_events>
void Mem::SlowDMA(s_DMAData* DMA) {
    // address alignment happens in read/write handlers
    u32 dad = DMA->DAD;
    u32 sad = DMA->SAD;
    u32 length = DMA->CNT_L ? DMA->CNT_L : DMA->CNT_L_MAX;
    const u16 control = DMA->CNT_H;

    i32 delta_dad = DeltaXAD<T>(control & static_cast<u16>(DMACNT_HFlags::DestAddrControl));
    i32 delta_sad = DeltaXAD<T>(control & static_cast<u16>(DMACNT_HFlags::SrcAddrControl));

    log_dma("Slow DMA %x -> %x (len: %x, control: %04x)", sad, dad, DMA->CNT_L, control);

    for (u32 i = 0; i < length; i++) {
        // we don't want to reflush
        // if a game overwrites the area PC is in with DMA, the result is probably fairly unpredictable anyway
        Write<T, true, false>(dad, Read<T, true>(sad));

        if constexpr(intermittent_events) {
            if (unlikely(should_do_events(Scheduler))) {
                do_events(Scheduler);
            }
        }

        dad += delta_dad;
        sad += delta_sad;
    }

    if ((DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl)) != static_cast<u16>(DMACNT_HFlags::DestIncrementReload)) {
        DMA->DAD = dad;
    }

    DMA->SAD = sad;
}

/*
 * We want to allow faster DMAs in certain memory regions
 * BIOS/unused is a separate case, cause nothing can be read from there anyway, so the DMA can be skipped entirely.
 * I/O we have to be careful, as well as EEPROM/Flash
 *
 * We can do a "fast DMA" if:
 *  - the whole transfer is from one memory region to another, without mirroring
 *  - the transfer is incrementing in both source and destination
 *  - it is not from or to IO
 *  - it is not from or to SRAM
 *  - it is not to the BIOS region, an unused region or out of bounds (skip DMA)
 *  - it is not from the BIOS region, an unused region or out of bounds (slow DMA)
 *  - it is not to ROM (in case of EEPROM)
 *  - it is not from the EEPROM region in ROM
 *
 * As for DMA priority: if another DMA is enabled while we are starting the DMA, we have to check events during the
 * transfer.
 *
 * This is easy to build in for a Slow DMA, for a fast DMA it's a little trickier.
 * What I want to do is get the timing for a memory region in advance, calculate how many transfers we can get in
 * until the next event, do this many, do events, repeat.
 * */
enum class DMAAction {
    Fast,
    Medium,
    Slow,
    Skip,  // like DMAs to BIOS (I know PMD does this)
};

/*
 * To check whether no wrapping is involved, we check if
 *  xad & ~mask == (xad + length) & ~mask
 * This fact is mostly important to note for the VRAM mask
 * We already check if a VRAM DMA would go wrong through the weird mirroring before, so
 * we can use 0x1'ffff as a mask
 * */
static constexpr u32 AllowFastDMAAddressMask[16] = {
                  0,           0,    0x3'ffff,      0x7fff,
                  0,       0x3ff,    0x1'ffff,       0x3ff,
        0x01ff'ffff, 0x01ff'ffff, 0x01ff'ffff, 0x01ff'ffff,
        0x01ff'ffff, 0x01ff'ffff,           0,           0,
};

template<typename T>
static constexpr DMAAction AllowFastDMA(const u32 dad, const u32 sad, const u32 length, const u16 control) {
    const auto dar = static_cast<MemoryRegion>(dad >> 24);
    const auto sar = static_cast<MemoryRegion>(sad >> 24);

    switch (dar) {
        case MemoryRegion::ROM_H:
        case MemoryRegion::ROM_H1:
        case MemoryRegion::ROM_H2:
            // todo: if EEPROM: slow
            // otherwise:
        case MemoryRegion::BIOS:
        case MemoryRegion::Unused:
        case MemoryRegion::ROM_L:
        case MemoryRegion::ROM_L1:
        case MemoryRegion::ROM_L2:
        default:
            return DMAAction::Skip;
        case MemoryRegion::IO:
        case MemoryRegion::SRAM:
            return DMAAction::Slow;
        case MemoryRegion::VRAM:
            if (MaskVRAMAddress(dad) + (length * sizeof(T)) > 0x1'8000) {
                return DMAAction::Slow;
            }
        case MemoryRegion::eWRAM:
        case MemoryRegion::iWRAM:
        case MemoryRegion::PAL:
        case MemoryRegion::OAM:
            break;  // allow fast
    }
    log_dma("DAR okay for fast DMA");

    switch (sar) {
        case MemoryRegion::IO:
        case MemoryRegion::SRAM:
        case MemoryRegion::BIOS:
        case MemoryRegion::Unused:
            return DMAAction::Slow;
        case MemoryRegion::VRAM:
            if (MaskVRAMAddress(sad) + (length * sizeof(T)) > 0x1'8000) {
                return DMAAction::Slow;
            }
        case MemoryRegion::eWRAM:
        case MemoryRegion::iWRAM:
        case MemoryRegion::PAL:
        case MemoryRegion::OAM:
        case MemoryRegion::ROM_L:
        case MemoryRegion::ROM_L1:
        case MemoryRegion::ROM_L2:
            break;  // allow fast
        case MemoryRegion::ROM_H:
        case MemoryRegion::ROM_H1:
        case MemoryRegion::ROM_H2:
            // todo: if EEPROM: slow
            // otherwise:
            break; // allow fast
        default:
            return DMAAction::Skip;
    }
    log_dma("SAR okay for fast DMA");

    const u32 dad_start_masked = dad & ~AllowFastDMAAddressMask[static_cast<u32>(dar)];
    const u32 dad_end_masked   = (dad + (length * sizeof(T))) & ~AllowFastDMAAddressMask[static_cast<u32>(dar)];
    const u32 sad_start_masked = sad & ~AllowFastDMAAddressMask[static_cast<u32>(sar)];
    const u32 sad_end_masked   = (sad + (length * sizeof(T))) & ~AllowFastDMAAddressMask[static_cast<u32>(sar)];

    if (dad_start_masked != dad_end_masked) {
        // destination mirroring effect
        log_dma("DAD masked %x not the same as DAD ending masked %x", dad_start_masked, dad_end_masked);
        return DMAAction::Slow;
    }

    if (sad_start_masked != sad_end_masked) {
        log_dma("SAD masked %x not the same as SAD ending masked %x", sad_start_masked, sad_end_masked);
        // source mirroring effect
        return DMAAction::Slow;
    }

    if (DeltaXAD<T>(control & static_cast<u16>(DMACNT_HFlags::DestAddrControl)) <= 0) {
        // decrement/fixed destination
        return DMAAction::Medium;
    }

    if (DeltaXAD<T>(control & static_cast<u16>(DMACNT_HFlags::SrcAddrControl)) <= 0) {
        // decrement/fixed source
        return DMAAction::Medium;
    }

    return DMAAction::Fast;
}

template<typename T, bool intermittent_events>
void Mem::DoDMA(s_DMAData* DMA) {
    u32 length = DMA->CNT_L ? DMA->CNT_L : DMA->CNT_L_MAX;

    DMAAction dma_action = AllowFastDMA<T>(DMA->DAD, DMA->SAD, length, DMA->CNT_H);

    switch(dma_action) {
        case DMAAction::Medium:
            // Invalidate VRAM, we do this before the transfer because the transfer updates DMA*
            if (static_cast<MemoryRegion>(DMA->DAD >> 24) == MemoryRegion::VRAM) {
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
            }
            MediumDMA<T, intermittent_events>(DMA);
            break;
        case DMAAction::Fast:
            // invalidate VRAM, we do this before the transfer because the transfer updates DMA*
            // for fast DMAs, we know both address controls are increasing
            if (static_cast<MemoryRegion>(DMA->DAD >> 24) == MemoryRegion::VRAM) {
                u32 low  = MaskVRAMAddress(DMA->DAD);
                u32 high = MaskVRAMAddress(DMA->DAD + length * sizeof(T));

                if ((high + sizeof(T)) > VRAMUpdate.max) {
                    VRAMUpdate.max = high + sizeof(T);
                }
                if (low < VRAMUpdate.min) {
                    VRAMUpdate.min = low;
                }
            }
            FastDMA<T, intermittent_events>(DMA);
            return;
        case DMAAction::Slow:
            SlowDMA<T, intermittent_events>(DMA);
            return;
        case DMAAction::Skip: {
            u32 cycles_per_access = 1;  // todo
            (*timer) += cycles_per_access * length;

            if ((DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl)) != static_cast<u16>(DMACNT_HFlags::DestIncrementReload)) {
                DMA->DAD += length * DeltaXAD<T>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl));
            }
            DMA->SAD += length * DeltaXAD<T>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::SrcAddrControl));;
        }
        default:
            break;
    }
}