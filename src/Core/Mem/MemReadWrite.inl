
template<typename T, bool count, bool in_dma>
ALWAYS_INLINE T Mem::ReadInline(u32 address) {
    // often, code will be ran from iWRAM
    // this makes it the hottest path
    // this will likely still be optimized as part of the switch statement by the compiler
    if (likely(static_cast<MemoryRegion>(address >> 24) == MemoryRegion::iWRAM)) {
        if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::iWRAM>(); }
        return ReadArray<T>(iWRAM, address & 0x7fff);
    }

    switch (static_cast<MemoryRegion>(address >> 24)) {
        case MemoryRegion::BIOS:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::BIOS>(); }
            if constexpr(in_dma) {
                return ReadDMALatch<T>(address);
            }
            if (likely((*pc_ptr) < 0x0100'0000)) {
                // read from within BIOS
                return ReadArray<T>(BIOS, address & 0x3fff);
            }
            return (T)static_cast<u32>(CurrentBIOSReadState);
        case MemoryRegion::Unused:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::Unused>(); }
            if constexpr(in_dma) {
                return ReadDMALatch<T>(address);
            }
            // todo: open bus
            return 0;
        case MemoryRegion::eWRAM:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::eWRAM>(); }
            return ReadArray<T>(eWRAM, address & 0x3'ffff);
        case MemoryRegion::IO:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::IO>(); }
            if ((address & 0x00ff'ffff) < 0x3ff) {
                return IO->Read<T>(address & 0x3ff);
            }
            if constexpr(in_dma) {
                return ReadDMALatch<T>(address);
            }
            return 0;  // todo: invalid reads
        case MemoryRegion::PAL:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::PAL>(); }
            return ReadArray<T>(PAL, address & 0x3ff);
        case MemoryRegion::VRAM:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::VRAM>(); }
            return ReadArray<T>(VRAM, MaskVRAMAddress(address));
        case MemoryRegion::OAM:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::OAM>(); }
            return ReadArray<T>(OAM, address & 0x3ff);
        case MemoryRegion::ROM_L1:
        case MemoryRegion::ROM_L2:
        case MemoryRegion::ROM_L:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::ROM_L>(); }
            if constexpr(in_dma) {
                if ((address & 0x01ff'ffff) > ROMSize) {
                    return ReadDMALatch<T>(address);
                }
            }

            return ReadArray<T>(ROM, address & 0x01ff'ffff);
        case MemoryRegion::ROM_H1:
        case MemoryRegion::ROM_H2:
        case MemoryRegion::ROM_H:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::ROM_H>(); }
            // EEPROM might be accessed in this region
            if (static_cast<u8>(Type) & static_cast<u8>(BackupType::EEPROM_bit)) {
                if (IsEEPROMAccess(address)) {
                    return Backup->Read(address);
                }
            }

            if constexpr(in_dma) {
                if ((address & 0x01ff'ffff) > ROMSize) {
                    return ReadDMALatch<T>(address);
                }
            }

            return ReadArray<T>(ROM, address & 0x01ff'ffff);
        case MemoryRegion::SRAM:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::SRAM>(); }
            if constexpr(std::is_same_v<T, u16>) {
                return 0x0101 * Backup->Read(address);
            }
            else if constexpr(std::is_same_v<T, u32>) {
                return 0x01010101 * Backup->Read(address);
            }
            return Backup->Read(address);
        default:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::OOB>(); }
            return 0;
    }
}


template<typename T, bool count, bool in_dma>
T Mem::Read(u32 address) {
    return ReadInline<T, count, in_dma>(address);
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
    switch (static_cast<MemoryRegion>(address >> 24)) {
        case MemoryRegion::BIOS:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::BIOS>(); }
            // BIOS is read-only
            return;
        case MemoryRegion::Unused:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::Unused>(); }
            return;
        case MemoryRegion::eWRAM:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::eWRAM>(); }
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3'ffff))) {
                    Reflush();
                }
            }

            WriteArray<T>(eWRAM, address & 0x3'ffff, value);
            return;
        case MemoryRegion::iWRAM:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::iWRAM>(); }
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x7fff))) {
                    Reflush();
                }
            }

            WriteArray<T>(iWRAM, address & 0x7fff, value);
            return;
        case MemoryRegion::IO:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::IO>(); }
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3ff))) {
                    log_warn("Code was being ran from MMIO and manipulated");
                    Reflush();
                }
            }
#endif
            if ((address & 0x00ff'ffff) < 0x3ff) {
                IO->Write<T>(address & 0x3ff, value);
            }
            return;
        case MemoryRegion::PAL:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::PAL>(); }
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3ff))) {
                    log_warn("Code was being ran from PAL and manipulated");
                    Reflush();
                }
            }
#endif
            if constexpr(std::is_same_v<T, u8>) {
                // mirrored byte writes
                WriteArray<u16>(PAL, address & 0x3ff, (u16)value | ((u16)value << 8));
            }
            else {
                WriteArray<T>(PAL, address & 0x3ff, value);
            }
            return;
        case MemoryRegion::VRAM: {
            u32 masked_address = MaskVRAMAddress(address);
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::VRAM>(); }
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

            if constexpr(std::is_same_v<T, u8>) {
                if (unlikely(((IO->DISPSTAT & 0x7) >= 3) && (masked_address >= 0x1'4000))) {
                    // ignore VRAM byte stores in BM modes
                }
                else if (unlikely((IO->DISPSTAT & 0x7) < 3 && (masked_address >= 0x1'0000))) {
                    // ignore byte writes in non-bitmap modes
                }
                else {
                    // mirrored byte writes
                    WriteVRAM<u16>(masked_address, value);
                    WriteArray<u16>(VRAM, masked_address, (u16)value | ((u16)value << 8));
                }
            }
            else {
                WriteVRAM<T>(masked_address, value);
                WriteArray<T>(VRAM, masked_address, value);
            }
            return;
        }
        case MemoryRegion::OAM:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::OAM>(); }
            if constexpr(std::is_same_v<T, u8>) {
                // byte writes are ignored
                return;
            }
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3ff))) {
                    log_warn("Code was being ran from OAM and manipulated");
                    Reflush();
                }
            }
#endif
            DirtyOAM |= ReadArray<T>(OAM, (address & 0x3ff) != value);
            WriteArray<T>(OAM, address & 0x3ff, value);
            return;
        case MemoryRegion::ROM_L1:
        case MemoryRegion::ROM_L2:
        case MemoryRegion::ROM_L:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::ROM_L>(); }
            return;
        case MemoryRegion::ROM_H1:
        case MemoryRegion::ROM_H2:
        case MemoryRegion::ROM_H:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::ROM_H>(); }

            // EEPROM might be accessed in this region
            if (static_cast<u8>(Type) & static_cast<u8>(BackupType::EEPROM_bit)) {
                if (IsEEPROMAccess(address)) {
                    Backup->Write(address, value);
                    Backup->Dirty = BackupMem::MaxDirtyChecks;
                    if (!DumpSave.active) {
                        Scheduler->AddEventAfter(&DumpSave, CYCLES_PER_FRAME);
                    }
                }
            }
            return;
        case MemoryRegion::SRAM:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::SRAM>(); }
            Backup->Write(address, value);
            Backup->Dirty = BackupMem::MaxDirtyChecks;
            if (!DumpSave.active) {
                Scheduler->AddEventAfter(&DumpSave, CYCLES_PER_FRAME);
            }
            return;
        default:
            if constexpr(count) { (*timer) += AccessTiming<T, MemoryRegion::OOB>(); }
            return;
    }
}