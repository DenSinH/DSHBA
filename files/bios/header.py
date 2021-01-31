with open("bios.bin", "rb") as f:
    code = f.read()

header = ""
for i in range(len(code) >> 4):
    line = "    "
    for j in range(16):
        line += "0x%02x, " % code[16 * i + j]
    header += line + "\n"

header = f"""#ifndef DSHBA_BIOS_H
#define DSHBA_BIOS_H

// GBA replacement BIOS made by me and Fleroviux

static constexpr const unsigned char BIOS[] = {{\n{
    header
}}};

#endif // DSHBA_BIOS_H
"""

with open("BIOS.h", "w+") as f:
    f.write(header)