#ifndef GC__SCHEDULER_H
#define GC__SCHEDULER_H

#include "default.h"
#include "flags.h"
#include "helpers.h"

#include <functional>
#include <queue>

#include "log.h"

#undef CHECK_EVENT_ACTIVE

#define SCHEDULER_MAX_EVENTS 64

struct s_scheduler;
struct s_event;

// todo: string field in event to view top event name in debugger?
#define SCHEDULER_EVENT(name) void name(void* caller, s_event* event, s_scheduler* scheduler)

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

/*
 * C++ implementation using std::heap
 * */

static auto cmp = [](s_event* left, s_event* right) {
    return left->time > right->time;
};

template<
        typename T,
        class Container=std::vector<T>,
        class Compare=std::less<typename Container::value_type>
>
class deleteable_priority_queue : public std::priority_queue<T, Container, Compare> {

public:

    bool remove(const T &value) {
        auto it = std::find(this->c.begin(), this->c.end(), value);
        if (it != this->c.end()) {
            this->c.erase(it);
            std::make_heap(this->c.begin(), this->c.end(), this->comp);
            return true;
        } else {
            log_fatal("Failed to remove event");
            return false;
        }
    };
};

// we know that there is always _something_ in the queue
struct s_scheduler {

    deleteable_priority_queue<s_event*, std::vector<s_event*>, decltype(cmp)> queue;
    u64* timer;
    u64 top;

    // event to put in queue to make sure it is not empty
    inline static s_event infty = (s_event) {
        .callback = [](void* caller, s_event* event, s_scheduler* scheduler) {
            return;
        },
        .time = (u64)-1
    };

    explicit s_scheduler(u64* timer) {
        this->timer = timer;

        this->Reset();
    }

    void Reset() {
        // clear queue (fill queue with initial event)
        queue = {};
        queue.push(&infty);
        top = (u64)-1;
    }

    void AddEvent(s_event* event) {
        if (likely(!event->active)) {
            event->active = true;
            queue.push(event);
            top = min(top, event->time);
        }
    }

    void AddEventAfter(s_event* event, u64 dt) {
        if (likely(!event->active)) {
            event->time = *timer + dt;
            event->active = true;
            queue.push(event);
            top = min(top, event->time);
        }
        else {
            RemoveEvent(event);
            AddEventAfter(event, dt);
        }
    }

    void RemoveEvent(s_event* event) {
        queue.remove(event);
        event->active = false;

        // top might have changed
        top = queue.top()->time;
    }

    void RescheduleEvent(s_event* event, u64 new_time) {
        if (event->active) {
            RemoveEvent(event);
        }
        event->time = new_time;
        AddEvent(event);
    }

    void DoEvents() {
        s_event* first = queue.top();
        while (first->time <= *timer) {
            first->active = false;

            queue.pop();
            first->callback(first->caller, first, this);

            log_sched("Doing event at time %llx, now %x events in queue", first->time, queue.size());

            // new first element
            first = queue.top();
        }
        top = first->time;
    }

    ALWAYS_INLINE u64 PeekEvent() const {
        return top;
    }

    ALWAYS_INLINE bool ShouldDoEvents() const {
        return top < *timer;
    }
};

#endif //GC__SCHEDULER_H
