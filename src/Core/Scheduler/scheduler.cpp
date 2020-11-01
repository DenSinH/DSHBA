#include "scheduler.h"
#include "scheduler_impl.h"

extern "C" {
void add_event(s_scheduler *scheduler, s_event *event) {
    scheduler->AddEvent(event);
}

void remove_event(s_scheduler *scheduler, s_event *event) {
    scheduler->RemoveEvent(event);
}

void reschedule_event(s_scheduler *scheduler, s_event *event, u64 new_time) {
    scheduler->RescheduleEvent(event, new_time);
}

void do_events(s_scheduler* scheduler) {
    scheduler->DoEvents();
}

u64 peek_event(s_scheduler* scheduler) {
    return scheduler->PeekEvent();
}

u64 get_time(s_scheduler* scheduler) {
    return *scheduler->timer;
}

void set_time(s_scheduler* scheduler, u64 time) {
    *scheduler->timer = time;
}

s_scheduler* create_scheduler(u64* timer) {
    return new s_scheduler(timer);
}

bool should_do_events(s_scheduler *scheduler) {
    return scheduler->ShouldDoEvents();
}

}