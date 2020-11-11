// defined externally
#version 430 core

// BEGIN ObjectVertexShaderSource

#define attr0 x
#define attr1 y
#define attr2 z
#define attr3 w

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

const s_ObjSize ObjSizeTable[3][4] = {
    { s_ObjSize(8, 8),  s_ObjSize(16, 16), s_ObjSize(32, 32), s_ObjSize(64, 64) },
    { s_ObjSize(16, 8), s_ObjSize(32, 8),  s_ObjSize(32, 16), s_ObjSize(64, 32) },
    { s_ObjSize(8, 16), s_ObjSize(8, 32),  s_ObjSize(16, 32), s_ObjSize(32, 62) }
};

struct s_Position {
    bool right;
    bool low;
};

const s_Position PositionTable[4] = {
    s_Position(false, false), s_Position(true, false), s_Position(true, true), s_Position(false, true)
};

void main() {
    OBJ = InOBJ;
    s_Position Position = PositionTable[gl_VertexID & 3];

    uint Shape = OBJ.attr0 >> 14;
    uint Size  = OBJ.attr1 >> 14;

    s_ObjSize ObjSize = ObjSizeTable[Shape][Size];
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

        if ((OBJ.attr0 & ++ATTR0_OM++) == ++ATTR0_AFF_DBL++) {
            // double rendering
            InObjPos.x  += ObjWidth;
            ScreenPos.x += int(ObjWidth);
        }
    }

    if (Position.low) {
        InObjPos.y  += ObjHeight;
        ScreenPos.y += int(ObjHeight);

        if ((OBJ.attr0 & ++ATTR0_OM++) == ++ATTR0_AFF_DBL++) {
            // double rendering
            InObjPos.y  += ObjHeight;
            ScreenPos.y += int(ObjHeight);
        }
    }

    // flipping only applies to regular sprites
    if ((OBJ.attr0 & ++ATTR0_OM++) == ++ATTR0_REG++) {
        if ((OBJ.attr1 & ++ATTR1_VF++) != 0) {
            // VFlip
            InObjPos.y = ObjHeight - InObjPos.y;
        }

        if ((OBJ.attr1 & ++ATTR1_HF++) != 0) {
            // HFlip
            InObjPos.x = ObjWidth - InObjPos.x;
        }
    }

    OnScreenPos = vec2(ScreenPos);
    gl_Position = vec4(
        -1.0 + 2.0 * OnScreenPos.x / float(++VISIBLE_SCREEN_WIDTH++),
        1 - 2.0 * OnScreenPos.y / float(++VISIBLE_SCREEN_HEIGHT++),
        0,
        1
    );
}

// END ObjectVertexShaderSource