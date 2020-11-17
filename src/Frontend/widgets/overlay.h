#ifndef GC__OVERLAY_H
#define GC__OVERLAY_H

#include <list>
#include "imgui/imgui.h"

#include "../interface.h"

#define MAX_OVERLAY_INFO_LENGTH 50

typedef struct s_overlay_info {
    OVERLAY_INFO((*getter));
} s_overlay_info;

struct Overlay
{
    ImGuiIO* io;
    std::list<s_overlay_info> info;
    char gfx_info[0x200];
    char buffer[MAX_OVERLAY_INFO_LENGTH];

    Overlay()
    {

    }
    ~Overlay()
    {

    }

    void AddInfo(OVERLAY_INFO((*getter))) {
        auto overlay_info = (s_overlay_info) { .getter = getter };

        this->info.push_back(overlay_info);
    }

    void Draw(bool* p_open)
    {
        const float Y_DISTANCE = 20.0f;
        const float X_DISTANCE = 10.0f;

        ImVec2 window_pos = ImVec2(X_DISTANCE, Y_DISTANCE);
        ImVec2 window_pos_pivot = ImVec2(0.0f, 0.0f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

        ImGui::SetNextWindowBgAlpha(0.50f); // Transparent background
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

        window_flags |= ImGuiWindowFlags_NoMove;
        if (ImGui::Begin("Example: Simple overlay", p_open, window_flags))
        {
            ImGui::Text("[INFO]\n");
            ImGui::Separator();

            for (s_overlay_info info_iter : this->info) {
                info_iter.getter(this->buffer, MAX_OVERLAY_INFO_LENGTH, io->DeltaTime);
                ImGui::Text("%s\n", buffer);
            }

            ImGui::Separator();
            ImGui::Text("%s\n", gfx_info);
            ImGui::Separator();
        }
        ImGui::End();
    }
};


#endif //GC__OVERLAY_H
