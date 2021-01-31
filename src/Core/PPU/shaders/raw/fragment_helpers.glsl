// BEGIN FragmentHelperSource

#version 330 core

/* GENERAL */
in vec2 OnScreenPos;

uniform sampler2D PAL;
uniform usampler2D VRAM;
uniform usampler2D IO;
uniform isampler1D OAM;
uniform usampler2D Window;

uniform bool Bottom;

uniform int PALBufferIndex[++VISIBLE_SCREEN_HEIGHT++];

uint readIOreg(uint address);

// algorithm from https://byuu.net/video/color-emulation/
const float lcdGamma = 4.0;
const float outGamma = 2.2;
const mat3x3 CorrectionMatrix = mat3x3(
        255.0,  10.0,  50.0,
         50.0, 230.0,  10.0,
          0.0,  30.0, 220.0
) / 255.0;

vec4 ColorCorrect(vec4 color) {
    vec3 lrgb = pow(color.rgb, vec3(lcdGamma));
    vec3 rgb = pow(CorrectionMatrix * lrgb, vec3(1.0 / outGamma)) * (255.0 / 280.0);
    return vec4(rgb, color.a);
}

void CheckBottom(uint layer, uint window) {
    if (Bottom) {
        uint BLDCNT = readIOreg(++BLDCNT++);
        if (((BLDCNT & 0x00c0u) != 0x0040u)) {
            // not interesting, not normal alpha blending
            discard;
        }

        if ((window & 0x20u) == 0u) {
            // blending disabled in window, don't render on bottom frame
            discard;
        }
    }
}

vec4 AlphaCorrect(vec4 color, uint layer, uint window) {
    // BG0-3, 4 for Obj, 5 for BD
    if ((window & 0x20u) == 0u) {
        // blending disabled in window
        return vec4(color.rgb, -1);
    }

    uint BLDCNT = readIOreg(++BLDCNT++);
    uint BLDY = clamp(readIOreg(++BLDY++) & 0x1fu, 0u, 16u);

    switch (BLDCNT & 0x00c0u) {
        case 0x0000u:
            // blending disabled
            return vec4(color.rgb, -1);
        case 0x0040u:
            // normal blending, do this after (most complicated)
            break;
        case 0x0080u:
        {
            // blend A with white
            if ((BLDCNT & (1u << layer)) != 0u) {
                // layer is top layer
                return vec4(mix(color.rgb, vec3(1), float(BLDY) / 16.0), -1.0);
            }
            // bottom layer, not blended
            return vec4(color.rgb, -1);
        }
        case 0x00c0u:
        {
            // blend A with black
            if ((BLDCNT & (1u << layer)) != 0u) {
                // layer is top layer
                return vec4(mix(color.rgb, vec3(0), float(BLDY) / 16.0), -1.0);
            }
            // bottom layer, not blended
            return vec4(color.rgb, -1);
        }
    }

    // value was not normal blending / fade
    uint BLDALPHA = readIOreg(++BLDALPHA++);
    uint BLD_EVA = clamp(BLDALPHA & 0x1fu, 0u, 16u);
    uint BLD_EVB = clamp((BLDALPHA >> 8u) & 0x1fu, 0u, 16u);

    if ((BLDCNT & (1u << layer)) != 0u) {
        // top layer
        if (!Bottom) {
            return vec4(color.rgb, float(BLD_EVA) / 16.0);
        }
        else {
            discard;
        }
    }
    // bottom layer
    if ((BLDCNT & (0x100u << layer)) != 0u) {
        // set alpha value to -half of the actual value
        // -1 means: final color
        // negative: bottom
        // positive: top
        return vec4(color.rgb, -0.25 - (float(BLD_EVB) / 32.0));
    }

    // neither
    return vec4(color.rgb, -1);
}

uint readVRAM8(uint address) {
    return texelFetch(
        VRAM, ivec2(address & 0x7fu, address >> 7u), 0
    ).x;
}

uint readVRAM16(uint address) {
    address &= ~1u;
    uint lsb = readVRAM8(address);
    return lsb | (readVRAM8(address + 1u) << 8u);
}

uint readVRAM32(uint address) {
    address &= ~3u;
    uint lsh = readVRAM16(address);
    return lsh | (readVRAM16(address + 2u) << 16u);
}

uint readIOreg(uint address) {
    return texelFetch(
        IO, ivec2(address >> 1u, uint(OnScreenPos.y)), 0
    ).x;
}

ivec4 readOAMentry(uint index) {
    return texelFetch(
        OAM, int(index), 0
    );
}

vec4 readPALentry(uint index) {
    // Conveniently, since PAL stores the converted colors already, getting a color from an index is as simple as this:
    return texelFetch(
        PAL, ivec2(index, PALBufferIndex[uint(OnScreenPos.y)]), 0
    );
}

uint getWindow(uint x, uint y) {
    return texelFetch(
        Window, ivec2(x, ++VISIBLE_SCREEN_HEIGHT++ - y), 0
    ).r;
}

// END FragmentHelperSource