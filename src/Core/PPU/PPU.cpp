#include "PPU.h"

#include "GXHelpers.h"

#include "shaders/shaders.h"

#include "../../Frontend/interface.h"

#include <glad/glad.h>

#include "log.h"
#include "const.h"

static float LineData[VISIBLE_SCREEN_HEIGHT][4];

GBAPPU::GBAPPU(s_scheduler* scheduler, Mem *memory) {
    Scheduler = scheduler;
    Memory = memory;

    BufferScanline = (s_event) {
        .callback = &GBAPPU::BufferScanlineEvent,
        .caller = this,
        .time = CYCLES_PER_SCANLINE
    };

    add_event(scheduler, &BufferScanline);

    for (int i = 0; i < VISIBLE_SCREEN_HEIGHT; i++) {
        // left point:
        LineData[i][0] = -1.0;
        LineData[i][1] = static_cast<float>(-1.0 + 2 * (float)i / (float)VISIBLE_SCREEN_HEIGHT);
        // right point:
        LineData[i][2] = 1.0;
        LineData[i][3] = static_cast<float>(-1.0 + 2 * (float)i / (float)VISIBLE_SCREEN_HEIGHT);
    }
}

SCHEDULER_EVENT(GBAPPU::BufferScanlineEvent) {
    auto ppu = (GBAPPU*)caller;

    // wait until the next buffer is free

    // copy over the video data
    memcpy(
            &ppu->VMEMBuffer[ppu->BufferFrame][ppu->BufferScanlineCount],
            &ppu->Memory->VMEM,
            sizeof(s_VMEM)
    );

    ppu->BufferScanlineCount++;
    if (ppu->BufferScanlineCount == VISIBLE_SCREEN_HEIGHT) {
        ppu->BufferScanlineCount = 0;
        ppu->BufferFrame ^= 1;
        ppu->Frame++;
        event->time += (TOTAL_SCREEN_HEIGHT - VISIBLE_SCREEN_HEIGHT) * CYCLES_PER_SCANLINE;
    }
    else {
        event->time += CYCLES_PER_SCANLINE;
    }

    add_event(scheduler, event);
}

void GBAPPU::InitFramebuffers() {
    // framebuffer[2] is the efb
    // we render to this separately and blit it over the actual picture like an overlay

    glGenFramebuffers(1, &Framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);

    GLuint rendered_texture, depth_buffer;
    // create a texture to render to and fill it with 0 (also set filtering to low)
    glGenTextures(1, &rendered_texture);
    glBindTexture(GL_TEXTURE_2D, rendered_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VISIBLE_SCREEN_WIDTH, VISIBLE_SCREEN_WIDTH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // add depth buffer
    glGenRenderbuffers(1, &depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, VISIBLE_SCREEN_WIDTH, VISIBLE_SCREEN_WIDTH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rendered_texture, 0);
    GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, draw_buffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        log_fatal("Error initializing framebuffer");
    }
}

void GBAPPU::InitPrograms() {
    unsigned int vertexShader;
    unsigned int fragmentShader, modeShaders[6];

    // create shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    // compile it
    glShaderSource(vertexShader, 1, &VertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    CompileShader(vertexShader, "VertexShaderSource");

    // create and compile fragmentshaders
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &FragmentShaderSource, nullptr);
    CompileShader(fragmentShader, "fragmentShader");

    modeShaders[4] = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(modeShaders[4], 1, &FragmentShaderMode4Source, nullptr);
    CompileShader(modeShaders[4], "modeShaders[4]");

    // create program object
    Program = glCreateProgram();
    glAttachShader(Program, vertexShader);
    glAttachShader(Program, fragmentShader);
    glAttachShader(Program, modeShaders[4]);
    LinkProgram(Program);

    // dump shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(modeShaders[4]);
}

void GBAPPU::InitBuffers() {
    glGenBuffers(1, &PALUBO);
    glGenBuffers(1, &OAMUBO);
    glGenBuffers(1, &IOUBO);
    glGenBuffers(1, &VRAMSSBO);

    // Setup VAO to bind the buffers to
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_UNIFORM_BUFFER, PALUBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BufferBindings::PALUBO), PALUBO);

    glBindBuffer(GL_UNIFORM_BUFFER, OAMUBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BufferBindings::OAMUBO), OAMUBO);

    glBindBuffer(GL_UNIFORM_BUFFER, IOUBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BufferBindings::IOUBO), IOUBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, VRAMSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BufferBindings::VRAMSSBO), VRAMSSBO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GBAPPU::VideoInit() {
    InitFramebuffers();
    InitPrograms();
    InitBuffers();

    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);  // todo: base this on BP register
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GBAPPU::DrawScanLine(s_VMEM* VMEM, u32 scanline) const {
    glBindBuffer(GL_UNIFORM_BUFFER, PALUBO);
    glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            sizeof(VMEM->PAL),
            &VMEM->PAL,
            GL_STATIC_COPY
    );

    glBindBuffer(GL_UNIFORM_BUFFER, OAMUBO);
    glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            sizeof(VMEM->OAM),
            &VMEM->OAM,
            GL_STATIC_COPY
    );

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, VRAMSSBO);
    glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            sizeof(VMEM->VRAM),
            &VMEM->VRAM,
            GL_STATIC_COPY
    );

    // todo: buffer IO

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glUseProgram(Program);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), &LineData[scanline], GL_STATIC_DRAW);

    glDrawArrays(GL_LINES, 0, 2);

    glBindVertexArray(0);
    glUseProgram(0);
}

struct s_framebuffer GBAPPU::Render() {

    // draw command is already ready to be drawn, or is ready withing the specified timeout
    // bind our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);
    glViewport(0, 0, VISIBLE_SCREEN_WIDTH, VISIBLE_SCREEN_HEIGHT);

    // todo: backdrop color
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw scanlines that are available
    for (int i = 0; i < VISIBLE_SCREEN_HEIGHT; i++) {
        DrawScanLine(&VMEMBuffer[!BufferFrame][i], i);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return (s_framebuffer) {
            .id = Framebuffer,
            .draw_overlay = nullptr,
            .caller = nullptr,
            .src_width = VISIBLE_SCREEN_WIDTH,
            .src_height = VISIBLE_SCREEN_HEIGHT,
            .dest_width = VISIBLE_SCREEN_WIDTH,
            .dest_height = VISIBLE_SCREEN_HEIGHT
    };
}
