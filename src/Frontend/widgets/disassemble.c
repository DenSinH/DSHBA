#include "disassemble.h"

#ifdef DO_CAPSTONE

void init_disassembly(csh* handle) {
    cs_err open = cs_open(CS_ARCH_PPC, CS_MODE_32 | CS_MODE_BIG_ENDIAN, handle);
    if (open != CS_ERR_OK)
        exit(open);

    cs_opt_skipdata skipdata = {
            .mnemonic = "????",
    };

    cs_option(*handle, CS_OPT_DETAIL, CS_OPT_ON);
    cs_option(*handle, CS_OPT_SKIPDATA, CS_OPT_ON);
    cs_option(*handle, CS_OPT_SKIPDATA_SETUP, (size_t)&skipdata);
}

void close_disassembly(csh* handle) {
    cs_close(handle);
}

size_t disassemble(const csh* handle, uint8_t* code, size_t code_size, uint32_t address, uint32_t count, cs_insn** out) {
    return cs_disasm(*handle, code, code_size, address, count, out);
}

void free_disassembly(cs_insn* out, size_t count) {
    cs_free(out, count);
}

#endif