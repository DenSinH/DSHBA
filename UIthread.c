#include "UIthread.h"
#include "src/Frontend/interface.h"

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>

static DWORD WINAPI UI_thread(LPVOID _) {
    ui_run();
    return 0;
}

static HANDLE ui_thread;

void start_ui_thread() {
    ui_thread = CreateThread(NULL, 0, UI_thread, NULL, 0, NULL);
}

void join_ui_thread() {
    while (WaitForSingleObject(ui_thread, 50) != WAIT_OBJECT_0) {
        printf("Joining thread failed, trying again...\n");
    }
}

#else

#include <pthread.h>

void* UI_thread(void* arg) {
    ui_run();
    return NULL;
}

static pthread_t ui_thread;

void start_ui_thread() {
    pthread_create(&ui_thread, NULL, UI_thread, NULL);
}

void join_ui_thread() {
    pthread_join(ui_thread, NULL);
}

#endif