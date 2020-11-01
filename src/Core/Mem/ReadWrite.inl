
template<typename T, bool count>
T Mem::Read(u32 address) {
    if constexpr(count) {
        // todo: actual timings
        (*timer)++;
    }

    switch (static_cast<MemoryRegion>(address >> 24)) {
        case MemoryRegion::BIOS:
            return ReadArray<T>(BIOS, address & 0x3fff);
        case MemoryRegion::Unused:
            return 0;
        case MemoryRegion::eWRAM:
            return ReadArray<T>(eWRAM, address & 0x3'ffff);
        case MemoryRegion::iWRAM:
            return ReadArray<T>(iWRAM, address & 0x7fff);
        case MemoryRegion::IO:
            if ((address & 0x00ff'ffff) > 0x3ff) {
                return 0;
            }
            return IO->Read<T>(address & 0x3ff);
        case MemoryRegion::PAL:
            return ReadArray<T>(PAL, address & 0x3ff);
        case MemoryRegion::VRAM:
            return ReadArray<T>(VRAM, MaskVRAMAddress(address));
        case MemoryRegion::OAM:
            return ReadArray<T>(OAM, address & 0x3ff);
        case MemoryRegion::ROM_L1:
        case MemoryRegion::ROM_L2:
            // todo: EEPROM attempts
        case MemoryRegion::ROM_L:
        case MemoryRegion::ROM_H1:
        case MemoryRegion::ROM_H2:
        case MemoryRegion::ROM_H:
            // todo: EEPROM attempts
            return ReadArray<T>(ROM, address & 0x01ff'ffff);
        case MemoryRegion::SRAM:
            log_warn("SRAM read @0x%08x", address);
            return 0;
        default:
            return 0;
    }
}

template<typename T>
ALWAYS_INLINE void Mem::WriteVRAM(u32 address, T value) {
    // on writes, I guess it's actually pretty likely the old value gets overwritten
    if (likely(ReadArray<T>(VRAM, address) != value)) {
        if ((address + sizeof(T)) > VRAMUpdate.max) {
            VRAMUpdate.max = address + sizeof(T);
        }
        if (address < VRAMUpdate.min) {
            VRAMUpdate.min = address;
        }
    }
}

// We only want to re-flush the ARM7TDMI pipeline if we are in an instruction when it happens
// DMAs, we assume that PC is not in the DMA-ed part of the code
#define NEAR_PC(mask) ((address >> 24) == (*pc_ptr >> 24)) && ((std::abs(int(address - *pc_ptr)) & (mask)) <= 8)
template<typename T, bool count, bool do_reflush>
void Mem::Write(u32 address, T value) {
    if constexpr(count) {
        // todo: actual timings
        (*timer)++;
    }

    switch (static_cast<MemoryRegion>(address >> 24)) {
        case MemoryRegion::BIOS:
            // BIOS is read-only
        case MemoryRegion::Unused:
            return;
        case MemoryRegion::eWRAM:
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3'ffff))) {
//                    log_debug("%x near %x, distance %x", address >> 24, *pc_ptr >> 24, std::abs(int(address - *pc_ptr)) & (0x3'ffff));
                    Reflush();
                }
            }

            WriteArray<T>(eWRAM, address & 0x3'ffff, value);
            return;
        case MemoryRegion::iWRAM:
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x7fff))) {
//                    log_debug("%x near %x, distance %x", address >> 24, *pc_ptr >> 24, std::abs(int(address - *pc_ptr)) & (0x3'ffff));
                    Reflush();
                }
            }

            WriteArray<T>(iWRAM, address & 0x7fff, value);
            return;
        case MemoryRegion::IO:
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3ff))) {
                    log_warn("Code was being ran from MMIO and manipulated");
                    Reflush();
                }
            }
#endif
            IO->Write<T>(address & 0x3ff, value);
            return;
        case MemoryRegion::PAL:
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3ff))) {
                    log_warn("Code was being ran from PAL and manipulated");
                    Reflush();
                }
            }
#endif
            WriteArray<T>(PAL, address & 0x3ff, value);
            return;
        case MemoryRegion::VRAM:
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                // this address is actually not quite right, but we are doing this as check anyway
                // I doubt many games will actually run code from VRAM AND manipulate the code right in front of PC
                if (unlikely(NEAR_PC(0xffff))) {
                    log_warn("Code was being ran from VRAM and manipulated");
                    Reflush();
                }
            }
#endif

            WriteVRAM<T>(MaskVRAMAddress(address), value);
            WriteArray<T>(VRAM, MaskVRAMAddress(address), value);
            return;
        case MemoryRegion::OAM:
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3ff))) {
                    log_warn("Code was being ran from OAM and manipulated");
                    Reflush();
                }
            }
#endif
            WriteArray<T>(OAM, address & 0x3ff, value);
            return;
        case MemoryRegion::ROM_L1:
        case MemoryRegion::ROM_L2:
        case MemoryRegion::ROM_L:
        case MemoryRegion::ROM_H1:
        case MemoryRegion::ROM_H2:
        case MemoryRegion::ROM_H:
            // todo: EEPROM attempts
            return;
        case MemoryRegion::SRAM:
            log_warn("SRAM write @0x%08x", address);
            return;
        default:
            return;
    }
}