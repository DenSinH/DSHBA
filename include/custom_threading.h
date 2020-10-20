#ifndef GC__CUSTOM_THREADING_H
#define GC__CUSTOM_THREADING_H

#include "default.h"
#include <stdbool.h>

#ifdef _WIN32

#include "windows.h"

typedef HANDLE mutex;
typedef HANDLE wait_event;

#else
#error "unimplemented: thread syncing"
#endif

void create_mutex(mutex* m, bool owned);
bool owns_mutex(mutex* m);
void acquire_mutex(mutex* m);
void release_mutex(mutex* m);

void create_wait_event(wait_event* e, bool signaled);
void wait_for_event(wait_event* e);
void wait_for_event_timeout(wait_event* e, u32 timeout);
bool is_wait_event_set(wait_event* e) ;
void set_wait_event(wait_event* e);
void clear_wait_event(wait_event* e);

#endif //GC__CUSTOM_THREADING_H
