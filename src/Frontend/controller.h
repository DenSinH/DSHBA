#ifndef GC__CONTROLLER_H
#define GC__CONTROLLER_H

#include "interface.h"

#include <stdint.h>

/*
 * Idea: create a file with entries like:
 * A = 0
 * B = 1
 * axis_left = 0
 *
 * Which would map out the controller to A <-> button 0, B <-> button 1, axis_left <-> axis 0
 *
 * Then we can poll the controller struct to this mapping. In the emulator program, we can then parse the controller
 * struct to output the desired values
 * */

// custom controller struct to access from emulator
typedef enum e_controller_input {
    cinp_A, cinp_B, cinp_X, cinp_Y,
    cinp_up, cinp_down, cinp_left, cinp_right,
    cinp_start, cinp_select,
    cinp_L, cinp_R,
    cinp_left_x, cinp_left_y, cinp_right_x, cinp_right_y,

    // for keyboard we need 2 buttons per axis:
    cinp_left_x_neg, cinp_left_y_neg, cinp_right_x_neg, cinp_right_y_neg
} e_controller_input;

void init_controller_mapping(const char* file_name);
void parse_keyboard_input(s_controller* controller, int code, uint8_t down);
void parse_controller_button(s_controller* controller, uint16_t code, uint8_t down);
void parse_controller_axis(s_controller* controller, uint16_t code, int16_t value);

#endif //GC__CONTROLLER_H
