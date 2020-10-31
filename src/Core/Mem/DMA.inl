#include "../IO/IOFlags.h"

#include <type_traits>


template<typename T, bool intermittent_events>
void Mem::FastDMA(s_DMAData* DMA) {
    u32 cycles_per_access = 1;  // todo
    u8* dest   = GetPtr(DMA->DAD);
    u8* src    = GetPtr(DMA->SAD);
    u32 length = DMA->CNT_L ? DMA->CNT_L : DMA->CNT_L_MAX;

    log_dma("Fast DMA %x -> %x (len: %x, control: %04x)", DMA->SAD, DMA->DAD, length, DMA->CNT_H);

    if constexpr(!intermittent_events) {
        // direct memcpy
        log_dma("Fast DMA, no intermissions");
        memcpy(
                dest,
                src,
                length << (std::is_same_v<T, u32> ? 2 : 1)
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
                    accesses_until_next_event << (std::is_same_v<T, u32> ? 2 : 1)
            );
            (*timer) += accesses_until_next_event * cycles_per_access;
            do_events(Scheduler);

            length -= accesses_until_next_event;
            dest += accesses_until_next_event << (std::is_same_v<T, u32> ? 2 : 1);
            src  += accesses_until_next_event << (std::is_same_v<T, u32> ? 2 : 1);
        }
    }

    // update DMA data
    if ((DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl)) != static_cast<u16>(DMACNT_HFlags::DestIncrementReload)) {
        DMA->DAD += length << (std::is_same_v<T, u32> ? 2 : 1);
    }
    DMA->SAD += length << (std::is_same_v<T, u32> ? 2 : 1);
}

template<typename T, bool intermittent_events>
void Mem::SlowDMA(s_DMAData* DMA) {
    u32 dad = DMA->DAD;
    u32 sad = DMA->SAD;
    u32 length = DMA->CNT_L ? DMA->CNT_L : DMA->CNT_L_MAX;
    const u16 control = DMA->CNT_H;

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

        switch (static_cast<DMACNT_HFlags>(control & static_cast<u16>(DMACNT_HFlags::DestAddrControl))) {
            case DMACNT_HFlags::DestIncrement:
            case DMACNT_HFlags::DestIncrementReload:
                dad += sizeof(T);
                break;
            case DMACNT_HFlags::DestDecrement:
                dad -= sizeof(T);
                break;
            case DMACNT_HFlags::DestFixed:
            default:
                break;
        }

        switch (static_cast<DMACNT_HFlags>(control & static_cast<u16>(DMACNT_HFlags::SrcAddrControl))) {
            case DMACNT_HFlags::SrcIncrement:
                sad += sizeof(T);
                break;
            case DMACNT_HFlags::SrcDecrement:
                sad -= sizeof(T);
                break;
            case DMACNT_HFlags::SrcFixed:
            default:
                break;
        }
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
             0x3fff,           0,    0x3'ffff,           0,
             0x7fff,       0x3ff,    0x1'ffff,       0x3ff,
        0x01ff'ffff, 0x01ff'ffff, 0x01ff'ffff, 0x01ff'ffff,
        0x01ff'ffff, 0x01ff'ffff,           0,           0,
};

template<typename T>
static constexpr DMAAction AllowFastDMA(const u32 dad, const u32 sad, const u32 length) {
    const auto dar = static_cast<MemoryRegion>(dad >> 24);
    const auto sar = static_cast<MemoryRegion>(sad >> 24);

    switch (dar) {
        case MemoryRegion::ROM_H:
        case MemoryRegion::ROM_H1:
        case MemoryRegion::ROM_H2:
            // if EEPROM: slow
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
            if (MaskVRAMAddress(dad) + (length << (std::is_same_v<T, u32> ? 2 : 1)) > 0x1'8000) {
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
            if (MaskVRAMAddress(sad) + (length << (std::is_same_v<T, u32> ? 2 : 1)) > 0x1'8000) {
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
            // if EEPROM: slow
            // otherwise:
            break; // allow fast
        default:
            return DMAAction::Skip;
    }
    log_dma("SAR okay for fast DMA");

    const u32 dad_start_masked = dad & ~AllowFastDMAAddressMask[static_cast<u32>(dar)];
    const u32 dad_end_masked   = (dad + (length << (std::is_same_v<T, u32> ? 2 : 1))) & ~AllowFastDMAAddressMask[static_cast<u32>(dar)];
    const u32 sad_start_masked = sad & ~AllowFastDMAAddressMask[static_cast<u32>(sar)];
    const u32 sad_end_masked   = (sad + (length << (std::is_same_v<T, u32> ? 2 : 1))) & ~AllowFastDMAAddressMask[static_cast<u32>(sar)];

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

    return DMAAction::Fast;
}

template<typename T, bool intermittent_events>
void Mem::DoDMA(s_DMAData* DMA) {
    u32 length = DMA->CNT_L ? DMA->CNT_L : DMA->CNT_L_MAX;

    switch(AllowFastDMA<T>(DMA->DAD, DMA->SAD, length)) {
        case DMAAction::Fast:
            // invalidate VRAM
            if (static_cast<MemoryRegion>(DMA->DAD >> 24) == MemoryRegion::VRAM) {
                u32 low, high;

                if (static_cast<DMACNT_HFlags>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl)) ==
                    DMACNT_HFlags::DestDecrement
                        ) {
                    low = DMA->DAD - (length << (std::is_same_v<T, u32> ? 2 : 1));
                    high = DMA->DAD;
                }
                else {
                    low = DMA->DAD;
                    high = DMA->DAD + ((length + 1) << (std::is_same_v<T, u32> ? 2 : 1));
                }

                low  = MaskVRAMAddress(low);
                high = MaskVRAMAddress(high);

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
                switch (static_cast<DMACNT_HFlags>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl))) {
                    case DMACNT_HFlags::DestIncrement:
                        DMA->DAD += length * sizeof(T);
                        break;
                    case DMACNT_HFlags::DestDecrement:
                        DMA->DAD -= length * sizeof(T);
                        break;
                    case DMACNT_HFlags::DestFixed:
                    default:
                        break;
                }
            }

            switch (static_cast<DMACNT_HFlags>(DMA->CNT_H & static_cast<u16>(DMACNT_HFlags::SrcAddrControl))) {
                case DMACNT_HFlags::SrcIncrement:
                    DMA->SAD += length * sizeof(T);
                    break;
                case DMACNT_HFlags::SrcDecrement:
                    DMA->DAD -= length * sizeof(T);
                    break;
                case DMACNT_HFlags::SrcFixed:
                default:
                    break;
            }
        }
        default:
            break;
    }
}