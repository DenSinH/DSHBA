#ifndef GC__DISASSEMBLE_H
#define GC__DISASSEMBLE_H

#ifdef DO_CAPSTONE

#include <capstone/capstone.h>

#ifdef __cplusplus
extern "C" {
#endif
void init_disassembly(csh* handle);
void close_disassembly(csh* handle);
size_t disassemble(const csh* handle, uint8_t* code, size_t code_size, uint32_t address, uint32_t count, cs_insn** out);
void free_disassembly(cs_insn* out, size_t count);
#ifdef __cplusplus
};
#endif

#endif

#endif //GC__DISASSEMBLE_H
