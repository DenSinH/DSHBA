
template<>
inline u32 MMIO::Read<u32>(u32 address) {
    // 32 bit writes are a little different
    if (unlikely(ReadPrecall[address >> 1])) {
        WriteArray<u16>(Registers, address, (this->*ReadPrecall[address >> 1])());
    }
    if (unlikely(ReadPrecall[1 + (address >> 1)])) {
        WriteArray<u16>(Registers, address + 2, (this->*ReadPrecall[1 + (address >> 1)])());
    }

    return ReadArray<u32>(Registers, address);
}

template<typename T>
T MMIO::Read(u32 address) {
    if (unlikely(ReadPrecall[address >> 1])) {
        WriteArray<u16>(Registers, address, (this->*ReadPrecall[address >> 1])());
    }
    return ReadArray<T>(Registers, address);
}

template<>
inline void MMIO::Write<u32>(u32 address, u32 value) {
    // 32 bit writes are a little different
    // Remember Little Endianness!
    WriteArray<u16>(Registers, address, value & WriteMask[address >> 1]);
    WriteArray<u16>(Registers, address + 2, (value >> 16) & WriteMask[(address >> 1) + 1]);

    if (unlikely(WriteCallback[address >> 1])) {
        (this->*WriteCallback[address >> 1])(value);
    }
    if (unlikely(WriteCallback[1 + (address >> 1)])) {
        (this->*WriteCallback[1 + (address >> 1)])(value >> 16);
    }
}

template<>
inline void MMIO::Write<u16>(u32 address, u16 value) {
    WriteArray<u16>(Registers, address, value & WriteMask[address >> 1]);

    if (unlikely(WriteCallback[address >> 1])) {
        (this->*WriteCallback[address >> 1])(value);
    }
}

template<>
inline void MMIO::Write<u8>(u32 address, u8 value) {
    if (address & 1) {
        WriteArray<u8>(Registers, address, value & WriteMask[address >> 1]);
    }
    else {
        WriteArray<u8>(Registers, address, value & (WriteMask[address >> 1] >> 8));
    }

    if (unlikely(WriteCallback[address >> 1])) {
        // We have to be careful with this:
        (this->*WriteCallback[address >> 1])(ReadArray<u16>(Registers, address));
    }
}