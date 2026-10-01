#ifndef FOURIER_ANDROID_NATIVE_WINDOW_OUTPUT_H
#define FOURIER_ANDROID_NATIVE_WINDOW_OUTPUT_H

#include "rgb24.h"

#include <android/native_window.h>
#include <stddef.h>

enum native_window_output_result {
    NATIVE_WINDOW_OUTPUT_OK = 0,
    NATIVE_WINDOW_OUTPUT_INVALID,
    NATIVE_WINDOW_OUTPUT_GEOMETRY_ERROR,
    NATIVE_WINDOW_OUTPUT_LOCK_ERROR,
    NATIVE_WINDOW_OUTPUT_FORMAT_ERROR,
    NATIVE_WINDOW_OUTPUT_BUFFER_TOO_SMALL,
    NATIVE_WINDOW_OUTPUT_CONVERSION_ERROR,
    NATIVE_WINDOW_OUTPUT_POST_ERROR
};

enum native_window_output_result android_window_present_rgb24(
    ANativeWindow *window, const struct rgb24 *pixels,
    size_t width, size_t height, size_t pixel_capacity);

const char *native_window_output_result_text(
    enum native_window_output_result result);

#endif
