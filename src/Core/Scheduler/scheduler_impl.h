#ifndef DSHBA_SCHEDULER_IMPL_H
#define DSHBA_SCHEDULER_IMPL_H

#include "scheduler.h"

#include <functional>
#include <queue>

#include "log.h"

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
            return false;
        }
    };
};

struct s_scheduler {

    deleteable_priority_queue<s_event*, std::vector<s_event*>, decltype(cmp)> queue;
    u64* timer;

    explicit s_scheduler(u64* timer) {
        this->timer = timer;
    }

    void AddEvent(s_event* event) {
        if (!event->active) {
            event->active = true;
            queue.push(event);
        }
    }

    void RemoveEvent(s_event* event) {
        queue.remove(event);
        event->active = false;
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
        while (!this->queue.empty() && first->time <= *timer) {
            first->active = false;

            queue.pop();
            first->callback(first->caller, first, this);

            log_sched("Doing event at time %llx, now %x events in queue", first->time, queue.size());

            // new first element
            first = queue.top();
        }
    }

    u64 PeekEvent() const {
        if (queue.empty()) {
            return 0;
        }
        return queue.top()->time;
    }

    bool ShouldDoEvents() const {
        return !queue.empty() && queue.top()->time < *timer;
    }
};

#endif //DSHBA_SCHEDULER_IMPL_H
