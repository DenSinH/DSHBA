#ifndef GC__SCHEDULER_H
#define GC__SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "default.h"
#include "flags.h"
#include <stdbool.h>

#undef CHECK_EVENT_ACTIVE

#define SCHEDULER_MAX_EVENTS 64

typedef struct s_scheduler s_scheduler;

// todo: string field in event to view top event name in debugger?
#define SCHEDULER_EVENT(name) void name(void* caller, struct s_event* event, s_scheduler* scheduler)

typedef struct s_event {
    SCHEDULER_EVENT((*callback));

    void *caller;
    u64 time;
    bool active;   // signifies if event is in the scheduler or not
} s_event;

/*
 * Idea: make a heap structure, add events sorted on time, remove events sorted on time, then check if callback is equal
 * (function pointers can be compared anyway)
 * */
void add_event(s_scheduler *scheduler, s_event *event);
void remove_event(s_scheduler *scheduler, s_event *event);
void reschedule_event(s_scheduler *scheduler, s_event *event, u64 new_time);
void do_events(s_scheduler* scheduler);
u64 peek_event(s_scheduler* scheduler);
u64 get_time(s_scheduler* scheduler);
void set_time(s_scheduler* scheduler, u64 time);
s_scheduler* create_scheduler(u64* timer);

bool should_do_events(s_scheduler *scheduler);

#ifdef __cplusplus
};
#endif

#endif //GC__SCHEDULER_H
