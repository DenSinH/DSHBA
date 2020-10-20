#include "scheduler.h"
#include "log.h"

#define LEFT(index) (((index) << 1) + 1)
#define RIGHT(index) (((index) << 1) + 2)
#define PARENT(index) ((index) >> 1)
#define ROOT 0

static inline void swap(s_event* pointers[], size_t i, size_t j) {
    void* temp = pointers[i];
    pointers[i] = pointers[j];
    pointers[j] = temp;
}

void trickle_up(s_scheduler* scheduler, size_t index) {
    // head element
    if (!index) return;

    s_event* event = scheduler->events[index];
    s_event* parent = scheduler->events[PARENT(index)];
    if (event->time < parent->time) {
        swap(scheduler->events, index, PARENT(index));
        trickle_up(scheduler, PARENT(index));
    }
}

void trickle_down(s_scheduler* scheduler, size_t index) {
    // bottom of heap
    size_t child = LEFT(index);
    if (child >= scheduler->count) return;

    s_event* event = scheduler->events[index];
    if (child + 1 < scheduler->count && scheduler->events[child]->time > scheduler->events[child + 1]->time) {
        // right child is smallest
        child++;
    }

    if (scheduler->events[child]->time < event->time) {
        // swap parent with smallest child if child is smaller than parent
        swap(scheduler->events, index, child);
        trickle_down(scheduler, child);
    }
}

void add_event(s_scheduler* scheduler, s_event* event) {
    log_sched("add event at time %lld (now %d events in queue)", event->time, scheduler->count);
    scheduler->events[scheduler->count] = event;
    event->active = true;
    trickle_up(scheduler, scheduler->count++);
}

s_event* pop_event(s_scheduler* scheduler) {
    if (scheduler->count) {
        s_event* first = scheduler->events[ROOT];
        scheduler->events[ROOT] = scheduler->events[--scheduler->count];
        trickle_down(scheduler, ROOT);
        return first;
    }
    return NULL;
}

size_t find_event(s_scheduler* scheduler, s_event* event, size_t index) {
    // index is the index we are currently checking
    if (scheduler->events[index] == event) {
        // found the node
        return index;
    }
    else if (scheduler->events[index]->time <= event->time) {
        // search deeper
        if (RIGHT(index) < scheduler->count) {
            size_t found = find_event(scheduler, event, LEFT(index));
            if (found != (size_t)-1) {
                return found;
            }
            return find_event(scheduler, event, RIGHT(index));
        }
        else if (LEFT(index) < scheduler->count) {
            return find_event(scheduler, event, LEFT(index));
        }
    }
    // not found
    return (size_t)-1;
}

void remove_event(s_scheduler* scheduler, s_event* event) {
    // swap with last then heapify
    size_t index = find_event(scheduler, event, ROOT);
    if (index != (size_t)-1) {
        // event was found
        log_sched("Remove event (time = %lld) at %d", event->time, index);
        event->active = false;
        swap(scheduler->events, index, --scheduler->count);

        if (scheduler->events[index]->time < scheduler->events[PARENT(index)]->time) {
            // swapped event was placed below later event
            trickle_up(scheduler, index);
        }
        else {
            // if it wasn't placed below a later event, it might be placed above a sooner event
            trickle_down(scheduler, index);
        }
    }
    else {
        log_sched("could not find event to be removed (time %lld)", event->time);
    }
}

void reschedule_event(s_scheduler* scheduler, s_event* event, u64 new_time) {
    // hard reschedule (probably faster if the event changed a lot)
    remove_event(scheduler, event);
    event->time = new_time;
    event->active = true;
    add_event(scheduler, event);
}

void change_event(s_scheduler* scheduler, s_event* event, u64 new_time) {
    // trickle the event down or up (probably faster if the event did not change by much)
    size_t index = find_event(scheduler, event, ROOT);
    if (index != (size_t)-1) {
        // event was found
        log_sched("Change event (time = %lld) at %d", event->time, index);
        event->time = new_time;
        if (new_time < scheduler->events[PARENT(index)]->time) {
            // swapped event was placed below later event
            trickle_up(scheduler, index);
        }
        else {
            // if it wasn't placed below a later event, it might be placed above a sooner event
            trickle_down(scheduler, index);
        }
    }
    else {
        log_warn("could not find changed event (time %lld)", event->time);
        event->time = new_time;
        add_event(scheduler, event);
        event->active = true;
    }
}

void delay_event_by(s_scheduler* scheduler, s_event* event, uint64_t dt) {
    // event was delayed, so old time < new time, we find it with the old time anyway
    size_t index = find_event(scheduler, event, ROOT);
    if (index != (size_t)-1) {
        // event was found, we know it has been delayed
        log_sched("Delay event (time = %lld) at %d", event->time, index);
        event->time += dt;
        trickle_down(scheduler, index);
    }
    else {
        log_warn("could not find delayed event (time %lld)", event->time);
        event->time += dt;
        add_event(scheduler, event);
        event->active = true;
    }
}

void hasten_event_by(s_scheduler* scheduler, s_event* event, u64 dt) {
    size_t index = find_event(scheduler, event, ROOT);
    if (index != (size_t)-1) {
        // event was found, we know it has been hastened
        log_sched("Hasten event (time = %lld) at %d", event->time, index);
        event->time -= dt;
        trickle_up(scheduler, index);
    }
    else {
        log_warn("could not find hastened event (time %lld)", event->time);
        event->time -= dt;
        add_event(scheduler, event);
        event->active = true;
    }
}

void do_events(s_scheduler* scheduler) {
    s_event* first = scheduler->events[ROOT];
    while (scheduler->count > 0 && first->time < *scheduler->timer) {
        first->active = false;
        first->callback(first->caller, first, scheduler);

        scheduler->events[ROOT] = scheduler->events[--scheduler->count];
        trickle_down(scheduler, ROOT);

        // new first element
        first = scheduler->events[ROOT];
    }
}
