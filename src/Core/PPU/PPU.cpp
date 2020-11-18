#include "PPU.h"

#include "../IO/MMIO.h"
#include "../Mem/Mem.h"

#include "GXHelpers.h"
#include "shaders/shaders.h"

#include "../../Frontend/interface.h"

#include "log.h"
#include "const.h"

#include <SDL.h>

const char* glsl_version = "#version 430 core\n";

#define INTERNAL_FRAMEBUFFER_WIDTH 480
#define INTERNAL_FRAMEBUFFER_HEIGHT 320


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
    if (unlikely(scanline == 0)) {
        Frame++;

        // we really only need to lock for swapping the frame
        if (FrameSkip) {
            DrawMutex.lock();
            BufferFrame ^= 1;
            DrawMutex.unlock();
        }
        else {
            {
                std::unique_lock<std::mutex> lock(DrawMutex);
                FrameDrawnVariable.wait(lock, [this]{ return FrameDrawn; });
                FrameDrawn = false;
            }

            BufferFrame ^= 1;

            if (unlikely(!SyncToVideo)) {
                std::lock_guard<std::mutex> lock(FrameWaitMutex);
                FrameReady = true;
                FrameReadyVariable.notify_all();
            }
        }

        // reset scanline batching
        CurrentVRAMScanlineBatch = 0;  // reset batch
        ScanlineVRAMBatchSizes[BufferFrame][0] = 0;
        CurrentOAMScanlineBatch = 0;  // reset batch
        ScanlineOAMBatchSizes[BufferFrame][0] = 0;
    }

    // copy over range data
    VRAMRanges[BufferFrame][scanline] = Memory->VRAMUpdate;

    // copy over actual new data
    memcpy(
        PALBuffer[BufferFrame][scanline],
        Memory->PAL,
        sizeof(PALMEM)
    );
#ifndef FULL_VRAM_BUFFER
    s_UpdateRange range = Memory->VRAMUpdate;
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
    if (Memory->PrevDirtyOAM || (scanline == 0)) {
        memcpy(
                OAMBuffer[BufferFrame][scanline],
                Memory->OAM,
                sizeof(OAMMEM)
        );
        // go to next batch
        CurrentOAMScanlineBatch = scanline;
        ScanlineOAMBatchSizes[BufferFrame][CurrentOAMScanlineBatch] = 1;
    }
    else {
        // we can use the same batch of scanlines since OAM was not updated
        ScanlineOAMBatchSizes[BufferFrame][CurrentOAMScanlineBatch]++;
    }
    // reset dirty OAM status
    Memory->PrevDirtyOAM = Memory->DirtyOAM;
    Memory->DirtyOAM = false;

    memcpy(
        LCDIOBuffer[BufferFrame][scanline],
        Memory->IO->Registers,
        sizeof(LCDIO)
    );

    // copy over reference line data
    ReferenceLine2Buffer[BufferFrame][scanline] = Memory->IO->ReferenceLine2;
    ReferenceLine3Buffer[BufferFrame][scanline] = Memory->IO->ReferenceLine3;

    if (FrameSkip) {
        // next time: update whatever was new last frame, plus what gets drawn next
        DrawMutex.lock();
        Memory->VRAMUpdate = VRAMRanges[BufferFrame ^ 1][scanline];
        DrawMutex.unlock();
    }
    else {
        Memory->VRAMUpdate = { .min = sizeof(VRAMMEM), .max = 0 };
    }
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

    CheckFramebufferInit("general");

    glGenFramebuffers(1, &WinFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, WinFramebuffer);

    // create a texture to render to and fill it with 0 (also set filtering to low)
    glGenTextures(1, &WinTexture);
    glBindTexture(GL_TEXTURE_2D, WinTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, VISIBLE_SCREEN_WIDTH, VISIBLE_SCREEN_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // add depth buffer
    glGenRenderbuffers(1, &WinDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, WinDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, VISIBLE_SCREEN_WIDTH, VISIBLE_SCREEN_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, WinDepthBuffer);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, WinTexture, 0);
    GLenum tex_draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, tex_draw_buffers);

    CheckFramebufferInit("window");
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GBAPPU::InitBGProgram() {
    GLuint vertexShader;
    GLuint fragmentShader, modeShaders[6], fragmentHelpers;

    // create shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    // compile it
    glShaderSource(vertexShader, 1, &VertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    CompileShader(vertexShader, "BGVertexShader");

    // create and compile fragmentshaders
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &FragmentShaderSource, nullptr);
    CompileShader(fragmentShader, "BGFragmentShader");

    fragmentHelpers = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentHelpers, 1, &FragmentHelperSource, nullptr);
    CompileShader(fragmentHelpers, "BGFragmentHelpers");

    modeShaders[0] = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(modeShaders[0], 1, &FragmentShaderMode0Source, nullptr);
    CompileShader(modeShaders[0], "modeShaders[0]");

    modeShaders[1] = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(modeShaders[1], 1, &FragmentShaderMode1Source, nullptr);
    CompileShader(modeShaders[1], "modeShaders[1]");

    modeShaders[2] = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(modeShaders[2], 1, &FragmentShaderMode2Source, nullptr);
    CompileShader(modeShaders[2], "modeShaders[2]");

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
    glAttachShader(BGProgram, fragmentHelpers);
    glAttachShader(BGProgram, modeShaders[0]);
    glAttachShader(BGProgram, modeShaders[1]);
    glAttachShader(BGProgram, modeShaders[2]);
    glAttachShader(BGProgram, modeShaders[3]);
    glAttachShader(BGProgram, modeShaders[4]);
    LinkProgram(BGProgram);

    // dump shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(fragmentHelpers);
    glDeleteShader(modeShaders[0]);
    glDeleteShader(modeShaders[1]);
    glDeleteShader(modeShaders[2]);
    glDeleteShader(modeShaders[3]);
    glDeleteShader(modeShaders[4]);
}

void GBAPPU::InitObjProgram() {
    GLuint vertexShader;
    GLuint fragmentShader, fragmentHelpers;

    // create shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    // compile it
    const char* vtx_sources[3] = {glsl_version, "#undef OBJ_WINDOW\n", ObjectVertexShaderSource};
    glShaderSource(vertexShader, 3, vtx_sources, nullptr);
    glCompileShader(vertexShader);
    CompileShader(vertexShader, "ObjectVertexShader");

    // create and compile fragmentshaders
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* frag_sources[3] = {glsl_version, "#undef OBJ_WINDOW\n", ObjectFragmentShaderSource};
    glShaderSource(fragmentShader, 3, frag_sources, nullptr);
    CompileShader(fragmentShader, "ObjectFragmentShader");

    fragmentHelpers = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentHelpers, 1, &FragmentHelperSource, nullptr);
    CompileShader(fragmentHelpers, "ObjectFragmentHelper");

    // create program object
    ObjProgram = glCreateProgram();
    glAttachShader(ObjProgram, vertexShader);
    glAttachShader(ObjProgram, fragmentShader);
    glAttachShader(ObjProgram, fragmentHelpers);
    LinkProgram(ObjProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(fragmentHelpers);
}

void GBAPPU::InitWinBGProgram() {
    GLuint vertexShader;
    GLuint fragmentShader, fragmentHelpers;

    // create shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    // same vertex shader as BGProgram
    glShaderSource(vertexShader, 1, &VertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    CompileShader(vertexShader, "WindowVertexShader");

    // create and compile fragmentshaders
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &WindowFragmentShaderSource, nullptr);
    CompileShader(fragmentShader, "WindowFragmentShader");

    fragmentHelpers = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentHelpers, 1, &FragmentHelperSource, nullptr);
    CompileShader(fragmentHelpers, "WindowFragmentHelper");

    // create program object
    WinBGProgram = glCreateProgram();
    glAttachShader(WinBGProgram, vertexShader);
    glAttachShader(WinBGProgram, fragmentShader);
    glAttachShader(WinBGProgram, fragmentHelpers);
    LinkProgram(WinBGProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(fragmentHelpers);
}

void GBAPPU::InitWinObjProgram() {
    GLuint vertexShader;
    GLuint fragmentShader, fragmentHelpers;

    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    // compile it
    const char* vtx_sources[3] = {glsl_version, "#define OBJ_WINDOW\n", ObjectVertexShaderSource};
    glShaderSource(vertexShader, 3, vtx_sources, nullptr);
    glCompileShader(vertexShader);
    CompileShader(vertexShader, "WinObjectVertexShader");

    // create and compile fragmentshaders
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* frag_sources[3] = {glsl_version, "#define OBJ_WINDOW\n", ObjectFragmentShaderSource};
    glShaderSource(fragmentShader, 3, frag_sources, nullptr);
    CompileShader(fragmentShader, "WinObjectFragmentShader");

    fragmentHelpers = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentHelpers, 1, &FragmentHelperSource, nullptr);
    CompileShader(fragmentHelpers, "WinObjectFragmentHelper");

    // create program object
    WinObjProgram = glCreateProgram();
    glAttachShader(WinObjProgram, vertexShader);
    glAttachShader(WinObjProgram, fragmentShader);
    glAttachShader(WinObjProgram, fragmentHelpers);
    LinkProgram(WinObjProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(fragmentHelpers);
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

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::Window));
    glBindTexture(GL_TEXTURE_2D, WinTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    BGWindowLocation = glGetUniformLocation(BGProgram, "Window");

    glGenBuffers(1, &BGVBO);
    glBindBuffer(GL_ARRAY_BUFFER, BGVBO);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), nullptr, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    ReferenceLine2Location = glGetUniformLocation(BGProgram, "ReferenceLine2");
    ReferenceLine3Location = glGetUniformLocation(BGProgram, "ReferenceLine3");
    BGLocation             = glGetUniformLocation(BGProgram, "BG");

    glUseProgram(BGProgram);
    glUniform1i(BGPALLocation, static_cast<u32>(BufferBindings::PAL));
    glUniform1i(BGIOLocation, static_cast<u32>(BufferBindings::LCDIO));
    glUniform1i(BGWindowLocation, static_cast<u32>(BufferBindings::Window));

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);

    log_debug("OpenGL error after BG initialization: %x", glGetError());
}

void GBAPPU::InitObjBuffers() {
    // BG buffers are already initialized, so we just need to initialize the actual Obj buffers
    glGenVertexArrays(1, &ObjVAO);
    glBindVertexArray(ObjVAO);

    glGenBuffers(1, &ObjVBO);
    glBindBuffer(GL_ARRAY_BUFFER, ObjVBO);

    glVertexAttribIPointer(0, 4, GL_UNSIGNED_SHORT, sizeof(u64), (void*)0);  // OBJ attributes
    glEnableVertexAttribArray(0);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::OAM));
    glGenTextures(1, &OAMTexture);
    glBindTexture(GL_TEXTURE_1D, OAMTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // dimensions need to be a power of 2. Since VISIBLE_SCREEN_HEIGHT is not, we have to pick the next highest one
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA16I, sizeof(OAMMEM) >> 3, 0, GL_RGBA_INTEGER,
                 GL_SHORT, nullptr);

    ObjOAMLocation = glGetUniformLocation(ObjProgram, "OAM");

    // buffer elements, we only need to do this once
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
    glUniform1i(ObjOAMLocation, static_cast<u32>(BufferBindings::OAM));

    ObjYClipStartLocation = glGetUniformLocation(ObjProgram, "YClipStart");
    ObjYClipEndLocation   = glGetUniformLocation(ObjProgram, "YClipEnd");
    ObjWindowLocation     = glGetUniformLocation(ObjProgram, "Window");
    glUniform1i(ObjWindowLocation, static_cast<u32>(BufferBindings::Window));

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);

    log_debug("OpenGL error after Obj initialization: %x", glGetError());
}

void GBAPPU::InitWinBGBuffers() {
    // BG buffers are already initialized, so we just need to initialize the actual Obj buffers
    glGenVertexArrays(1, &WinBGVAO);
    glBindVertexArray(WinBGVAO);

    // use same buffer for object in object window and BG buffer
    glBindBuffer(GL_ARRAY_BUFFER, BGVBO);
    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glBindTexture(GL_TEXTURE_2D, IOTexture);
    WinBGIOLocation = glGetUniformLocation(WinBGProgram, "IO");

    glUseProgram(WinBGProgram);
    glUniform1i(WinBGIOLocation, static_cast<u32>(BufferBindings::LCDIO));

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    log_debug("OpenGL error after Win BG initialization: %x", glGetError());
}

void GBAPPU::InitWinObjBuffers() {
    // BG/window buffers are already initialized, so we just need to initialize the actual Obj buffers
    glGenVertexArrays(1, &WinObjVAO);
    glBindVertexArray(WinObjVAO);

    // use same buffers as Object program
    glBindBuffer(GL_ARRAY_BUFFER, ObjVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ObjEBO);

    glVertexAttribIPointer(0, 4, GL_UNSIGNED_SHORT, sizeof(u64), (void*)0);  // OBJ attributes
    glEnableVertexAttribArray(0);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::OAM));
    glGenTextures(1, &OAMTexture);
    glBindTexture(GL_TEXTURE_1D, OAMTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // dimensions need to be a power of 2. Since VISIBLE_SCREEN_HEIGHT is not, we have to pick the next highest one
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA16I, sizeof(OAMMEM) >> 3, 0, GL_RGBA_INTEGER,
                 GL_SHORT, nullptr);

    WinObjOAMLocation = glGetUniformLocation(ObjProgram, "OAM");

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glBindTexture(GL_TEXTURE_2D, IOTexture);
    WinObjIOLocation = glGetUniformLocation(WinObjProgram, "IO");

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, VRAMSSBO);

    glUseProgram(WinObjProgram);
    glUniform1i(WinObjIOLocation, static_cast<u32>(BufferBindings::LCDIO));
    glUniform1i(WinObjOAMLocation, static_cast<u32>(BufferBindings::OAM));

    WinObjYClipStartLocation = glGetUniformLocation(WinObjProgram, "YClipStart");
    WinObjYClipEndLocation   = glGetUniformLocation(WinObjProgram, "YClipEnd");

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);

    log_debug("OpenGL error after Win Obj initialization: %x", glGetError());
}

void GBAPPU::VideoInit() {
    InitFramebuffers();
    log_debug("OpenGL error after framebuffer initialization: %x", glGetError());

    InitBGProgram();
    InitObjProgram();
    InitWinBGProgram();
    InitWinObjProgram();
    log_debug("OpenGL error after program initialization: %x", glGetError());

    InitBGBuffers();
    InitObjBuffers();
    InitWinBGBuffers();
    InitWinObjBuffers();
}

void GBAPPU::DrawBGWindow(int win_start, int win_end) const {
    glUseProgram(WinBGProgram);
    glBindVertexArray(WinBGVAO);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glBindTexture(GL_TEXTURE_2D, IOTexture);

    glBindBuffer(GL_ARRAY_BUFFER, BGVBO);
    const float quad[8] = {
            -1.0, (float)win_start,  // top left
             1.0, (float)win_start,  // top right
             1.0, (float)win_end,    // bottom right
            -1.0, (float)win_end,    // bottom left
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, 8 * sizeof(float), quad);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 8);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void GBAPPU::DrawObjWindow(int win_start, int win_end) {
    glUseProgram(WinObjProgram);
    glBindVertexArray(WinObjVAO);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glBindTexture(GL_TEXTURE_2D, IOTexture);

    glBindBuffer(GL_ARRAY_BUFFER, ObjVBO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ObjEBO);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xffff);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::OAM));

    size_t scanline = 0;
    u32 NumberOfObjVerts;
    do {
        u32 batch_size = ScanlineOAMBatchSizes[BufferFrame ^ 1][scanline];

        if (scanline < win_start) {
            scanline += batch_size;
        }
        else if (scanline > win_end) {
            break;
        }

        NumberOfObjVerts = BufferObjects<true>(BufferFrame ^ 1, scanline, batch_size);
        if (!NumberOfObjVerts) {
            scanline += batch_size;
            continue;
        }

        // bind and buffer OAM texture
        glBindTexture(GL_TEXTURE_1D, OAMTexture);
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, sizeof(OAMMEM) >> 3,
                        GL_RGBA_INTEGER, GL_SHORT, OAMBuffer[BufferFrame ^ 1][scanline]);

        glUniform1ui(WinObjYClipStartLocation, scanline);
        glUniform1ui(WinObjYClipEndLocation, scanline + batch_size);

        glBufferData(GL_ARRAY_BUFFER, sizeof(u64) * NumberOfObjVerts, ObjAttrBuffer, GL_STATIC_DRAW);
        // * 5 for the restartindex
        glDrawElements(GL_TRIANGLE_FAN, (NumberOfObjVerts >> 2) * 5, GL_UNSIGNED_SHORT, 0);

        log_ppu("%d objects enabled in lines %d - %d of object window", NumberOfObjVerts >> 2, scanline, scanline + batch_size);

        scanline += batch_size;

        log_ppu("%d Object window scanlines batched (accum %d)", batch_size, scanline);
        // should actually be !=, but just to be sure we don't ever get stuck
    } while (scanline < VISIBLE_SCREEN_HEIGHT);

    glDisable(GL_PRIMITIVE_RESTART);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_1D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void GBAPPU::DrawWindows() {
    // scanlines in which window is enabled first and last
    int win_start = -1;
    for (int line = 0; line < VISIBLE_SCREEN_HEIGHT; line++) {
        if (LCDIOBuffer[BufferFrame ^ 1][line][1] & 0xe0) {
            // top 3 bits of DISPCNT nonzero
            win_start = line;
            break;
        }
    }

    // render BG and object window
    glBindFramebuffer(GL_FRAMEBUFFER, WinFramebuffer);
    glViewport(0, 0, VISIBLE_SCREEN_WIDTH, VISIBLE_SCREEN_HEIGHT);

    // window is never enabled
    if (win_start < 0) {
        log_ppu("Window not enabled in frame");
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    else {
        int win_end = -1;
        for (int line = VISIBLE_SCREEN_HEIGHT - 1; line >= win_start; line--) {
            if (LCDIOBuffer[BufferFrame ^ 1][line][1] & 0xe0) {
                // top 3 bits of DISPCNT nonzero
                win_end = line;
                break;
            }
        }

        log_ppu("Window enabled from %d to %d", win_start, win_end);
        DrawBGWindow(win_start, win_end);
        DrawObjWindow(win_start, win_end);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBAPPU::DrawObjects(u32 scanline, u32 amount) {
    if (unlikely(!ExternalObjEnable)) {
        return;
    }

    u32 NumberOfObjVerts = BufferObjects<false>(BufferFrame ^ 1, scanline, amount);
    if (!NumberOfObjVerts) {
        return;
    }

    glUseProgram(ObjProgram);
    glBindVertexArray(ObjVAO);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::PAL));
    glBindTexture(GL_TEXTURE_2D, PALTexture);

    // bind and buffer OAM texture
    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::OAM));
    glBindTexture(GL_TEXTURE_1D, OAMTexture);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, sizeof(OAMMEM) >> 3,
                    GL_RGBA_INTEGER, GL_SHORT, OAMBuffer[BufferFrame ^ 1][scanline]);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glBindTexture(GL_TEXTURE_2D, IOTexture);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::Window));
    glBindTexture(GL_TEXTURE_2D, WinTexture);

    glBindBuffer(GL_ARRAY_BUFFER, ObjVBO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ObjEBO);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xffff);

    glUniform1ui(ObjYClipStartLocation, scanline);
    glUniform1ui(ObjYClipEndLocation, scanline + amount);

    glBufferData(GL_ARRAY_BUFFER, sizeof(u64) * NumberOfObjVerts, ObjAttrBuffer, GL_STATIC_DRAW);
    // * 5 for the restartindex
    glDrawElements(GL_TRIANGLE_FAN, (NumberOfObjVerts >> 2) * 5, GL_UNSIGNED_SHORT, 0);

    log_ppu("%d objects enabled in lines %d - %d", NumberOfObjVerts >> 2, scanline, scanline + amount);

    glDisable(GL_PRIMITIVE_RESTART);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_1D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void GBAPPU::DrawScanlines(u32 scanline, u32 amount) {
    s_UpdateRange range;
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
        if (FrameSkip) {
            // reset range data here if we are frameskipping
            VRAMRanges[BufferFrame ^ 1][scanline] = { .min = sizeof(VRAMMEM), .max = 0 };
        }
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

    // first render window, then actual backgrounds
    glUseProgram(BGProgram);
    glBindVertexArray(BGVAO);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::PAL));
    glBindTexture(GL_TEXTURE_2D, PALTexture);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::LCDIO));
    glBindTexture(GL_TEXTURE_2D, IOTexture);

    glActiveTexture(GL_TEXTURE0 + static_cast<u32>(BufferBindings::Window));
    glBindTexture(GL_TEXTURE_2D, WinTexture);

    glBindBuffer(GL_ARRAY_BUFFER, BGVBO);

    const float quad[8] = {
            -1.0, (float) scanline,            // top left
             1.0, (float) scanline,            // top right
             1.0, (float)(scanline + amount),  // bottom right
            -1.0, (float)(scanline + amount),  // bottom left
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, 8 * sizeof(float), quad);

    for (u32 BG = 0; BG <= 4; BG++) {
        if (likely(ExternalBGEnable[BG])) {
            // draw each of the backgrounds separately
            glUniform1ui(BGLocation, BG);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 8);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

struct s_framebuffer GBAPPU::Render() {

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_SCISSOR_TEST);

    if (FrameSkip) {
        DrawMutex.lock();
    }
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

    DrawWindows();

    // use normal framebuffer again
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);
    glViewport(0, 0, INTERNAL_FRAMEBUFFER_WIDTH, INTERNAL_FRAMEBUFFER_HEIGHT);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // buffer reference points for affine backgrounds
    glUniform1uiv(ReferenceLine2Location, VISIBLE_SCREEN_HEIGHT, ReferenceLine2Buffer[BufferFrame ^ 1]);
    glUniform1uiv(ReferenceLine3Location, VISIBLE_SCREEN_HEIGHT, ReferenceLine3Buffer[BufferFrame ^ 1]);

    size_t VRAM_scanline = 0, OAM_scanline = 0;
    do {
        if (VRAM_scanline <= OAM_scanline) {
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

    if (FrameSkip) {
        DrawMutex.unlock();
    }
    else {
        std::lock_guard<std::mutex> lock(DrawMutex);
        FrameDrawn = true;
        FrameDrawnVariable.notify_all();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    u16 backdrop = ReadArray<u16>(PALBuffer[BufferFrame ^ 1][0], 0);
    float r = (float)(backdrop & 0x1fu) / 31.0;
    float g = (float)((backdrop >> 5) & 0x1fu) / 31.0;
    float b = (float)((backdrop >> 10) & 0x1fu) / 31.0;

    // same color correction as in shader
    // results are slightly different somehow, but eh
    const float lcdGamma = 4.0;
    const float outGamma = 2.0;

    float lr = std::pow(r, lcdGamma);
    float lg = std::pow(g, lcdGamma);
    float lb = std::pow(b, lcdGamma);

    r = std::pow((  0 * lb +  50 * lg + 255 * lr) / 255.0, 1.0 / outGamma) * (255.0 / 280.0);
    g = std::pow(( 30 * lb + 230 * lg +  10 * lr) / 255.0, 1.0 / outGamma) * (255.0 / 280.0);
    b = std::pow((220 * lb +  10 * lg +  50 * lr) / 255.0, 1.0 / outGamma) * (255.0 / 280.0);

    return (s_framebuffer) {
            .id = Framebuffer,
            .src_width = INTERNAL_FRAMEBUFFER_WIDTH,
            .src_height = INTERNAL_FRAMEBUFFER_HEIGHT,
            .dest_width = VISIBLE_SCREEN_WIDTH,
            .dest_height = VISIBLE_SCREEN_HEIGHT,
            .r = r,
            .g = g,
            .b = b,
    };
}

struct s_framebuffer GBAPPU::RenderUntil(size_t ticks) {
    if (likely(SyncToVideo || FrameSkip)) {
        // only render once if synced
        return Render();
    }
    else {
        s_framebuffer framebuffer;
        size_t current_ticks;

        do {
            framebuffer = Render();

            current_ticks = SDL_GetTicks();
            std::unique_lock<std::mutex> lock(FrameWaitMutex);
            if ((current_ticks < ticks) && FrameReadyVariable.wait_for(
                    lock,
                    std::chrono::milliseconds(current_ticks - ticks),
                    [this]{ return FrameReady; }
            )) {
                FrameReady = false;
            }
            else {
                // timeout expired
                break;
            }
        } while (true);
        return framebuffer;
    }
}

void GBAPPU::VideoDestroy() {
    glDeleteBuffers(1, &VRAMSSBO);
    glDeleteTextures(1, &OAMTexture);
    glDeleteTextures(1, &PALTexture);
    glDeleteTextures(1, &IOTexture);
    glDeleteFramebuffers(1, &Framebuffer);
    glDeleteFramebuffers(1, &WinFramebuffer);
    glDeleteTextures(1, &WinTexture);
    glDeleteProgram(BGProgram);
    glDeleteProgram(ObjProgram);
    glDeleteProgram(WinBGProgram);
    glDeleteProgram(WinObjProgram);
}
