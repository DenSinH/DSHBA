#include "debugger.h"
#include "frontend.h"
#include "interface.h"

#include "widgets/menubar.h"
#include "widgets/overlay.h"
#include "widgets/console.h"
#include "widgets/register_viewer.h"
#include "widgets/disassembly_viewer.h"
#include "widgets/memory_viewer.h"
#include "widgets/cache_block_stats.h"

#ifdef NDEBUG
#define DEFAULT_DEBUGGER_WIDGET_STATE false
#else
#define DEFAULT_DEBUGGER_WIDGET_STATE true
#endif

static bool show_console = DEFAULT_DEBUGGER_WIDGET_STATE;
static bool show_register_viewer = DEFAULT_DEBUGGER_WIDGET_STATE;
static bool show_disassembly_viewer = DEFAULT_DEBUGGER_WIDGET_STATE;
static bool show_overlay = false;
static bool show_memory_viewer = DEFAULT_DEBUGGER_WIDGET_STATE;
static bool show_cached_block_stats = DEFAULT_DEBUGGER_WIDGET_STATE;

// Our state
static struct s_debugger {
    ConsoleWidget console;
    RegisterViewer register_viewer;
    DisassemblyViewer disassembly_viewer;
    Overlay overlay;
    MemoryViewer memory_viewer;
    CacheBlockStats cache_block_stats;
} Debugger;

void add_command(const char* command, const char* description, CONSOLE_COMMAND((*callback))) {
    Debugger.console.AddCommand(s_console_command {
            .command = command,
            .description = description,
            .callback = callback
    });
}

void bind_cache_step_data(uint32_t* cached, uint32_t* make_cached, uint32_t* non_cached) {
    Debugger.cache_block_stats = CacheBlockStats(cached, make_cached, non_cached);
}

void add_overlay_info(OVERLAY_INFO((*getter))) {
    Debugger.overlay.AddInfo(getter);
}

int add_register_tab(const char* name){
    return Debugger.register_viewer.AddRegisterTab(name);
}

void add_register_data(const char* name, const void* value, size_t size, int tab) {
    Debugger.register_viewer.AddRegister(name, value, size, tab);
}

void debugger_init(
        uint32_t* PC,
        uint64_t mem_size,
        uint8_t* (*valid_address_mask)(uint32_t),
        uint8_t (*mem_read)(uint64_t off),
        bool (*arm_mode)()
) {
    Debugger.disassembly_viewer.PC = PC;
    Debugger.disassembly_viewer.valid_address = valid_address_mask;
    Debugger.disassembly_viewer.arm_mode = arm_mode;
    Debugger.memory_viewer.mem_size = mem_size;
    Debugger.memory_viewer.ReadFn = mem_read;
}


void debugger_video_init(const char* glsl_version, SDL_Window* window, SDL_GLContext* gl_context) {
// Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

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
                &show_overlay,
                &show_cached_block_stats
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
    Debugger.cache_block_stats.Update();
    if (show_cached_block_stats)
        Debugger.cache_block_stats.Draw(&show_cached_block_stats);
}