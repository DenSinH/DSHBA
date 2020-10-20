#ifndef GC__DISASSEMBLY_VIEWER_H
#define GC__DISASSEMBLY_VIEWER_H

#include <stdint.h>
#include <inttypes.h>
#include "disassemble.h"

#include "imgui/imgui.h"

#define INSTRS_BEFORE_PC 20
#define INSTRS_AFTER_PC 20
#define DISASM_BUFER_SIZE 0x100


struct DisassemblyViewer
{
    uint32_t* PC;
    uint8_t* memory;
    uint32_t (*valid_address_check)(uint32_t address);

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

        if (!memory || !valid_address_check) {
            // if nullptr is passed to memory, we can't disassemble anything
            // so just don't even start on the window then
            return;
        }

        if (!ImGui::Begin("Disassembly Viewer", p_open))
        {
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
        if (!valid_address_check(address + (INSTRS_AFTER_PC << 2))) {
            count -= INSTRS_AFTER_PC;
        }

        if (!valid_address_check(address - (INSTRS_BEFORE_PC << 2))) {
            count -= INSTRS_BEFORE_PC;
        }
        else {
            address -= (INSTRS_BEFORE_PC << 2);
        }

#ifdef DO_CAPSTONE
        count = disassemble(
                &this->handle,
                this->memory + valid_address_check(address),
                count << 2,
                address,
                count,
                &this->insn
                );

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        for (int i = 0; i < count; i++) {
            if (insn[i].address == current_PC) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 100));
            }
            ImGui::Text("%08" PRIx32 ":\t%-10s\t%s", (uint32_t)insn[i].address, insn[i].mnemonic, insn[i].op_str);
            if (insn[i].address == current_PC) {
                ImGui::PopStyleColor(1);
            }
        }
        ImGui::PopStyleVar();

        free_disassembly(insn, count);
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
