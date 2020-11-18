#include <cstdio>
#include "imgui/imgui.h"

#include "menubar.h"
#include "default.h"

#include <vector>
#include <list>

#define MAX_MENU_TAB_NAME_LENGTH 32
#define MAX_MENU_ITEM_LENGTH 32


struct MenuItem {
    virtual void Draw() {}
};


struct BoolItem : MenuItem {
    BoolItem(const char* name, bool* selected, MENU_ITEM_CALLBACK((*callback))) {
        this->name = new char[MAX_MENU_ITEM_LENGTH];
        STRCPY(this->name, MAX_MENU_ITEM_LENGTH, name);

        this->selected = selected;
        this->callback = callback;
    }

    char* name;
    bool* selected;
    MENU_ITEM_CALLBACK((*callback));

    void Draw() override {
        if (name[0]) {
            if (ImGui::MenuItem(name, nullptr, selected)) {
                if (callback) {
                    callback(selected == nullptr || *selected);
                }
            }
        }
        else {
            ImGui::Separator();
        }
    }
};

struct MenuSliderf : MenuItem {

    MenuSliderf(const char* name, float* value, float min, float max) {
        this->name = new char[MAX_MENU_ITEM_LENGTH];
        STRCPY(this->name, MAX_MENU_ITEM_LENGTH, name);

        this->value = value;
        this->min = min;
        this->max = max;
    }

    char* name;
    float* value;
    float min, max;

    void Draw() override {
        ImGui::SliderFloat(name, value, min, max);
    }
};

struct SubMenu : MenuItem {

    explicit SubMenu(const char* name) {
        this->name = new char[MAX_MENU_ITEM_LENGTH];
        STRCPY(this->name, MAX_MENU_ITEM_LENGTH, name);
    }

    char* name;
    std::list<MenuItem*> items= {};

    void Draw() override {
        if (ImGui::BeginMenu(name)) {
            for (auto& item : items) {
                item->Draw();
            }
            ImGui::EndMenu();
        }
    }

};

struct MenuTab {
    std::list<MenuItem*> items;
    char* name;
};

static std::list<MenuTab> tabs = {};

size_t add_menu_tab(char* name) {
    MenuTab tab = {
            .name = new char[MAX_MENU_TAB_NAME_LENGTH]
    };
    STRCPY(tab.name, MAX_MENU_TAB_NAME_LENGTH, name);
    tabs.push_back(tab);
    return tabs.size() - 1;
}


void add_menu_item(int tab, const char* name, bool* selected, MENU_ITEM_CALLBACK((*callback))) {
    auto* data = new BoolItem(
            name,
            selected,
            callback
    );

    auto menu_tab = tabs.begin();
    std::advance(menu_tab, tab);
    menu_tab->items.push_back(data);
}

void add_sliderf(int tab, const char* name, float* value, float min, float max) {
    auto* data = new MenuSliderf(
            name,
            value,
            min,
            max
    );

    auto menu_tab = tabs.begin();
    std::advance(menu_tab, tab);
    menu_tab->items.push_back(data);
}

size_t add_submenu(int tab, const char* name) {
    auto* menu = new SubMenu(name);

    auto menu_tab = tabs.begin();
    std::advance(menu_tab, tab);
    menu_tab->items.push_back(menu);
    return menu_tab->items.size() - 1;
}

SubMenu* get_submenu(int tab, int sub_menu) {
    auto menu_tab = tabs.begin();
    std::advance(menu_tab, tab);

    auto submenu = menu_tab->items.begin();
    std::advance(submenu, sub_menu);

    return (SubMenu*)*submenu;
}

void add_submenu_item(int tab, int sub_menu, const char* name, bool* selected, MENU_ITEM_CALLBACK((*callback))) {
    SubMenu* submenu = get_submenu(tab, sub_menu);

    submenu->items.push_back(new BoolItem(
            name,
            selected,
            callback
    ));
}

void add_submenu_sliderf(int tab, int sub_menu, const char* name, float* value, float min, float max) {
    SubMenu* submenu = get_submenu(tab, sub_menu);

    submenu->items.push_back(new MenuSliderf(
            name,
            value,
            min,
            max
    ));
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
                item->Draw();
            }
            ImGui::EndMenu();
        }
    }

    ImGui::EndMainMenuBar();
}
