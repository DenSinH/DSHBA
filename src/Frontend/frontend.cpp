#include "frontend.h"

#include "debugger.h"
#include "interface.h"
#include "controller.h"
#include "settings.h"

#include "widgets/file_dialog.h"

#include <cstdio>
#include <SDL.h>

const unsigned WINDOW_WIDTH = 1280;
const unsigned WINDOW_HEIGHT = 720;
#define FPS 60
#define DELTA_TIME (1000 / FPS)

#define FILE_BROWSER_PWD_SETTING "PWD"

static struct s_frontend {
    ImGui::FileBrowser file_dialog = ImGui::FileBrowser(ImGuiFileBrowserFlags_CloseOnEsc);

    ImGuiIO io;
    s_controller controller;
    void (*parse_input)(s_controller* controller);

    void (*shutdown)();
    void (*video_init)();
    void (*video_destroy)();
    s_framebuffer (*render)(size_t render_until);

    void (*audio_init)();
    void (*audio_destroy)();
} Frontend;

ImGuiIO *frontend_set_io() {
    Frontend.io = ImGui::GetIO();
    return &Frontend.io;
}

#ifdef __cplusplus
extern "C" {
#endif

void frontend_init(
        void (*shutdown)(),
        uint32_t *PC,
        uint64_t mem_size,
        uint8_t *(*valid_address_mask)(uint32_t),
        uint8_t (*mem_read)(uint64_t),
        void (*parse_input)(s_controller*),
        bool (*arm_mode)()
) {
    Frontend.shutdown = shutdown;
    Frontend.parse_input = parse_input;
    debugger_init(
            PC, mem_size, valid_address_mask, mem_read, arm_mode
    );
}

void bind_video_init(void (*initializer)()) {
    Frontend.video_init = initializer;
}

void bind_video_render(s_framebuffer (*render)(size_t)) {
    Frontend.render = render;
}

void bind_video_destroy(void (*destroy)()) {
    Frontend.video_destroy = destroy;
}

void bind_audio_init(void (*initializer)()) {
    Frontend.audio_init = initializer;
}

void bind_audio_destroy(void (*destroy)()) {
    Frontend.audio_destroy = destroy;
}

void open_file_explorer(const char* title, char** filters, size_t filter_count, FILE_SELECT_CALLBACK((*callback))) {

    std::vector<const char*> _filters;
    _filters.reserve(filter_count);
    for (size_t i = 0; i < filter_count; i++) {
        _filters.push_back(filters[i]);
    }

    if (!Frontend.file_dialog.IsOpened()) {
        Frontend.file_dialog.callback = callback;
        Frontend.file_dialog.SetTitle(title);
        Frontend.file_dialog.SetTypeFilters(_filters);
        Frontend.file_dialog.Open();
    }
}

SDL_GameController* gamecontroller = nullptr;

void init_gamecontroller() {
    if (SDL_NumJoysticks() < 0) {
        printf("No gamepads detected\n");
    }
    else {
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
                gamecontroller = SDL_GameControllerOpen(i);

                if (!gamecontroller) {
                    printf("Failed to connect to gamecontroller at index %d\n", i);
                    continue;
                }

                printf("Connected game controller at index %d\n", i);
                return;
            }
        }
    }
    printf("No gamepads detected (only joysticks)\n");
}

int ui_run() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
    init_controller_mapping(CONTROLLER_MAP_FILE);
    init_gamecontroller();

    // Decide GL+GLSL versions
#if __APPLE__
    const char* glsl_version = "#version 400";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    const char *glsl_version = "#version 400";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    auto window_flags = (SDL_WindowFlags) (SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | // SDL_WINDOW_RESIZABLE |
                                           SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window = SDL_CreateWindow("DSHBA", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                                          window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // vsync

    debugger_video_init(glsl_version, window, &gl_context);
    if (Frontend.video_init) {
        Frontend.video_init();
    }
    else {
        printf("No frontend initializer function bound\n");
    }

    if (Frontend.audio_init) {
        Frontend.audio_init();
    }


    Settings settings = Settings();

    if (settings.Has(FILE_BROWSER_PWD_SETTING)) {
        std::string pwd = settings.Get(FILE_BROWSER_PWD_SETTING);
        Frontend.file_dialog.SetPwd(pwd);
    }

    printf("Done initializing frontend\n");

    // Main loop
    bool shutdown = false;
    while (!shutdown) {
        uint32_t frame_ticks = SDL_GetTicks();

        // Get events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch (event.type) {
                case SDL_QUIT:
                    shutdown = true;
                    if (Frontend.shutdown) {
                        Frontend.shutdown();
                    }
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE &&
                        event.window.windowID == SDL_GetWindowID(window)) {
                        shutdown = true;
                        if (Frontend.shutdown) {
                            Frontend.shutdown();
                        }
                    }
                    break;
                case SDL_CONTROLLERDEVICEADDED:
                    gamecontroller =  SDL_GameControllerOpen(event.cdevice.which);
                    if (gamecontroller == nullptr) {
                        printf("Error with connected gamecontroller\n");
                    }
                    break;
                case SDL_KEYDOWN:
                    // printf("pressed keyboard key %d\n", event.key.keysym.sym);
                    parse_keyboard_input(&Frontend.controller, event.key.keysym.sym, 1);
                    break;
                case SDL_KEYUP:
                    parse_keyboard_input(&Frontend.controller, event.key.keysym.sym, 0);
                    break;
                case SDL_CONTROLLERBUTTONDOWN:
                    // printf("pressed button %d\n", event.cbutton.button);
                    parse_controller_button(&Frontend.controller, event.cbutton.button, 1);
                    break;
                case SDL_CONTROLLERBUTTONUP:
                    parse_controller_button(&Frontend.controller, event.cbutton.button, 0);
                    break;
                case SDL_CONTROLLERAXISMOTION:
                    // printf("moved axis %d, %d\n", event.caxis.axis, event.caxis.value);
                    parse_controller_axis(&Frontend.controller, event.caxis.axis, event.caxis.value);
                    break;
                default:
                    break;
            }
        }

        if (Frontend.parse_input) {
            Frontend.parse_input(&Frontend.controller);
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        debugger_render();
        // ImGui::ShowDemoWindow();

        Frontend.file_dialog.Handle();

        // Rendering
        ImGui::Render();

#ifdef SHOW_EXAMPLE_MENU
        ImGui::ShowDemoWindow(NULL);
#endif
        glDisable(GL_SCISSOR_TEST);
        // render actual emulation
        s_framebuffer emu_framebuffer = {
                .id = 0,
        };

        if (Frontend.render) {
            emu_framebuffer = Frontend.render(DELTA_TIME + frame_ticks);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(emu_framebuffer.r, emu_framebuffer.g, emu_framebuffer.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        float scale = (float) WINDOW_WIDTH / emu_framebuffer.dest_width;
        if ((float) WINDOW_HEIGHT / emu_framebuffer.dest_height < scale) {
            scale = (float) WINDOW_HEIGHT / emu_framebuffer.dest_height;
        }

        auto offsx = (unsigned)((WINDOW_WIDTH - scale * emu_framebuffer.dest_width) / 2);
        auto offsy = (unsigned)((WINDOW_HEIGHT - scale * emu_framebuffer.dest_height) / 2);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, emu_framebuffer.id);
        glBlitFramebuffer(0, 0,
                          emu_framebuffer.src_width, emu_framebuffer.src_height,
                          offsx, offsy,
                          offsx + scale * emu_framebuffer.dest_width, offsy + scale * emu_framebuffer.dest_height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST
        );

        // then draw the imGui stuff over it
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // frameswap
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    settings.Set(FILE_BROWSER_PWD_SETTING, Frontend.file_dialog.GetPwd().string());
    settings.Dump();

    if (Frontend.video_destroy) {
        Frontend.video_destroy();
    }

    if (Frontend.audio_destroy) {
        Frontend.audio_destroy();
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("Frontend destroyed\n");
    return 0;
}

#ifdef __cplusplus
}
#endif