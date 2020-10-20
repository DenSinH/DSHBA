#include "breakpoints.h"

#include "log.h"

void add_breakpoint(s_breakpoints* breakpoints, u32 value) {
    // add breakpoint at location
    if (breakpoints->number_of_breakpoints == MAX_BREAKPOINTS) {
        log_fatal("Max number of breakpoints reached");
    }

    // check if it already exists
    for (int i = 0; i < breakpoints->number_of_breakpoints; i++) {
        if (value == breakpoints->breakpoints[i]) {
            return;
        }
    }

    breakpoints->breakpoints[breakpoints->number_of_breakpoints++] = value;
}

void remove_breakpoint(s_breakpoints* breakpoints, u32 value) {
    // remove breakpoint from location
    for (int i = 0; i < breakpoints->number_of_breakpoints; i++) {
        if (value == breakpoints->breakpoints[i]) {
            breakpoints->breakpoints[i] = breakpoints->breakpoints[--breakpoints->number_of_breakpoints];
            return;
        }
    }
}

bool check_breakpoints(s_breakpoints* breakpoints, u32 value) {
    // check if we need to break (this is a very slow way to do it, but we disable this in release mode anyway)
    for (int i = 0; i < breakpoints->number_of_breakpoints; i++) {
        if (value == breakpoints->breakpoints[i]) {
            return true;
        }
    }
    return false;
}