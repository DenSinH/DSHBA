#include "PPU.h"

#include "GXHelpers.h"

#include "shaders/shaders.h"

#include "../../Frontend/interface.h"

#include <glad/glad.h>

#include "log.h"
#include "const.h"

#define INTERNAL_FRAMEBUFFER_WIDTH 480
#define INTERNAL_FRAMEBUFFER_HEIGHT 320

static float QuadData[VISIBLE_SCREEN_HEIGHT][12];

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
        // top left point:
        QuadData[i][0] = -1.0;
        QuadData[i][1] = static_cast<float>(-1.0 + 2.0 * (float)i / (float)VISIBLE_SCREEN_HEIGHT);
        // top right point:
        QuadData[i][2] = 1.0;
        QuadData[i][3] = static_cast<float>(-1.0 + 2.0 * (float)i / (float)VISIBLE_SCREEN_HEIGHT);
        // bottom right point
        QuadData[i][4] = 1.0;
        QuadData[i][5] = static_cast<float>(-1.0 + 2.0 * (float)(i + 1) / (float)VISIBLE_SCREEN_HEIGHT);
        // top left point:
        QuadData[i][6] = QuadData[i][0];
        QuadData[i][7] = QuadData[i][1];
        // bottom right point
        QuadData[i][8] = QuadData[i][4];
        QuadData[i][9] = QuadData[i][5];
        // bottom left point
        QuadData[i][0] = -1.0;
        QuadData[i][1] = static_cast<float>(-1.0 + 2.0 * (float)(i + 1) / (float)VISIBLE_SCREEN_HEIGHT);
    }
}

static ALWAYS_INLINE void ConditionalBuffer(s_UpdateRange* range, u8* dest, u8* src) {
    range->min &= ~3;
    range->max = (range->max + 3) & ~3;

    if (range->min <= range->max) {
        memcpy(
            dest + range->min,
            src + range->min,
            range->max + 4 - range->min
        );
    }
}

SCHEDULER_EVENT(GBAPPU::BufferScanlineEvent) {
    auto ppu = (GBAPPU*)caller;

    // copy over range data
    ppu->VRAMRanges[ppu->BufferFrame][ppu->BufferScanlineCount] = ppu->Memory->VRAMUpdate;
    ppu->OAMRanges [ppu->BufferFrame][ppu->BufferScanlineCount] = ppu->Memory->OAMUpdate;

    // copy over actual new data
    memcpy(
        ppu->PALBuffer[ppu->BufferFrame][ppu->BufferScanlineCount],
        ppu->Memory->PAL,
        sizeof(PALMEM)
    );
    ConditionalBuffer(
            &ppu->VRAMRanges[ppu->BufferFrame][ppu->BufferScanlineCount],
            ppu->VRAMBuffer[ppu->BufferFrame][ppu->BufferScanlineCount],
            ppu->Memory->VRAM
    );
    memcpy(
        ppu->OAMBuffer[ppu->BufferFrame][ppu->BufferScanlineCount],
        ppu->Memory->OAM,
        sizeof(OAMMEM)
    );
    memcpy(
        ppu->LCDIOBuffer[ppu->BufferFrame][ppu->BufferScanlineCount],
        ppu->Memory->IO,
        sizeof(LCDIO)
    );

    ppu->BufferScanlineCount++;
    ppu->DrawMutex.lock();
    // next time: update whatever was new last scanline, plus what gets drawn next
    if (unlikely(ppu->BufferScanlineCount == VISIBLE_SCREEN_HEIGHT)) {
        ppu->BufferScanlineCount = 0;
        ppu->Frame++;
        ppu->BufferFrame ^= 1;
        event->time += (TOTAL_SCREEN_HEIGHT - VISIBLE_SCREEN_HEIGHT) * CYCLES_PER_SCANLINE;
    }
    else {
        event->time += CYCLES_PER_SCANLINE;
    }

    ppu->Memory->VRAMUpdate = ppu->VRAMRanges[ppu->BufferFrame][ppu->BufferScanlineCount];
    ppu->Memory->OAMUpdate  = ppu->OAMRanges [ppu->BufferFrame][ppu->BufferScanlineCount];
    ppu->DrawMutex.unlock();
    // log_debug("from last time: %x -> %x (%d, %d)", ppu->Memory->VRAMUpdate.min, ppu->Memory->VRAMUpdate.max, ppu->BufferFrame, ppu->BufferScanlineCount);

    add_event(scheduler, event);
}

void GBAPPU::InitFramebuffers() {
    // we render to this separately and blit it over the actual picture like an overlay
    glGenFramebuffers(1, &Framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);

    GLuint rendered_texture, depth_buffer;
    // create a texture to render to and fill it with 0 (also set filtering to low)
    glGenTextures(1, &rendered_texture);
    glBindTexture(GL_TEXTURE_2D, rendered_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, INTERNAL_FRAMEBUFFER_WIDTH, INTERNAL_FRAMEBUFFER_HEIGHT,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // add depth buffer
    glGenRenderbuffers(1, &depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, INTERNAL_FRAMEBUFFER_WIDTH, INTERNAL_FRAMEBUFFER_HEIGHT);
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

    // Setup VAO to bind the buffers to
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::PAL));
    glGenTextures(1, &PALTexture);
    glBindTexture(GL_TEXTURE_2D, PALTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // dimensions need to be a power of 2. Since VISIBLE_SCREEN_HEIGHT is not, we have to pick the next highest one
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sizeof(PALMEM) >> 1, 256, 0, GL_BGRA,
                 GL_UNSIGNED_SHORT_1_5_5_5_REV, nullptr);

    PALLocation = glGetUniformLocation(Program, "PAL");

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::OAM));
    glGenTextures(1, &OAMTexture);
    glBindTexture(GL_TEXTURE_2D, OAMTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    log_debug("OpenGL error: %x", glGetError());
    // dimensions need to be a power of 2. Since VISIBLE_SCREEN_HEIGHT is not, we have to pick the next highest one
    // OAM holds 4 shorts per "index", so we store those in vec4s
    // therefore, the dimension should be sizeof(OAMMEM) / (2 * 4 bytes per OAM entry)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16UI, sizeof(OAMMEM) >> 3, 256, 0, GL_RGBA_INTEGER,
                 GL_UNSIGNED_SHORT, nullptr);
    log_debug("OpenGL error: %x", glGetError());

    OAMLocation = glGetUniformLocation(Program, "OAM");

    // Initially buffer the buffers with some data so we don't have to reallocate memory every time
    glGenBuffers(1, &VRAMSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, VRAMSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<unsigned int>(BufferBindings::VRAMSSBO), VRAMSSBO);
    glBufferData(
            GL_SHADER_STORAGE_BUFFER, sizeof(VRAMMEM),
            VRAMBuffer[0][0], GL_STREAM_DRAW
    );

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glGenTextures(1, &IOTexture);
    glBindTexture(GL_TEXTURE_2D, IOTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    log_debug("OpenGL error: %x", glGetError());
    // dimensions need to be a power of 2. Since VISIBLE_SCREEN_HEIGHT is not, we have to pick the next highest one
    // each relevant register is 16bit, so we store them in ushorts
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, sizeof(LCDIO) >> 1, 256, 0, GL_RED_INTEGER,
                 GL_UNSIGNED_SHORT, nullptr);
    log_debug("OpenGL error: %x", glGetError());

    IOLocation = glGetUniformLocation(Program, "IO");

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(Program);
    glUniform1i(PALLocation, static_cast<u32>(BufferBindings::PAL));
    glUniform1i(OAMLocation, static_cast<u32>(BufferBindings::OAM));
    glUniform1i(IOLocation, static_cast<u32>(BufferBindings::LCDIO));


    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    log_debug("OpenGL error: %x", glGetError());
}

void GBAPPU::VideoInit() {
    InitFramebuffers();
    InitPrograms();
    InitBuffers();

    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LEQUAL);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GBAPPU::DrawScanLine(u32 scanline) {
    s_UpdateRange range;
    u8 DrawFrame = BufferFrame ^ 1;
    glBindVertexArray(VAO);

    range = VRAMRanges[DrawFrame][scanline];
    if (range.min <= range.max) {
        log_ppu("Buffering %x bytes of VRAM data (%x -> %x)", range.max + 4 - range.min, range.min, range.max);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, VRAMSSBO);
        glBufferSubData(
                GL_SHADER_STORAGE_BUFFER,
                range.min,
                range.max + 4 - range.min,
                &VRAMBuffer[DrawFrame][scanline][range.min]
        );
        // reset range data
        VRAMRanges[DrawFrame][scanline] = { .min = sizeof(VRAMMEM), .max = 0 };
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glUseProgram(Program);
    glBindVertexArray(VAO);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::PAL));
    glBindTexture(GL_TEXTURE_2D, PALTexture);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::OAM));
    glBindTexture(GL_TEXTURE_2D, OAMTexture);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glBindTexture(GL_TEXTURE_2D, IOTexture);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), QuadData[scanline], GL_STATIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, 12);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

struct s_framebuffer GBAPPU::Render() {

    // draw command is already ready to be drawn, or is ready withing the specified timeout
    // bind our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);
    glViewport(0, 0, INTERNAL_FRAMEBUFFER_WIDTH, INTERNAL_FRAMEBUFFER_HEIGHT);

    // todo: backdrop color
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw scanlines that are available
    DrawMutex.lock();
    // bind PAL texture
    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::PAL));
    glBindTexture(GL_TEXTURE_2D, PALTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizeof(PALMEM) >> 1, VISIBLE_SCREEN_HEIGHT,
                    GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, PALBuffer[BufferFrame ^ 1]);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::OAM));
    glBindTexture(GL_TEXTURE_2D, OAMTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizeof(OAMMEM) >> 3, VISIBLE_SCREEN_HEIGHT,
                    GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, OAMBuffer[BufferFrame ^ 1]);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glBindTexture(GL_TEXTURE_2D, IOTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizeof(LCDIO) >> 1, VISIBLE_SCREEN_HEIGHT,
                    GL_RED_INTEGER, GL_UNSIGNED_SHORT, LCDIOBuffer[BufferFrame ^ 1]);
    glBindTexture(GL_TEXTURE_2D, 0);

    for (int i = 0; i < VISIBLE_SCREEN_HEIGHT; i++) {
        DrawScanLine(i);
    }
    DrawMutex.unlock();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return (s_framebuffer) {
            .id = Framebuffer,
            .draw_overlay = nullptr,
            .caller = nullptr,
            .src_width = INTERNAL_FRAMEBUFFER_WIDTH,
            .src_height = INTERNAL_FRAMEBUFFER_HEIGHT,
            .dest_width = VISIBLE_SCREEN_WIDTH,
            .dest_height = VISIBLE_SCREEN_HEIGHT
    };
}
