#include "debugger.h"
#include "frontend.h"
#include "interface.h"

#include "widgets/menubar.h"
#include "widgets/overlay.h"
#include "widgets/console.h"
#include "widgets/register_viewer.h"
#include "widgets/disassembly_viewer.h"
#include "widgets/memory_viewer.h"

static bool show_console = true;
static bool show_register_viewer = true;
static bool show_disassembly_viewer = true;
static bool show_overlay = false;
static bool show_memory_viewer = true;

// Our state
static struct s_debugger {
    ConsoleWidget console;
    RegisterViewer register_viewer;
    DisassemblyViewer disassembly_viewer;
    Overlay overlay;
    MemoryViewer memory_viewer;
} Debugger;

void add_command(const char* command, const char* description, CONSOLE_COMMAND((*callback))) {
    Debugger.console.AddCommand(s_console_command {
            .command = command,
            .description = description,
            .callback = callback
    });
}


void add_overlay_info(OVERLAY_INFO((*getter))) {
    Debugger.overlay.AddInfo(getter);
}

int add_register_tab(const char* name){
    return Debugger.register_viewer.AddRegisterTab(name);
}

void add_register_data(char* name, const void* value, size_t size, int tab) {
    Debugger.register_viewer.AddRegister(name, value, size, tab);
}

void debugger_init(
        uint32_t* PC,
        uint8_t* memory,
        uint64_t mem_size,
        uint32_t (*valid_address_mask)(uint32_t),
        uint8_t (*mem_read)(const uint8_t* data, uint64_t off)
) {
    Debugger.disassembly_viewer.PC = PC;
    Debugger.disassembly_viewer.memory = memory;
    Debugger.disassembly_viewer.valid_address_check = valid_address_mask;
    Debugger.memory_viewer.mem_data = memory;
    Debugger.memory_viewer.mem_size = mem_size;
    Debugger.memory_viewer.ReadFn = mem_read;
}


void debugger_video_init(const char* glsl_version, SDL_Window* window, SDL_GLContext* gl_context) {
// Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    Debugger.overlay.io = frontend_set_io();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Initialize OpenGL loader
    bool err = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress) == 0;
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        exit(1);
    }

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // report init info
    sprintf(Debugger.overlay.gfx_info,
            "OpenGL Info\n"
            "    Version: %s\n"
            "     Vendor: %s\n"
            "   Renderer: %s\n"
            "    Shading: %s\n",
            glGetString(GL_VERSION),
            glGetString(GL_VENDOR),
            glGetString(GL_RENDERER),
            glGetString(GL_SHADING_LANGUAGE_VERSION)
    );
}

void debugger_render() {
    // render the widgets
    if (SDL_GetMouseFocus()) {
        ShowMenuBar(
                &show_console,
                &show_register_viewer,
                &show_disassembly_viewer,
                &show_memory_viewer,
                &show_overlay
        );
    }

    if (show_console)
        Debugger.console.Draw(&show_console);
    if (show_register_viewer)
        Debugger.register_viewer.Draw(&show_register_viewer);
    if (show_disassembly_viewer)
        Debugger.disassembly_viewer.Draw(&show_disassembly_viewer);
    if (show_overlay)
        Debugger.overlay.Draw(&show_overlay);
    if (show_memory_viewer)
        Debugger.memory_viewer.Draw(&show_memory_viewer);

    // Rendering
    ImGui::Render();
}