#include "PPU.h"

#include "../IO/MMIO.h"
#include "../Mem/Mem.h"

#include "GXHelpers.h"
#include "shaders/shaders.h"

#include "../../Frontend/interface.h"

#include "log.h"
#include "const.h"

#define INTERNAL_FRAMEBUFFER_WIDTH 1280
#define INTERNAL_FRAMEBUFFER_HEIGHT 720


GBAPPU::GBAPPU(s_scheduler* scheduler, Mem *memory) {
    Scheduler = scheduler;
    Memory = memory;

    /* prevent possible race condition:
     *
     * (In theory, if the frontend started up really quickly, the first frame could be drawn when ScanlineBatchSizes
     * was still all 0s, causing it to get stuck in an infinite loop)
     * */
    for (int i = 0; i < VISIBLE_SCREEN_HEIGHT; i++) {
        ScanlineVRAMBatchSizes[0][i] = ScanlineVRAMBatchSizes[1][i] = 1;
        ScanlineOAMBatchSizes[0][i] = ScanlineOAMBatchSizes[1][i] = 1;
    }
    ScanlineVRAMBatchSizes[1][0] = 160;
    ScanlineOAMBatchSizes[1][0] = 160;

    // initially, VRAM is not updated at all
    for (int i = 0; i < VISIBLE_SCREEN_HEIGHT; i++) {
        VRAMRanges[0][i] = VRAMRanges[1][i] = { .min=sizeof(VRAMMEM), .max=0 };
    }
}

void GBAPPU::BufferScanline(u32 scanline) {
    // copy over range data
    VRAMRanges[BufferFrame][scanline] = Memory->VRAMUpdate;

    // copy over actual new data
    memcpy(
        PALBuffer[BufferFrame][scanline],
        Memory->PAL,
        sizeof(PALMEM)
    );
#ifndef FULL_VRAM_BUFFER
    s_UpdateRange range = VRAMRanges[BufferFrame][scanline];
    if (range.min <= range.max) {
        memcpy(
                VRAMBuffer[BufferFrame][scanline] + range.min,
                Memory->VRAM + range.min,
                range.max - range.min
        );

        // go to next batch
        CurrentVRAMScanlineBatch = scanline;
        ScanlineVRAMBatchSizes[BufferFrame][CurrentVRAMScanlineBatch] = 1;

        if (range.max > 0x10000) {
            // mark OAM as dirty as well if the object VRAM region has been updated
            Memory->DirtyOAM = true;
        }
    }
    else {
        // we can use the same batch of scanlines since VRAM was not updated
        ScanlineVRAMBatchSizes[BufferFrame][CurrentVRAMScanlineBatch]++;
    }
#else
    memcpy(
        VRAMBuffer[BufferFrame][scanline],
        Memory->VRAM,
        sizeof(VRAMMEM)
    );
    CurrentVRAMScanlineBatch = scanline;
    ScanlineVRAMBatchSizes[BufferFrame][CurrentVRAMScanlineBatch] = 1;
#endif
    if (Memory->DirtyOAM || unlikely(scanline == 0)) {
        memcpy(
                OAMBuffer[BufferFrame][scanline],
                Memory->OAM,
                sizeof(OAMMEM)
        );
        // go to next batch
        CurrentOAMScanlineBatch = scanline;
        ScanlineOAMBatchSizes[BufferFrame][CurrentOAMScanlineBatch] = 1;

        // reset dirty OAM status
        Memory->DirtyOAM = false;
    }
    else {
        // we can use the same batch of scanlines since OAM was not updated
        ScanlineOAMBatchSizes[BufferFrame][CurrentOAMScanlineBatch]++;
    }
    memcpy(
        LCDIOBuffer[BufferFrame][scanline],
        Memory->IO->Registers,
        sizeof(LCDIO)
    );

    // copy over reference line data
    ReferenceLine2Buffer[BufferFrame][scanline] = Memory->IO->ReferenceLine2;
    ReferenceLine3Buffer[BufferFrame][scanline] = Memory->IO->ReferenceLine3;

    DrawMutex.lock();
    if (unlikely(scanline == (VISIBLE_SCREEN_HEIGHT - 1))) {
        Frame++;
        BufferFrame ^= 1;

        // reset scanline batching
        CurrentVRAMScanlineBatch = 0;  // reset batch
        ScanlineVRAMBatchSizes[BufferFrame][0] = 0;
        CurrentOAMScanlineBatch = 0;  // reset batch
        ScanlineOAMBatchSizes[BufferFrame][0] = 0;
    }

    // next time: update whatever was new last scanline, plus what gets drawn next
    Memory->VRAMUpdate = VRAMRanges[BufferFrame][scanline];
    DrawMutex.unlock();
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

void GBAPPU::InitBGProgram() {
    GLuint vertexShader;
    GLuint fragmentShader, modeShaders[6];

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

    modeShaders[0] = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(modeShaders[0], 1, &FragmentShaderMode0Source, nullptr);
    CompileShader(modeShaders[0], "modeShaders[0]");

    modeShaders[1] = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(modeShaders[1], 1, &FragmentShaderMode1Source, nullptr);
    CompileShader(modeShaders[1], "modeShaders[1]");

    modeShaders[3] = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(modeShaders[3], 1, &FragmentShaderMode3Source, nullptr);
    CompileShader(modeShaders[3], "modeShaders[3]");

    modeShaders[4] = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(modeShaders[4], 1, &FragmentShaderMode4Source, nullptr);
    CompileShader(modeShaders[4], "modeShaders[4]");

    // create program object
    BGProgram = glCreateProgram();
    glAttachShader(BGProgram, vertexShader);
    glAttachShader(BGProgram, fragmentShader);
    glAttachShader(BGProgram, modeShaders[0]);
    glAttachShader(BGProgram, modeShaders[1]);
    glAttachShader(BGProgram, modeShaders[3]);
    glAttachShader(BGProgram, modeShaders[4]);
    LinkProgram(BGProgram);

    // dump shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(modeShaders[0]);
    glDeleteShader(modeShaders[1]);
    glDeleteShader(modeShaders[3]);
    glDeleteShader(modeShaders[4]);
}

void GBAPPU::InitObjProgram() {
    GLuint vertexShader;
    GLuint fragmentShader;

    // create shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    // compile it
    glShaderSource(vertexShader, 1, &ObjectVertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    CompileShader(vertexShader, "ObjectVertexShaderSource");

    // create and compile fragmentshaders
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &ObjectFragmentShaderSource, nullptr);
    CompileShader(fragmentShader, "ObjectFragmentShader");

    // create program object
    ObjProgram = glCreateProgram();
    glAttachShader(ObjProgram, vertexShader);
    glAttachShader(ObjProgram, fragmentShader);
    LinkProgram(ObjProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void GBAPPU::InitBGBuffers() {

    // Setup VAO to bind the buffers to
    glGenVertexArrays(1, &BGVAO);
    glBindVertexArray(BGVAO);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::PAL));
    glGenTextures(1, &PALTexture);
    glBindTexture(GL_TEXTURE_2D, PALTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // dimensions need to be a power of 2. Since VISIBLE_SCREEN_HEIGHT is not, we have to pick the next highest one
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sizeof(PALMEM) >> 1, 256, 0, GL_BGRA,
                 GL_UNSIGNED_SHORT_1_5_5_5_REV, nullptr);

    BGPALLocation = glGetUniformLocation(BGProgram, "PAL");

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

    // dimensions need to be a power of 2. Since VISIBLE_SCREEN_HEIGHT is not, we have to pick the next highest one
    // each relevant register is 16bit, so we store them in ushorts
    // again, since LCDIO is not of a size that is a power of 2, we use the next best one instead
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, 0x40, 256, 0, GL_RED_INTEGER,
                 GL_UNSIGNED_SHORT, nullptr);

    BGIOLocation = glGetUniformLocation(BGProgram, "IO");

    glGenBuffers(1, &BGVBO);
    glBindBuffer(GL_ARRAY_BUFFER, BGVBO);

    ReferenceLine2Location = glGetUniformLocation(BGProgram, "ReferenceLine2");
    ReferenceLine3Location = glGetUniformLocation(BGProgram, "ReferenceLine3");

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(BGProgram);
    glUniform1i(BGPALLocation, static_cast<u32>(BufferBindings::PAL));
    glUniform1i(BGIOLocation, static_cast<u32>(BufferBindings::LCDIO));

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    log_debug("OpenGL error after BG initialization: %x", glGetError());
}

void GBAPPU::InitObjBuffers() {
    // BG buffers are already initialized, so we just need to initialize the actual Obj buffers
    glGenVertexArrays(1, &ObjVAO);
    glBindVertexArray(ObjVAO);

    glGenBuffers(1, &ObjVBO);
    glBindBuffer(GL_ARRAY_BUFFER, ObjVBO);

    glVertexAttribIPointer(0, 4, GL_UNSIGNED_SHORT, sizeof(u64), (void*)0);  // OBJ_ATTR0
    glEnableVertexAttribArray(0);

    // buffer elements
    glGenBuffers(1, &ObjEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ObjEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ObjIndices.size(), ObjIndices.data(), GL_STATIC_COPY);

    // bind textures/VRAM like in BGBuffers
    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::PAL));
    glBindTexture(GL_TEXTURE_2D, PALTexture);
    ObjPALLocation = glGetUniformLocation(ObjProgram, "PAL");

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glBindTexture(GL_TEXTURE_2D, IOTexture);
    ObjIOLocation = glGetUniformLocation(ObjProgram, "IO");

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, VRAMSSBO);

    glUseProgram(ObjProgram);
    glUniform1i(ObjPALLocation, static_cast<u32>(BufferBindings::PAL));
    glUniform1i(ObjIOLocation, static_cast<u32>(BufferBindings::LCDIO));

    YClipStartLocation = glGetUniformLocation(ObjProgram, "YClipStart");
    YClipEndLocation   = glGetUniformLocation(ObjProgram, "YClipEnd");

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    log_debug("OpenGL error after Obj initialization: %x", glGetError());
}

void GBAPPU::VideoInit() {
    InitFramebuffers();
    InitBGProgram();
    InitObjProgram();
    InitBGBuffers();
    InitObjBuffers();

    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GBAPPU::DrawObjects(u32 scanline, u32 amount) {
    glUseProgram(ObjProgram);
    glBindVertexArray(ObjVAO);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::PAL));
    glBindTexture(GL_TEXTURE_2D, PALTexture);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glBindTexture(GL_TEXTURE_2D, IOTexture);

    glBindBuffer(GL_ARRAY_BUFFER, ObjVBO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ObjEBO);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xffff);

    BufferObjects<false>(BufferFrame ^ 1, scanline, amount);

    if (NumberOfObjVerts) {
        glUniform1ui(YClipStartLocation, scanline);
        glUniform1ui(YClipEndLocation, scanline + amount);

        glBufferData(GL_ARRAY_BUFFER, sizeof(u64) * NumberOfObjVerts, ObjAttr01Buffer, GL_STATIC_DRAW);
        // * 5 for the restartindex
        glDrawElements(GL_TRIANGLE_FAN, (NumberOfObjVerts >> 2) * 5, GL_UNSIGNED_SHORT, 0);
    }

    log_ppu("%d objects enabled", NumberOfObjVerts >> 2);

    glDisable(GL_PRIMITIVE_RESTART);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void GBAPPU::DrawScanlines(u32 scanline, u32 amount) {
    s_UpdateRange range;
    glBindVertexArray(BGVAO);

#ifndef FULL_VRAM_BUFFER
    range = VRAMRanges[BufferFrame ^ 1][scanline];
    if (range.min <= range.max) {
        log_ppu("Buffering %x bytes of VRAM data (%x -> %x)", range.max + 4 - range.min, range.min, range.max);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, VRAMSSBO);
        glBufferSubData(
                GL_SHADER_STORAGE_BUFFER,
                range.min,
                range.max - range.min,
                &VRAMBuffer[BufferFrame ^ 1][scanline][range.min]
        );
        // reset range data
        VRAMRanges[BufferFrame ^ 1][scanline] = { .min = sizeof(VRAMMEM), .max = 0 };
    }
#else
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, VRAMSSBO);
    glBufferSubData(
            GL_SHADER_STORAGE_BUFFER,
            0,
            sizeof(VRAMMEM),
            &VRAMBuffer[BufferFrame ^ 1][scanline][0]
    );
#endif

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glUseProgram(BGProgram);
    glBindVertexArray(BGVAO);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::PAL));
    glBindTexture(GL_TEXTURE_2D, PALTexture);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glBindTexture(GL_TEXTURE_2D, IOTexture);

    glBindBuffer(GL_ARRAY_BUFFER, BGVBO);
    const float quad[8] = {
            -1.0, (float) scanline,            // top left
             1.0, (float) scanline,            // top right
             1.0, (float)(scanline + amount),  // bottom right
            -1.0, (float)(scanline + amount),  // bottom left
    };
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), quad, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 8);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

struct s_framebuffer GBAPPU::Render() {

    // draw command is already ready to be drawn, or is ready withing the specified timeout
    // bind our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);
    glViewport(0, 0, INTERNAL_FRAMEBUFFER_WIDTH, INTERNAL_FRAMEBUFFER_HEIGHT);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawMutex.lock();
    // buffer PAL texture
    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::PAL));
    glBindTexture(GL_TEXTURE_2D, PALTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizeof(PALMEM) >> 1, VISIBLE_SCREEN_HEIGHT,
                    GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, PALBuffer[BufferFrame ^ 1]);
    glBindTexture(GL_TEXTURE_2D, 0);

    // buffer IO texture
    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glBindTexture(GL_TEXTURE_2D, IOTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizeof(LCDIO) >> 1, VISIBLE_SCREEN_HEIGHT,
                    GL_RED_INTEGER, GL_UNSIGNED_SHORT, LCDIOBuffer[BufferFrame ^ 1]);
    glBindTexture(GL_TEXTURE_2D, 0);

    // buffer reference points for affine backgrounds
    glUniform1uiv(ReferenceLine2Location, VISIBLE_SCREEN_HEIGHT, ReferenceLine2Buffer[BufferFrame ^ 1]);
    glUniform1uiv(ReferenceLine3Location, VISIBLE_SCREEN_HEIGHT, ReferenceLine3Buffer[BufferFrame ^ 1]);

    size_t VRAM_scanline = 0, OAM_scanline = 0;
    do {
        if (VRAM_scanline < OAM_scanline) {
            u32 batch_size = ScanlineVRAMBatchSizes[BufferFrame ^ 1][VRAM_scanline];
            DrawScanlines(VRAM_scanline, batch_size);
            VRAM_scanline += batch_size;
            log_ppu("%d VRAM scanlines batched (accum %d)", batch_size, VRAM_scanline);
        }
        else {
            u32 batch_size = ScanlineOAMBatchSizes[BufferFrame ^ 1][OAM_scanline];
            DrawObjects(OAM_scanline, batch_size);
            OAM_scanline += batch_size;

            log_ppu("%d OAM scanlines batched (accum %d)", batch_size, OAM_scanline);
        }
        // should actually be !=, but just to be sure we don't ever get stuck
    } while (VRAM_scanline < VISIBLE_SCREEN_HEIGHT
          || OAM_scanline < VISIBLE_SCREEN_HEIGHT);

#ifdef CHECK_SCANLINE_BATCH_ACCUM
    if (unlikely((VRAM_scanline != VISIBLE_SCREEN_HEIGHT)) || (OAM_scanline != VISIBLE_SCREEN_HEIGHT)) {
        // log_warn("Something went wrong in batching scanlines: expected %d, got %d, %d", VISIBLE_SCREEN_HEIGHT, VRAM_scanline, OAM_scanline);
    }
#endif

    DrawMutex.unlock();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return (s_framebuffer) {
            .id = Framebuffer,
            .src_width = INTERNAL_FRAMEBUFFER_WIDTH,
            .src_height = INTERNAL_FRAMEBUFFER_HEIGHT,
            .dest_width = VISIBLE_SCREEN_WIDTH,
            .dest_height = VISIBLE_SCREEN_HEIGHT
    };
}
