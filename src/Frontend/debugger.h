#pragma once

#include <glad/glad.h>
#include <SDL.h>

#undef SHOW_EXAMPLE_MENU

void debugger_init(
        uint32_t* PC,
        uint64_t mem_size,
        uint8_t* (*valid_address_mask)(uint32_t),
        uint8_t (*mem_read)(uint64_t off)
);

void debugger_video_init(const char* glsl_version, SDL_Window* window, SDL_GLContext* gl_context);
void debugger_render();