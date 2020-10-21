#ifndef GC__DISASSEMBLY_VIEWER_H
#define GC__DISASSEMBLY_VIEWER_H

#include <stdint.h>
#include <inttypes.h>
#include <cstdlib>
#include "disassemble.h"

#include "imgui/imgui.h"

#define INSTRS_BEFORE_PC 20
#define INSTRS_AFTER_PC 20
#define DISASM_BUFER_SIZE 0x100


struct DisassemblyViewer
{
    uint32_t* PC;
    uint8_t* (*valid_address)(uint32_t address);
    bool (*arm_mode)();

#ifdef DO_CAPSTONE
    csh handle;
    cs_insn* insn;
    char buffer[DISASM_BUFER_SIZE];

    DisassemblyViewer()
    {
        init_disassembly(&handle);
    }
    ~DisassemblyViewer()
    {
        close_disassembly(&handle);
    }
#endif

    void Draw(bool* p_open)
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(-1, -1),    ImVec2(-1, -1));
        ImGui::SetNextWindowSize(ImVec2(400, (INSTRS_BEFORE_PC + INSTRS_AFTER_PC + 1) * 14), ImGuiCond_Once);

        if (!ImGui::Begin("Disassembly Viewer", p_open))
        {
            ImGui::End();
            return;
        }

        if (!valid_address) {
            // if nullptr is passed to memory, we can't disassemble anything
            // so just don't even start on the window then
            ImGui::End();
            return;
        }

        // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
        // So e.g. IsItemHovered() will return true when hovering the title bar.
        // Here we create a context menu only available from the title bar.
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Close Disassembly Viewer"))
                *p_open = false;
            ImGui::EndPopup();
        }

        ImGui::BeginChild("Disassembly");

        uint32_t address = *this->PC;
        uint32_t current_PC = address;

        size_t count = INSTRS_BEFORE_PC + INSTRS_AFTER_PC + 1;
        if (!valid_address(address + (INSTRS_AFTER_PC << 2))) {
            count -= INSTRS_AFTER_PC;
        }

        if (!valid_address(address - (INSTRS_BEFORE_PC << 2))) {
            count -= INSTRS_BEFORE_PC;
        }
        else {
            address -= (INSTRS_BEFORE_PC << 2);
        }

#ifdef DO_CAPSTONE

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        for (int i = 0; i < count; i++) {
            uint8_t* loc = valid_address(address);
            if (!loc) {
                address += 4;
            }

            unsigned int instructions = disassemble(
                    &this->handle,
                    loc,
                    4,
                    address,
                    0,
                    &this->insn,
                    arm_mode == nullptr || arm_mode()
            );

            address += 4;

            // THUMB mode is severely annoying
            if (instructions == 1 ||
                std::abs(int(address - current_PC)) < INSTRS_BEFORE_PC * 2) {

                for (int j = 0; j < instructions; j++) {
                    if (insn[j].address == current_PC) {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 100));
                    }
                    if (instructions == 1) {
                        ImGui::Text("%08" PRIx32 ":\t%08x\t%-10s\t%s",
                                    (uint32_t)insn[j].address, *(uint32_t*)insn[j].bytes, insn[j].mnemonic, insn[j].op_str);
                    }
                    else {
                        ImGui::Text("%08" PRIx32 ":\t%04x    \t%-10s\t%s",
                                    (uint32_t)insn[j].address, *(uint16_t*)insn[j].bytes, insn[j].mnemonic, insn[j].op_str);
                    }
                    if (insn[j].address == current_PC) {
                        ImGui::PopStyleColor(1);
                    }
                }
            }

            free_disassembly(insn, instructions);
        }
        ImGui::PopStyleVar();
#else
        for (int i = 0; i < count; i++) {
            ImGui::Text("Disassembly unsupported");
        }
#endif
        ImGui::EndChild();
        ImGui::End();
    }
};

#endif //GC__DISASSEMBLY_VIEWER_H
