#include "controller.h"

#include <sstream>
#include <fstream>
#include <string>
#include <regex>
#include <map>

#define CONTROLLER_AXIS_BIT 0x8000
static std::map<uint16_t, e_controller_input> controller_map;
static std::map<int, e_controller_input> keyboard_map;

static bool set_button(s_controller* controller, e_controller_input button, uint8_t value) {
    switch (button) {
        case cinp_A:
            controller->A = value;
            break;
        case cinp_B:
            controller->B = value;
            break;
        case cinp_X:
            controller->X = value;
            break;
        case cinp_Y:
            controller->Y = value;
            break;
        case cinp_up:
            controller->up = value;
            break;
        case cinp_down:
            controller->down = value;
            break;
        case cinp_left:
            controller->left = value;
            break;
        case cinp_right:
            controller->right = value;
            break;
        case cinp_start:
            controller->start = value;
            break;
        case cinp_select:
            controller->select = value;
            break;
        case cinp_L:
            controller->L = value;
            break;
        case cinp_R:
            controller->R = value;
            break;
        default:
            // others are axis inputs
            return false;
    }
    return true;
}

static void set_axis(s_controller* controller, e_controller_input axis, int16_t value) {
    switch (axis) {
        case cinp_left_x:
            controller->left_x = value;
            break;
        case cinp_left_x_neg:
            controller->left_x = -value;
            break;
        case cinp_left_y:
            controller->left_y = value;
            break;
        case cinp_left_y_neg:
            controller->left_y = -value;
            break;
        case cinp_right_x:
            controller->right_x = value;
            break;
        case cinp_right_x_neg:
            controller->right_x = -value;
            break;
        case cinp_right_y:
            controller->right_y = value;
            break;
        case cinp_right_y_neg:
            controller->right_y = -value;
            break;
        default:
            break;
    }
}

void parse_keyboard_input(s_controller* controller, int code, uint8_t down) {
    if (keyboard_map.find(code) != keyboard_map.end()) {
        if (!set_button(controller, keyboard_map[code], down)) {
            // input was axis
            set_axis(controller, keyboard_map[code], down ? INT16_MAX : 0); // always either max or minimum offset
        }
    }
}

void parse_controller_button(s_controller* controller, uint16_t code, uint8_t down) {
    if (controller_map.find(code) != controller_map.end()) {
        set_button(controller, controller_map[code], down);
    }
}

void parse_controller_axis(s_controller* controller, uint16_t code, int16_t value) {
    code |= CONTROLLER_AXIS_BIT;
    if (controller_map.find(code) != controller_map.end()) {
        set_axis(controller, controller_map[code], value);
    }
}

/* parsing the input map file (input.map by default) */
typedef enum e_scan_types {
    BUTTON, AXIS
} e_scan_types;

typedef enum e_scan_modes {
    KEYBOARD, CONTROLLER
} e_scan_modes;

template<typename T>
static void set_input(std::map<T, e_controller_input>& map, e_scan_types type, const std::string& button, T key) {
    // printf("Set mapping for key %s to %d\n", button.c_str(), key);

    if (type == BUTTON) {
        if (button == "A")           map[key] = cinp_A;
        else if (button == "B")      map[key] = cinp_B;
        else if (button == "X")      map[key] = cinp_X;
        else if (button == "Y")      map[key] = cinp_Y;
        else if (button == "UP")     map[key] = cinp_up;
        else if (button == "DOWN")   map[key] = cinp_down;
        else if (button == "LEFT")   map[key] = cinp_left;
        else if (button == "RIGHT")  map[key] = cinp_right;
        else if (button == "L")      map[key] = cinp_L;
        else if (button == "R")      map[key] = cinp_R;
        else if (button == "START")  map[key] = cinp_start;
        else if (button == "SELECT") map[key] = cinp_select;
        else printf("Invalid button name for button: %s\n", button.c_str());
    }
    else {
        // axes
        if (button == "LEFT_X")           map[key] = cinp_left_x;
        else if (button == "LEFT_X_NEG")  map[key] = cinp_left_x_neg;
        else if (button == "LEFT_Y")      map[key] = cinp_left_y;
        else if (button == "LEFT_Y_NEG")  map[key] = cinp_left_y_neg;
        else if (button == "RIGHT_X")     map[key] = cinp_right_x;
        else if (button == "RIGHT_X_NEG") map[key] = cinp_right_x_neg;
        else if (button == "RIGHT_Y")     map[key] = cinp_right_y;
        else if (button == "RIGHT_Y_NEG") map[key] = cinp_right_y_neg;
        else printf("Invalid axis name for button: %s\n", button.c_str());
    }
}

void init_controller_mapping(const char* file_name) {
    e_scan_types type = BUTTON;  // scan for buttons by default
    e_scan_modes mode = CONTROLLER;
    std::string line;
    std::ifstream file(file_name);

    if (file.fail()) {
        printf("Could not open controller map file\n");
        return;
    }

    const std::regex mode_regex(R"(^\s*\[(KEYBOARD|CONTROLLER)\]\s*$)", std::regex_constants::icase);
    const std::regex type_regex(R"(^\s*\[(BUTTON(?:S)|AX(E|I)S)\]\s*$)", std::regex_constants::icase);
    const std::regex controller_input_regex(R"(^\s*(\w+)\s*=\s*(\d+)\s*(?://.*)?$)");
    const std::regex keyboard_input_regex(R"(^\s*(\w+)\s*=\s*(0x[a-f\d]+)\s*(?://.*)?$)");
    const std::regex empty_regex(R"(^\s*(?://.*)?$)");

    std::smatch match;

    while (std::getline(file, line)) {

        if (std::regex_match(line, match, mode_regex)) {
            if (tolower(match[1].str().at(0)) == 'k') {
                // keyboard mode_regex
                mode = KEYBOARD;
            }
            else {
                mode = CONTROLLER;
            }
        }
        else if (std::regex_match(line, match, type_regex)) {
            // enter buttons or axes mode
            // we just check the first character of the identifier
            type = tolower(match[1].str().at(0)) == 'b' ? BUTTON : AXIS;
        }
        else if ((mode == CONTROLLER && std::regex_match(line, match, controller_input_regex)) ||
                 (mode == KEYBOARD   && std::regex_match(line, match, keyboard_input_regex))) {
            // parse a line of input
            std::string button = match[1].str();

            // transform to uppercase to match easier
            std::transform(button.begin(), button.end(), button.begin(), ::toupper);

            if (mode == CONTROLLER) {
                uint16_t id = std::stoi(match[2].str());
                if (type == AXIS) {
                    id |= CONTROLLER_AXIS_BIT;
                }

                set_input(controller_map, type, button, id);
            }
            else {
                int key = std::stoi(match[2].str(), nullptr, 16);
                set_input(keyboard_map, type, button, key);
            }
        }
        else if (std::regex_match(line, match, empty_regex)) {
            // empty lines are okay
        }
        else {
            printf("Invalid syntax in line: %s\n", line.c_str());
        }
    }
}