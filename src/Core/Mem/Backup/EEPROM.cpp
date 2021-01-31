#include "EEPROM.h"

#include "log.h"

#include <fstream>
#include <assert.h>

EEPROM::EEPROM(u32 bus_width) {
    BusWidth = bus_width;
    memset(_storage, 0xff, sizeof(_storage));
}

void EEPROM::Load(const std::string &file_name) {
    std::ifstream file(file_name, std::ios::binary);

    if (file.is_open()) {
        file.read(reinterpret_cast<char *>(_storage), sizeof(_storage));
        file.close();
    }
    else {
        // don't warn, file might not have been created for this game
        // banks have been filled with 1s already on construction
    }
}

void EEPROM::Dump(const std::string& file_name) {
    std::ofstream file(file_name, std::ios::trunc | std::ios::binary);

    if (file.is_open()) {
        file.write(reinterpret_cast<const char *>(_storage), sizeof(_storage));
        file.close();
    }
    else {
        log_warn("Opening save file failed!");
    }
}

u8 EEPROM::Read(u32 address) {
    if (Access == AccessType::Read) {
        /*
         *   GBATek:
         * Read a stream of 68 bits from EEPROM by using DMA,
         * then decipher the received data as follows:
         *   4 bits - ignore these
         *   64 bits - data (conventionally MSB first)
         */
        ReadBitCounter--;
        if (ReadBitCounter > 63)
        {
            return 1;  // ignore these
        }

        // MSB first (BitCounter == 7 (mod 8) when we first get here)
        u8 value = (u8)((_storage[ReadAddress] >> (ReadBitCounter & 7)) & 1);
        log_mem("Read Access %x, got %x from %x (%x)", ReadBitCounter, value, ReadAddress, _storage[ReadAddress]);

        if ((ReadBitCounter & 7) == 0)
        {
            ReadAddress++;  // move to next byte if BitCounter == 0 mod 8

            if (ReadBitCounter == 0)
            {
                // read done
                ReadBitCounter = ReadBitCounterReset;
            }
        }

        return value;
    }

    log_mem("EEPROM read in write access mode");
    /* for both write and invalid accesses
     * GBATek: After the DMA, keep reading from the chip,
     *         by normal LDRH [DFFFF00h], until Bit 0 of the returned data becomes "1" (Ready).
     */
    return 1;
}

void EEPROM::Write(u32 address, u8 value) {
    log_mem("EEPROM write %x to %x", value, address);
#ifndef NDEBUG
    if (!BusWidth) {
        log_warn("EEPROM bus width not set on first write");
    }
#endif

    Buffer <<= 1;
    Buffer |= value & 1;
    WriteBitCounter++;

    if (WriteBitCounter == 2)
    {
        // command write complete
        Access = static_cast<AccessType>(Buffer & 3);
        log_mem("Set EEPROM access type to %s", Access == AccessType::Read ? "Read" : "Write");
    }
    else if (WriteBitCounter == 2 + BusWidth)
    {
        /* address write complete
         * writes happen in units of 8 bytes at a time, so we must account for this in the address
         */
        if (Access == AccessType::Read)
        {
            ReadAddress = (Buffer << 3) & (Size - 1);
            log_mem("Set EEPROM read address to %x", ReadAddress);
        }
        else
        {
            WriteAddress = (Buffer << 3) & (Size - 1);
            log_mem("Set EEPROM write address to %x", WriteAddress);
        }
    }
    else if (WriteBitCounter > 2 + BusWidth)
    {
        if (Access == AccessType::Write)
        {
            // first time we get here, WriteBitCounter will be 63 (== 7 mod 8) and counting down
            int WriteBitCount = (64 + (int)BusWidth + 2 - (int)this->WriteBitCounter);
            if (WriteBitCount < 0)
            {
                // last bit (expect 0)
                // reset buffer values
                Buffer = 0;  // this doesn't actually matter, but just to be sure
                WriteBitCounter = 0;
                return;
            }

            if ((WriteBitCount & 7) == 0)  // 0 mod 8, proceed to next byte
            {
                // we might as well just write one bit at a time, we have the buffer after all
                _storage[WriteAddress++] = (u8)Buffer;
                return;
            }
        }
        else if (Access == AccessType::Read)
        {
            // expect 0 as final bit write for Read, I don't really care about this
            // ready counter values
            ReadBitCounter = ReadBitCounterReset;
            WriteBitCounter = 0;
            Buffer = 0;
        }
        else
        {
            log_warn(
                    "Something went wrong, WriteBitCounter overflow (%x) in invalid mode %s, resetting...",
                    WriteBitCounter,
                    Access == AccessType::Read ? "Read" : "Write"
            );
            WriteBitCounter = 0;
        }
    }
}