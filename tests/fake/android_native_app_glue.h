#ifndef FOURIER_TEST_APP_GLUE_H
#define FOURIER_TEST_APP_GLUE_H
#include <stdint.h>
#include <android/native_window.h>
typedef struct ANativeActivity { int unused; } ANativeActivity;
typedef struct ALooper { int unused; } ALooper;
struct android_app;
struct android_poll_source {
    void (*process)(struct android_app *, struct android_poll_source *);
};
struct android_app {
    void *userData;
    void (*onAppCmd)(struct android_app *, int32_t);
    ANativeActivity *activity;
    ALooper *looper;
    ANativeWindow *window;
    int destroyRequested;
};
enum {
    LOOPER_ID_USER = 3, ALOOPER_EVENT_INPUT = 1, ALOOPER_POLL_ERROR = -4,
    APP_CMD_INIT_WINDOW = 1, APP_CMD_TERM_WINDOW, APP_CMD_RESUME,
    APP_CMD_GAINED_FOCUS, APP_CMD_LOST_FOCUS, APP_CMD_PAUSE
};
int ALooper_pollOnce(int timeout_ms, int *fd, int *events, void **data);
int ALooper_removeFd(ALooper *looper, int fd);
int ALooper_addFd(ALooper *looper, int fd, int ident, int events,
                  int (*callback)(int, int, void *), void *data);
void ANativeActivity_finish(ANativeActivity *activity);
#endif
