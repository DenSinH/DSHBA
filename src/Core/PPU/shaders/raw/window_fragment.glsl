// BEGIN WindowFragmentShaderSource

#version 430 core

in vec2 screenCoord;

out uvec4 FragColor;
out float gl_FragDepth;

uint readVRAM8(uint address);
uint readVRAM16(uint address);
uint readVRAM32(uint address);

uint readIOreg(uint address);
vec4 readPALentry(uint index);

void main() {
    uint DISPCNT = readIOreg(++DISPCNT++);

    if ((DISPCNT & 0xe000u) == 0) {
        // windows are disabled, enable all windows
        // we should have caught this before rendering, but eh, I guess we'll check again...
        FragColor.x = 0x3f;
        gl_FragDepth = -1.0;
        return;
    }

    uint x = uint(screenCoord.x);
    uint y = uint(screenCoord.y);

    // window 0 has higher priority
    for (uint window = 0; window < 2; window++) {
        if ((DISPCNT & (++DisplayWin0++ << window)) == 0) {
            // window disabled
            continue;
        }

        uint WINH = readIOreg(++WIN0H++ + 2 * window);
        uint WINV = readIOreg(++WIN0V++ + 2 * window);
        uint WININ = (readIOreg(++WININ++) >> (window * 8)) & 0x3fu;

        uint X1 = WINH >> 8;
        uint X2 = WINH & 0xffu;
        if (X2 > ++VISIBLE_SCREEN_WIDTH++) {
            X2 = ++VISIBLE_SCREEN_WIDTH++;
        }

        uint Y1 = WINV >> 8;
        uint Y2 = WINV & 0xffu;

        if (Y1 <= Y2) {
            // no vert wrap and out of bounds, continue
            if (y < Y1 || y > Y2) {
                continue;
            }
        }
        else {
            // vert wrap and "in bounds":
            if ((y < Y1) && (y > Y2)) {
                continue;
            }
        }

        if (X1 <= X2) {
            // no hor wrap
            if (x >= X1 && x < X2) {
                // pixel in WININ
                FragColor.x = WININ;
                gl_FragDepth = 0.0;
                return;
            }
        }
        else {
            // hor wrap
            if (x < X2 || x >= X1) {
                // pixel in WININ
                FragColor.x = WININ;
                gl_FragDepth = 0.0;
                return;
            }
        }
    }

    FragColor.x = readIOreg(++WINOUT++) & 0x3fu;  // WINOUT
    gl_FragDepth = -1.0;
}

// END WindowFragmentShaderSource