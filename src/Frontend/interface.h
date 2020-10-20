#ifndef GC__INTERFACE_H
#define GC__INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

#define CONSOLE_COMMAND(name) void name(char** args, int argc, char* output)
#define OVERLAY_INFO(name) void name(char* output, size_t output_length, float delta_time)
#define MAX_OUTPUT_LENGTH 0x100
#define CONTROLLER_MAP_FILE "input.map"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct s_framebuffer {
        unsigned int id;
        void (*draw_overlay)(void* caller, const float x, const float y, const float width, const float height);
        void* caller;
        size_t src_width, src_height;
        size_t dest_width, dest_height;
    } s_framebuffer;

    typedef struct s_controller {
        // u8 for compatibility with C
        uint8_t A, B, X, Y, up, down, left, right, start, select, L, R;
        int16_t left_x, left_y, right_x, right_y;
    } s_controller;

    int ui_run(void);

    void frontend_init(
            bool* shutdown,
            uint32_t* PC,
            uint8_t* memory,
            uint64_t mem_size,
            uint32_t (*valid_address_mask)(uint32_t),
            uint8_t (*mem_read)(const uint8_t* data, uint64_t off),
            void (*parse_input)(s_controller* controller)
    );

    void add_command(const char* command, const char* description, CONSOLE_COMMAND((*callback)));
    void add_overlay_info(OVERLAY_INFO((*getter)));
    int add_register_tab(const char* name);
    void add_register_data(char* name, const void* value, size_t size, int tab);

    void bind_video_init(void (*initializer)(void));
    void bind_video_render(s_framebuffer (*render)(uint32_t time_left));
    void bind_video_destroy(void (*destroy)(void));
#ifdef __cplusplus
}
#endif

#endif //GC__INTERFACE_H
