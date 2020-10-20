#pragma once

#include <glad/glad.h>
#include <SDL.h>

#undef SHOW_EXAMPLE_MENU

void debugger_init(
        uint32_t* PC,
        uint8_t* memory,
        uint64_t mem_size,
        uint32_t (*valid_address_mask)(uint32_t),
        uint8_t (*mem_read)(const uint8_t* data, uint64_t off)
);

void debugger_video_init(const char* glsl_version, SDL_Window* window, SDL_GLContext* gl_context);
void debugger_render();