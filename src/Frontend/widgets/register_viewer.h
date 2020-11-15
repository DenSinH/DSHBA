#pragma once

#include "default.h"

#include <cstdint>
#include <cinttypes>

#include <SDL.h>
#include "imgui/imgui.h"

#define MAX_REGISTER_NAME_LENGTH 16
#define MAX_REGISTER_TAB_LENGTH 32

typedef struct s_register_data {
    char* name;
    const void* value;
    size_t size;
} s_register_data;


typedef struct s_register_tab {
    std::list<s_register_data> registers;
    char* name;
} s_register_tab;

typedef struct RegisterViewer
{
    std::list<s_register_tab> RegisterTabs;
    int columns;
    bool float_view;

    RegisterViewer()
    {
        this->columns = 8;
    }
    ~RegisterViewer()
    {

    }

    int AddRegisterTab(const char* name) {
        s_register_tab tab = {
                .name = new char[MAX_REGISTER_TAB_LENGTH]
        };
        STRCPY(tab.name, MAX_REGISTER_NAME_LENGTH, name);
        RegisterTabs.push_back(tab);
        return RegisterTabs.size() - 1;
    }

    void AddRegister(const char* reg, const void* value, size_t size, int tab) {
        s_register_data data = {
                .name = new char[MAX_REGISTER_NAME_LENGTH],
                .value = value,
                .size = size
        };

        STRCPY(data.name, MAX_REGISTER_NAME_LENGTH, reg);

        auto RegisterTab = RegisterTabs.begin();
        std::advance(RegisterTab, tab);
        RegisterTab->registers.push_back(data);
    }

    void Draw(bool* p_open)
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(-1, 0),    ImVec2(-1, FLT_MAX));
        ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_Once);

        if (!ImGui::Begin("Register Viewer", p_open))
        {
            ImGui::End();
            return;
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Close Register Viewer"))
                *p_open = false;
            ImGui::EndPopup();
        }

        char label[64];
        ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None);
        for (const auto& RegisterTab : RegisterTabs) {
            if (ImGui::BeginTabItem(RegisterTab.name)) {

                ImGui::Columns(columns, "Registers", true);  // 3-ways, no border
                int currentwidth = columns;
                size_t prev_size = 4;

                for (auto reg : RegisterTab.registers) {
                    if (!reg.value) {
                        ImGui::Columns(1);
                        ImGui::Columns(currentwidth);
                        ImGui::Separator();
                        continue;
                    }

                    if (prev_size != reg.size) {
                        switch (reg.size) {
                            case 1:
                                currentwidth = columns << 1;
                                break;
                            case 2:
                            case 4:
                                currentwidth = columns;
                                break;
                            case 8:
                                currentwidth = columns >> 1;
                                break;
                            default:
                                break;
                        }
                        ImGui::Columns(currentwidth);
                        prev_size = reg.size;
                    }

                    // name
                    sprintf(label, "%s", reg.name);
                    if (ImGui::Selectable(label)) {}
                    ImGui::NextColumn();

                    // value
                    switch (reg.size) {
                        case 1:
                            // no float mode for this one
                            sprintf(label, "%02x", *((uint8_t*)reg.value));
                            break;
                        case 2:
                            // no float mode for this one
                            sprintf(label, "%04x", *((uint16_t*)reg.value));
                            break;
                        case 4:
                            if (!float_view) sprintf(label, "%08x", *((uint32_t*)reg.value));
                            else sprintf(label, "%f", *((float*)reg.value));
                            break;
                        case 8:
                            if (!float_view) sprintf(label, "%016" PRIx64, *((uint64_t*)reg.value));
                            else sprintf(label, "%f", *((double*)reg.value));
                            break;
                        default:
                            break;
                    }
                    if (ImGui::Selectable(label)) {
                        SDL_SetClipboardText(label);
                    }
                    ImGui::NextColumn();
                }

                ImGui::Columns(1);
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();

        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Float view", nullptr, float_view)) float_view ^= true;
            if (p_open && ImGui::MenuItem("Close")) *p_open = false;
            ImGui::EndPopup();
        }

        ImGui::End();
    }
} RegisterViewer;