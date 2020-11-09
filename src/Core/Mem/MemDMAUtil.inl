#ifndef INLINED_INCLUDES
#include "Mem.h"

class Mem_INL : Mem {
#endif

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

#ifdef FAST_DMA

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

    if (DeltaXAD<T>(control & static_cast<u16>(DMACNT_HFlags::SrcAddrControl)) <= 0 && (dar < MemoryRegion::ROM_L)) {
        // decrement/fixed source
        return DMAAction::Medium;
    }

    return DMAAction::Fast;
}
#endif

#ifndef INLINED_INCLUDES
};
#endif