// BEGIN ObjectVertexShaderSource

#version 430 core

#define attr0 x
#define attr1 y
#define attr2 z
#define attr3 w

layout (location = 0) in uint ObjAttr0;
layout (location = 1) in uint ObjAttr1;

out vec2 InObjPos;
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
    s_Position Position = PositionTable[gl_VertexID & 3];

    uint Shape = ObjAttr0 >> 14;
    uint Size  = ObjAttr1 >> 14;

    s_ObjSize ObjSize = ObjSizeTable[Shape][Size];
    ObjWidth = ObjSize.width;
    ObjHeight = ObjSize.height;

    ivec2 ScreenPos = ivec2(ObjAttr1 & 0x1ffu, ObjAttr0 & 0xffu);
    InObjPos = uvec2(0, 0);
    if (Position.right) {
        InObjPos.x  += ObjWidth;
        ScreenPos.x += int(ObjWidth);
    }

    if (Position.low) {
        InObjPos.y  += ObjHeight;
        ScreenPos.y += int(ObjHeight);
    }

    // todo: if mirrored: ObjWidth - InObjPos.x

    gl_Position = vec4(
        -1.0 + 2.0 * float(ScreenPos.x) / float(++VISIBLE_SCREEN_WIDTH++),
        1.0 - 2.0 * float(ScreenPos.y) / float(++VISIBLE_SCREEN_HEIGHT++),
        0, 1
    );
}

// END ObjectVertexShaderSource