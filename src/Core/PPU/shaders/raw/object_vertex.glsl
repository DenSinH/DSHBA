// defined externally
#version 330 core

// BEGIN ObjectVertexShaderSource

#define attr0 x
#define attr1 y
#define attr2 z
#define attr3 w

uniform bool Affine;

layout (location = 0) in uvec4 InOBJ;

out vec2 InObjPos;
out vec2 OnScreenPos;
flat out uvec4 OBJ;
flat out uint ObjWidth;
flat out uint ObjHeight;

struct s_ObjSize {
    uint width;
    uint height;
};

const s_ObjSize ObjSizeTable[12] = s_ObjSize[](
    s_ObjSize(8u, 8u),  s_ObjSize(16u, 16u), s_ObjSize(32u, 32u), s_ObjSize(64u, 64u),
    s_ObjSize(16u, 8u), s_ObjSize(32u, 8u),  s_ObjSize(32u, 16u), s_ObjSize(64u, 32u),
    s_ObjSize(8u, 16u), s_ObjSize(8u, 32u),  s_ObjSize(16u, 32u), s_ObjSize(32u, 62u)
);

struct s_Position {
    bool right;
    bool low;
};

const s_Position PositionTable[4] = s_Position[](
    s_Position(false, false), s_Position(true, false), s_Position(true, true), s_Position(false, true)
);

void main() {
    OBJ = InOBJ;
    s_Position Position = PositionTable[gl_VertexID & 3];

    uint Shape = OBJ.attr0 >> 14;
    uint Size  = OBJ.attr1 >> 14;

    s_ObjSize ObjSize = ObjSizeTable[(Shape * 4u) + Size];
    ObjWidth = ObjSize.width;
    ObjHeight = ObjSize.height;

    ivec2 ScreenPos = ivec2(OBJ.attr1 & 0x1ffu, OBJ.attr0 & 0xffu);

    // correct position for screen wrapping
    if (ScreenPos.x > int(++VISIBLE_SCREEN_WIDTH++)) {
        ScreenPos.x -= 0x200;
    }

    if (ScreenPos.y > int(++VISIBLE_SCREEN_HEIGHT++)) {
        ScreenPos.y -= 0x100;
    }

    InObjPos = uvec2(0, 0);
    if (Position.right) {
        InObjPos.x  += ObjWidth;
        ScreenPos.x += int(ObjWidth);

        if (Affine) {
            if ((OBJ.attr0 & ++ATTR0_OM++) == ++ATTR0_AFF_DBL++) {
                // double rendering
                InObjPos.x  += ObjWidth;
                ScreenPos.x += int(ObjWidth);
            }
        }
    }

    if (Position.low) {
        InObjPos.y  += ObjHeight;
        ScreenPos.y += int(ObjHeight);

        if (Affine) {
            if ((OBJ.attr0 & ++ATTR0_OM++) == ++ATTR0_AFF_DBL++) {
                // double rendering
                InObjPos.y  += ObjHeight;
                ScreenPos.y += int(ObjHeight);
            }
        }
    }

    // flipping only applies to regular sprites
    if (!Affine) {
        if ((OBJ.attr1 & ++ATTR1_VF++) != 0u) {
            // VFlip
            InObjPos.y = ObjHeight - InObjPos.y;
        }

        if ((OBJ.attr1 & ++ATTR1_HF++) != 0u) {
            // HFlip
            InObjPos.x = ObjWidth - InObjPos.x;
        }
    }

    OnScreenPos = vec2(ScreenPos);

#ifndef OBJ_WINDOW
    // depth is the same everywhere in the object anyway
    uint Priority = (OBJ.attr2 & 0x0c00u) >> 10;

    gl_Position = vec4(
        -1.0 + 2.0 * OnScreenPos.x / float(++VISIBLE_SCREEN_WIDTH++),
        1 - 2.0 * OnScreenPos.y / float(++VISIBLE_SCREEN_HEIGHT++),
        -1 + float(Priority) / 2.0,  // /2.0 because openGL clips between -1 and 1 (-1 is in front)
        1
    );
#else
    gl_Position = vec4(
        -1.0 + 2.0 * OnScreenPos.x / float(++VISIBLE_SCREEN_WIDTH++),
        1 - 2.0 * OnScreenPos.y / float(++VISIBLE_SCREEN_HEIGHT++),
        0.5,  // between WIN1 and WINOUT
        1
    );
#endif
}

// END ObjectVertexShaderSource