#include <cstdio>
#include "imgui/imgui.h"

#include "menubar.h"

void ShowMenuBar(
        bool* console_enabled,
        bool* register_viewer_enabled,
        bool* disassembly_viewer_enabled,
        bool* memory_viewer_enabled,
        bool* overlay_enabled
        )
{
    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu("Debug"))
    {
        if (ImGui::MenuItem("Console", nullptr, console_enabled)) {}
        if (ImGui::MenuItem("Register Viewer", nullptr, register_viewer_enabled)) {}
        if (ImGui::MenuItem("Disassembly Viewer", nullptr, disassembly_viewer_enabled)) {}
        if (ImGui::MenuItem("Memory Viewer", nullptr, memory_viewer_enabled)) {}
        ImGui::Separator();
        if (ImGui::MenuItem("Overlay", nullptr, overlay_enabled)) {}
//            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}
