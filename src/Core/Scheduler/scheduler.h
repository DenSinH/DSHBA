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

typedef struct s_scheduler {
    struct s_event *events[SCHEDULER_MAX_EVENTS];
    size_t count;
    u64 *timer;
} s_scheduler;

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
s_event* pop_event(s_scheduler *scheduler);  // mostly only used for debugging the scheduler
void remove_event(s_scheduler *scheduler, s_event *event);
void reschedule_event(s_scheduler *scheduler, s_event *event, u64 new_time);
void change_event(s_scheduler *scheduler, s_event *event, u64 new_time);
void delay_event_by(s_scheduler *scheduler, s_event *event, uint64_t dt);
void hasten_event_by(s_scheduler *scheduler, s_event *event, uint64_t dt);
void do_events(s_scheduler *scheduler);
u64 peek_event(s_scheduler *scheduler);

static inline bool should_do_events(s_scheduler *scheduler) {
    return scheduler->events[0]->time < *scheduler->timer;
}


#ifdef __cplusplus
};
#endif

#endif //GC__SCHEDULER_H
