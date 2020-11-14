#include <cstdio>
#include "imgui/imgui.h"

#include "menubar.h"
#include "default.h"

#include <vector>
#include <list>

#define MAX_MENU_TAB_NAME_LENGTH 32
#define MAX_MENU_ITEM_LENGTH 32

typedef struct s_menu_item {
    char* name;
    bool* selected;
    MENU_ITEM_CALLBACK((*callback));
} s_register_data;


typedef struct s_menu_tab {
    std::list<s_menu_item> items;
    char* name;
} s_register_tab;

static std::list<s_menu_tab> tabs = {};

size_t add_menu_tab(char* name) {
    s_menu_tab tab = {
            .name = new char[MAX_MENU_TAB_NAME_LENGTH]
    };
    STRCPY(tab.name, MAX_MENU_TAB_NAME_LENGTH, name);
    tabs.push_back(tab);
    return tabs.size() - 1;
}


void add_menu_item(int tab, const char* name, bool* selected, MENU_ITEM_CALLBACK((*callback))) {
    s_menu_item data = {
            .name = new char[MAX_MENU_ITEM_LENGTH],
            .selected = selected,
            .callback = callback
    };

    STRCPY(data.name, MAX_MENU_ITEM_LENGTH, name);

    auto menu_tab = tabs.begin();
    std::advance(menu_tab, tab);
    menu_tab->items.push_back(data);
}


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
        if (ImGui::MenuItem("Mem Viewer", nullptr, memory_viewer_enabled)) {}
        ImGui::Separator();
        if (ImGui::MenuItem("Overlay", nullptr, overlay_enabled)) {}
        ImGui::EndMenu();
    }

    for (const auto& menu_tab : tabs) {
        if (ImGui::BeginMenu(menu_tab.name)) {
            for (auto item : menu_tab.items) {
                if (item.name) {
                    if (ImGui::MenuItem(item.name, nullptr, item.selected)) {
                        if (item.callback) {
                            item.callback(*item.selected);
                        }
                    }
                }
                else {
                    ImGui::Separator();
                }
            }
            ImGui::EndMenu();
        }
    }

    ImGui::EndMainMenuBar();
}
