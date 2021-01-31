#ifndef GC__SCHEDULER_H
#define GC__SCHEDULER_H

#include "default.h"
#include "flags.h"
#include "helpers.h"

#include <functional>
#include <queue>
#include <array>

#include "log.h"

#undef CHECK_EVENT_ACTIVE

#define SCHEDULER_MAX_EVENTS 64

struct s_scheduler;
struct s_event;

// returns whether CPU was affected
#define SCHEDULER_EVENT(name) bool name(void* caller, s_event* event, s_scheduler* scheduler)

struct s_event {
    constexpr s_event() :
        caller(nullptr),
        callback(nullptr) {

    }

    constexpr s_event(void* const _caller, SCHEDULER_EVENT((*_callback))) :
        caller(_caller),
        callback(_callback) {

    }

    constexpr s_event(void* const _caller, SCHEDULER_EVENT((*_callback)), i32 _time) :
        s_event(_caller, _callback) {
        time = _time;
    }

    SCHEDULER_EVENT((*callback));

    void* caller;
    i32 time = 0;
    bool active = false;   // signifies if event is in the scheduler or not
};

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

    void map(void (*f)(T*)) {
        for (auto it = this->c.begin(); it != this->c.end(); it++) {
            f(&(*it));
        }
    }
};

// we know that there is always _something_ in the queue
struct s_scheduler {
private:
    friend class Initializer;
    // event to put in queue to make sure it is not empty
    s_event infty = s_event(
        this,
        [] (void* caller, s_event* event, s_scheduler* scheduler) -> bool {
            *(scheduler->timer) -= 0x7000'0000;
            scheduler->queue.map([](s_event** value) {
                (*value)->time -= 0x7000'0000;
            });

            // schedule at 0x7000'0000 again
            scheduler->AddEvent(event);
            return false;
        },
        0x7000'0000
    );

    u32 number_of_events = 0;
    std::array<s_event, SCHEDULER_MAX_EVENTS> events = {};
    deleteable_priority_queue<s_event*, std::vector<s_event*>, decltype(cmp)> queue;

public:
    const static i32 TimeMask = 0x3fff'ffff;

    i32* const timer;
    i32 top;

    explicit s_scheduler(i32* _timer) :
            timer(_timer) {
        this->Reset();
    }

    constexpr s_event* MakeEvent(void* caller, SCHEDULER_EVENT((*callback))) {
        events[number_of_events] = s_event(caller, callback);
        return &events[number_of_events++];
    }

    constexpr s_event* MakeEvent(void* caller, SCHEDULER_EVENT((*callback)), i32 time) {
        events[number_of_events] = s_event(caller, callback, time);
        return &events[number_of_events++];
    }

    void Reset() {
        // clear queue (fill queue with initial event)
        queue = {};
        queue.push(&infty);
        top = 0x7000'0000;
    }

    void AddEvent(s_event* event) {
        if (likely(!event->active)) {
            event->active = true;
            queue.push(event);
#ifdef min
            top = min(top, event->time);
#else
            top = std::min(top, event->time);
#endif
        }
    }

    void AddEventAfter(s_event* event, u64 dt) {
        if (likely(!event->active)) {
            event->time = *timer + dt;
            event->active = true;
            queue.push(event);
#ifdef min
            top = min(top, event->time);
#else
            top = std::min(top, event->time);
#endif
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

    bool DoEvents() {
        s_event* first = queue.top();
        bool cpu_affected = false;
        while (first->time <= *timer) {
            first->active = false;

            queue.pop();
            cpu_affected |= first->callback(first->caller, first, this);

            log_sched("Doing event at time %llx, now %x events in queue", first->time, queue.size());

            // new first element
            first = queue.top();
        }
        top = first->time;
        return cpu_affected;
    }

    ALWAYS_INLINE u64 PeekEvent() const {
        return top;
    }

    ALWAYS_INLINE bool ShouldDoEvents() const {
        return top <= *timer;
    }
};

#endif //GC__SCHEDULER_H
