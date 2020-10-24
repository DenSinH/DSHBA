#include "PPU.h"

#include "GXHelpers.h"

#include "shaders/shaders.h"
#include "shaders/GX_constants.h"

#include "../../Frontend/interface.h"

#include <glad/glad.h>

#include "log.h"
#include "const.h"

GBAPPU::GBAPPU(s_scheduler* scheduler, Mem *memory) {
    Scheduler = scheduler;
    Memory = memory;

    BufferScanline = (s_event) {
        .callback = &GBAPPU::BufferScanlineEvent,
        .caller = this,
        .time = CYCLES_PER_SCANLINE
    };

    add_event(scheduler, &BufferScanline);

    // initially, all buffers are free
    for (size_t i = 0; i < MAX_BUFFERED_SCANLINES; i++) {
        VMEMBufferAvailable[i] = true;
    }
}

SCHEDULER_EVENT(GBAPPU::BufferScanlineEvent) {
    auto ppu = (GBAPPU*)caller;

    // wait until the next buffer is free
    ppu->WaitForBufferAvailable();

    // copy over the video data
    memcpy(
            &ppu->VMEMBuffer[ppu->BufferBufferIndex],
            &ppu->Memory->VMEM,
            sizeof(s_VMEM)
    );

    // set it to unavailable again (ready to be drawn)
    ppu->AvailabilityMutex.lock();
    ppu->VMEMBufferAvailable[ppu->BufferBufferIndex] = false;
    if (++ppu->BufferBufferIndex == MAX_BUFFERED_SCANLINES) {
        ppu->BufferBufferIndex = 0;
    }
    ppu->NewDrawAvailable.notify_all();

    ppu->AvailabilityMutex.unlock();

    ppu->BufferScanlineCount++;
    if (ppu->BufferScanlineCount == VISIBLE_SCREEN_HEIGHT) {
        event->time += (TOTAL_SCREEN_HEIGHT - VISIBLE_SCREEN_HEIGHT) * CYCLES_PER_SCANLINE;
    }
    else {
        event->time += CYCLES_PER_SCANLINE;
    }

    add_event(scheduler, event);
}

void GBAPPU::InitFramebuffers() {
    for (int i = 0; i < 2; i++) {
        // framebuffer[2] is the efb
        // we render to this separately and blit it over the actual picture like an overlay

        glGenFramebuffers(1, &Framebuffer[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer[i]);

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

    modeShaders[3] = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(modeShaders[3], 1, &FragmentShaderMode3Source, nullptr);
    CompileShader(modeShaders[3], "modeShaders[3]");

    // create program object
    Program = glCreateProgram();
    glAttachShader(Program, vertexShader);
    glAttachShader(Program, fragmentShader);
    glAttachShader(Program, modeShaders[3]);
    LinkProgram(Program);

    // dump shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(modeShaders[3]);
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

void GBAPPU::FrameSwap() {
    CurrentFramebuffer ^= 1;
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer[CurrentFramebuffer]);
    glViewport(0, 0, VISIBLE_SCREEN_WIDTH, VISIBLE_SCREEN_HEIGHT);

    // todo: backdrop color
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GBAPPU::DrawScanLine(s_VMEM* VMEM) {
    log_debug("draw scanline %d", DrawScanlineCount);

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

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, OAMUBO);
    glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            sizeof(VMEM->VRAM),
            &VMEM->VRAM,
            GL_STATIC_COPY
    );

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    const float line[] = {
        0.0, (float)DrawScanlineCount / (float)VISIBLE_SCREEN_HEIGHT,  // left
        1.0, (float)DrawScanlineCount / (float)VISIBLE_SCREEN_HEIGHT,  // right
    };


    glUseProgram(Program);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line), line, GL_STATIC_DRAW);

    glDrawArrays(GL_LINES, 0, 2);

    glBindVertexArray(0);
    glUseProgram(0);

    DrawScanlineCount++;
    if (DrawScanlineCount == VISIBLE_SCREEN_HEIGHT) {
        DrawScanlineCount = 0;
        FrameSwap();
    }
}

struct s_framebuffer GBAPPU::Render(u32 timeout) {

    // draw command is already ready to be drawn, or is ready withing the specified timeout
    if (!VMEMBufferAvailable[DrawBufferIndex] || WaitForDrawCommand(timeout)) {
        // bind our framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer[CurrentFramebuffer]);
        glViewport(0, 0, VISIBLE_SCREEN_WIDTH, VISIBLE_SCREEN_HEIGHT);

        u32 draw_start_index = DrawBufferIndex;

        AvailabilityMutex.lock();
        do {
            DrawScanLine(&VMEMBuffer[DrawBufferIndex]);

            if (++DrawBufferIndex == MAX_BUFFERED_SCANLINES) {
                DrawBufferIndex = 0;
            }

        } while (!VMEMBufferAvailable[DrawBufferIndex]
                 && DrawBufferIndex != BufferBufferIndex);


        u32 i = draw_start_index;
        do {
            VMEMBufferAvailable[i] = true;

            if (++i == MAX_BUFFERED_SCANLINES) {
                i = 0;
            }
        } while (i != DrawBufferIndex);
        NewBufferAvailable.notify_all();
        AvailabilityMutex.unlock();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    return (s_framebuffer) {
            .id = Framebuffer[CurrentFramebuffer ^ 1],
            .draw_overlay = nullptr,
            .caller = nullptr,
            .src_width = VISIBLE_SCREEN_WIDTH,
            .src_height = VISIBLE_SCREEN_HEIGHT,
            .dest_width = VISIBLE_SCREEN_WIDTH,
            .dest_height = VISIBLE_SCREEN_HEIGHT
    };
}
