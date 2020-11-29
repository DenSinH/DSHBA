#ifndef GC__INTERFACE_H
#define GC__INTERFACE_H

#include "default.h"

#include <stdbool.h>
#include <stdint.h>

#define CONSOLE_COMMAND(name) void name(char** args, int argc, char* output)
#define OVERLAY_INFO(name) void name(char* output, size_t output_length, float delta_time)
#define MENU_ITEM_CALLBACK(name) void name(bool selected)
#define FILE_SELECT_CALLBACK(name) void name(const char* file_name)
#define MAX_OUTPUT_LENGTH 0x100
#define CONTROLLER_MAP_FILE "input.map"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct s_framebuffer {
        unsigned int id;
        size_t src_width, src_height;
        size_t dest_width, dest_height;
        float r, g, b;
    } s_framebuffer;

    typedef struct s_controller {
        // u8 for compatibility with C
        uint8_t A, B, X, Y, up, down, left, right, start, select, L, R;
        int16_t left_x, left_y, right_x, right_y;
    } s_controller;

    int ui_run(void);

    void frontend_init(
            void (*shutdown)(),
            uint32_t* PC,
            uint64_t mem_size,
            uint8_t* (*valid_address_mask)(uint32_t),
            uint8_t (*mem_read)(uint64_t off),
            void (*parse_input)(s_controller* controller),
            bool (*arm_mode)()
    );

    void add_command(const char* command, const char* description, CONSOLE_COMMAND((*callback)));
    void add_overlay_info(OVERLAY_INFO((*getter)));
    int add_register_tab(const char* name);
    void add_register_data(const char* name, const void* value, size_t size, int tab);

    size_t add_menu_tab(char* name);

    size_t add_submenu(int tab, const char* name);
    void add_submenu_item(int tab, int sub_menu, const char* name, bool* selected, MENU_ITEM_CALLBACK((*callback)));
    void add_submenu_sliderf(int tab, int sub_menu, const char* name, float* value, float min, float max);

    void add_menu_item(int tab, const char* name, bool* selected, MENU_ITEM_CALLBACK((*callback)));
    void add_sliderf(int tab, const char* name, float* value, float min, float max);
    void open_file_explorer(const char* title, char** filters, size_t filter_count, FILE_SELECT_CALLBACK((*callback)));

    void bind_video_init(void (*initializer)());
    void bind_video_render(s_framebuffer (*render)(size_t));
    void bind_video_blit(void (*blit)(const float* data));
    void bind_video_destroy(void (*destroy)());

    void bind_audio_init(void (*initializer)());
    void bind_audio_destroy(void (*destroy)());
#ifdef __cplusplus
}
#endif

#endif //GC__INTERFACE_H
