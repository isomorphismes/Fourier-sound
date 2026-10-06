#include "native_window_output.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct ANativeWindow {
    int geometry_result;
    int lock_result;
    int post_result;
    int geometry_calls;
    int lock_calls;
    int post_calls;
    int32_t requested_width;
    int32_t requested_height;
    int32_t requested_format;
    ANativeWindow_Buffer buffer;
};

int32_t ANativeWindow_setBuffersGeometry(
    ANativeWindow *window, int32_t width, int32_t height, int32_t format)
{
    window->geometry_calls++;
    window->requested_width = width;
    window->requested_height = height;
    window->requested_format = format;
    return window->geometry_result;
}

int32_t ANativeWindow_lock(
    ANativeWindow *window, ANativeWindow_Buffer *out_buffer,
    ARect *in_out_dirty_bounds)
{
    (void)in_out_dirty_bounds;
    window->lock_calls++;
    if (window->lock_result) return window->lock_result;
    *out_buffer = window->buffer;
    return 0;
}

int32_t ANativeWindow_unlockAndPost(ANativeWindow *window)
{
    window->post_calls++;
    return window->post_result;
}

static struct ANativeWindow ready_window(unsigned char *bytes)
{
    struct ANativeWindow window = {0};
    window.buffer = (ANativeWindow_Buffer){
        .width = 2,
        .height = 2,
        .stride = 3,
        .format = WINDOW_FORMAT_RGBA_8888,
        .bits = bytes
    };
    return window;
}

static void success_and_stride(void)
{
    unsigned char bytes[24];
    memset(bytes, 0xcc, sizeof(bytes));
    struct ANativeWindow window = ready_window(bytes);
    struct rgb24 pixels[] = {
        {1, 2, 3}, {4, 5, 6},
        {7, 8, 9}, {10, 11, 12}
    };

    assert(android_window_present_rgb24(
        &window, pixels, 2U, 2U, 4U) == NATIVE_WINDOW_OUTPUT_OK);
    assert(window.geometry_calls == 1);
    assert(window.lock_calls == 1);
    assert(window.post_calls == 1);
    assert(window.requested_width == 2 && window.requested_height == 2);
    assert(window.requested_format == WINDOW_FORMAT_RGBA_8888);
    assert(bytes[0] == 1 && bytes[1] == 2 && bytes[2] == 3 && bytes[3] == 255);
    assert(bytes[4] == 4 && bytes[5] == 5 && bytes[6] == 6 && bytes[7] == 255);
    assert(bytes[8] == 0xcc && bytes[9] == 0xcc &&
           bytes[10] == 0xcc && bytes[11] == 0xcc);
    assert(bytes[12] == 7 && bytes[13] == 8 && bytes[14] == 9 && bytes[15] == 255);
}

static void platform_failures(void)
{
    unsigned char bytes[24] = {0};
    struct rgb24 pixels[4] = {0};

    struct ANativeWindow geometry = ready_window(bytes);
    geometry.geometry_result = -1;
    assert(android_window_present_rgb24(
        &geometry, pixels, 2U, 2U, 4U) == NATIVE_WINDOW_OUTPUT_GEOMETRY_ERROR);
    assert(geometry.lock_calls == 0 && geometry.post_calls == 0);

    struct ANativeWindow lock = ready_window(bytes);
    lock.lock_result = -2;
    assert(android_window_present_rgb24(
        &lock, pixels, 2U, 2U, 4U) == NATIVE_WINDOW_OUTPUT_LOCK_ERROR);
    assert(lock.lock_calls == 1 && lock.post_calls == 0);

    struct ANativeWindow format = ready_window(bytes);
    format.buffer.format = 99;
    assert(android_window_present_rgb24(
        &format, pixels, 2U, 2U, 4U) == NATIVE_WINDOW_OUTPUT_FORMAT_ERROR);
    assert(format.post_calls == 1);

    struct ANativeWindow small = ready_window(bytes);
    small.buffer.stride = 1;
    assert(android_window_present_rgb24(
        &small, pixels, 2U, 2U, 4U) == NATIVE_WINDOW_OUTPUT_BUFFER_TOO_SMALL);
    assert(small.post_calls == 1);

    struct ANativeWindow post = ready_window(bytes);
    post.post_result = -3;
    assert(android_window_present_rgb24(
        &post, pixels, 2U, 2U, 4U) == NATIVE_WINDOW_OUTPUT_POST_ERROR);
    assert(post.post_calls == 1);
}

static void invalid_arguments(void)
{
    unsigned char bytes[4] = {0};
    struct rgb24 pixel = {0};
    struct ANativeWindow window = ready_window(bytes);

    assert(android_window_present_rgb24(
        NULL, &pixel, 1U, 1U, 1U) == NATIVE_WINDOW_OUTPUT_INVALID);
    assert(android_window_present_rgb24(
        &window, NULL, 1U, 1U, 1U) == NATIVE_WINDOW_OUTPUT_INVALID);
    assert(android_window_present_rgb24(
        &window, &pixel, 0U, 1U, 1U) == NATIVE_WINDOW_OUTPUT_INVALID);
    assert(window.geometry_calls == 0);
}

int main(void)
{
    success_and_stride();
    platform_failures();
    invalid_arguments();
    puts("PASS native window geometry, lock, format, size, stride and post behavior");
    return 0;
}
