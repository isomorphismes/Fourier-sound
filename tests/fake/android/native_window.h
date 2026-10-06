#ifndef TEST_FAKE_ANDROID_NATIVE_WINDOW_H
#define TEST_FAKE_ANDROID_NATIVE_WINDOW_H

#include <stdint.h>

typedef struct ANativeWindow ANativeWindow;

typedef struct ARect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} ARect;

typedef struct ANativeWindow_Buffer {
    int32_t width;
    int32_t height;
    int32_t stride;
    int32_t format;
    void *bits;
    uint32_t reserved[6];
} ANativeWindow_Buffer;

enum { WINDOW_FORMAT_RGBA_8888 = 1 };

int32_t ANativeWindow_setBuffersGeometry(
    ANativeWindow *window, int32_t width, int32_t height, int32_t format);
int32_t ANativeWindow_lock(
    ANativeWindow *window, ANativeWindow_Buffer *out_buffer, ARect *in_out_dirty_bounds);
int32_t ANativeWindow_unlockAndPost(ANativeWindow *window);

#endif
