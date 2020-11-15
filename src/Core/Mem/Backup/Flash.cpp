#include "Flash.h"

#include <fstream>

#include "log.h"

Flash::Flash(bool megabit) {
    Megabit = megabit;
    // don't worry, nothing has been loaded yet
    EraseAll();
}

void Flash::Load(const std::string &file_name) {
    std::ifstream file(file_name, std::ios::binary);

    if (file.is_open()) {
        file.read(reinterpret_cast<char *>(_banks), sizeof(_banks));
        file.close();
    }
    else {
        // don't warn, file might not have been created for this game
        // banks have been filled with 1s already on construction
    }
}

void Flash::Dump(const std::string& file_name) {
    std::ofstream file(file_name, std::ios::trunc | std::ios::binary);

    if (file.is_open()) {
        file.write(reinterpret_cast<const char *>(_banks), sizeof(_banks));
        file.close();
    }
    else {
        log_warn("Opening save file failed!");
    }
}

u8 Flash::Read(u32 address) {
    u32 masked_address = address & 0xffff;

    if (IDMode && (masked_address < 2)) {
        if (masked_address == 0) {
            return SanyoManufacturerID;
        }
        return SanyoDeviceID;
    }
    return _banks[Bank][masked_address & 0xffff];
}

void Flash::Write(u32 address, u8 value) {
    u32 masked_address = address & 0xffff;

    switch (Expect) {
        case ExpectState::Byte:
            _banks[Bank][masked_address] = value;
            Expect = ExpectState::Nothing;
            return;
        case ExpectState::BankSwitch:
            if (Megabit) {
                // small flash doesn't do bank switching
                if (masked_address == 0) {
                    Bank = value & 1;
                }
                else {
                    log_warn("Expected bank switch, got write %02x to %x", value, masked_address);
                }
            }
            Expect = ExpectState::Nothing;
            return;
        default:
            break;
    }

    switch (State) {
        case WriteState::CommandStart1:
            // expect 0xAA to 0x5555
            if (masked_address == 0x5555 && value == 0xaa) {
                State = WriteState::CommandStart2;
            }
            else if (masked_address == 0x5555 && value == 0xf0) {
                // cancel command
            }
            else {
                log_warn("Expected 0xaa to 0x5555 got 0x%x to 0x%x", value, masked_address);
            }
            return;
        case WriteState::CommandStart2:
            // expect 0x55 to 0x2aaa
            if (masked_address == 0x2aaa && value == 0x55) {
                State = WriteState::Command;
            }
            else {
                log_warn("Expected 0xaa to 0x5555 got 0x%x to 0x%x", value, masked_address);
                // try to salvage the situation by returning to writestate 1
                State = WriteState::CommandStart1;
            }
            return;
        case WriteState::Command:
            // log_debug("Got flash command %x", value);
            State = WriteState::CommandStart1;
            switch (value)
            {
                case 0x90:  // Enter "Chip identification mode"
                    IDMode = true;
                    return;
                case 0xf0:  // Exit "Chip identification mode"
                    IDMode = false;
                    return;
                case 0x80:  // Prepare to receive erase command
                    Expect = ExpectState::Erase;
                    return;
                case 0x10:  // Erase entire chip
                    if (Expect == ExpectState::Erase)
                    {
                        EraseAll();
                        Expect = ExpectState::Nothing;
                        return;
                    }
                    else
                    {
                        log_warn("Flash erase command not preceded by prepare erase, command ignored");
                        return;
                    }
                case 0x30:  // Erase 4kB sector
                    if (Expect == ExpectState::Erase)
                    {
                        // Erase 4kB flash at address
                        Erase(masked_address, 0x1000);
                        Expect = ExpectState::Nothing;
                        return;
                    }
                    else
                    {
                        log_warn("Flash erase command not preceded by prepare erase, command ignored");
                        return;
                    }
                case 0xa0:  // Prepare to write single data byte
                    Expect = ExpectState::Byte;
                    return;
                case 0xb0:  // Bank switch
                    // only works for 128 kb flash devices
                    Expect = ExpectState::BankSwitch;
                    return;
                default:
                    log_warn("Invalid Flash command: %02x", value);
                    return;
            }
    }
}
