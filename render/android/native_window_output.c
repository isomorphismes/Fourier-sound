#include "native_window_output.h"

#include "rgb24_rgba8888.h"

#include <limits.h>

const char *native_window_output_result_text(
    enum native_window_output_result result)
{
    switch (result) {
    case NATIVE_WINDOW_OUTPUT_OK: return "presented";
    case NATIVE_WINDOW_OUTPUT_INVALID: return "invalid argument";
    case NATIVE_WINDOW_OUTPUT_GEOMETRY_ERROR: return "buffer geometry failed";
    case NATIVE_WINDOW_OUTPUT_LOCK_ERROR: return "window lock failed";
    case NATIVE_WINDOW_OUTPUT_FORMAT_ERROR: return "unexpected buffer format";
    case NATIVE_WINDOW_OUTPUT_BUFFER_TOO_SMALL: return "window buffer too small";
    case NATIVE_WINDOW_OUTPUT_CONVERSION_ERROR: return "pixel conversion failed";
    case NATIVE_WINDOW_OUTPUT_POST_ERROR: return "window post failed";
    }
    return "unknown window output result";
}

enum native_window_output_result android_window_present_rgb24(
    ANativeWindow *window, const struct rgb24 *pixels,
    size_t width, size_t height, size_t pixel_capacity)
{
    if (!window || !pixels || !width || !height ||
        width > INT32_MAX || height > INT32_MAX)
        return NATIVE_WINDOW_OUTPUT_INVALID;

    if (ANativeWindow_setBuffersGeometry(
            window, (int32_t)width, (int32_t)height,
            WINDOW_FORMAT_RGBA_8888) != 0)
        return NATIVE_WINDOW_OUTPUT_GEOMETRY_ERROR;

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window, &buffer, NULL) != 0)
        return NATIVE_WINDOW_OUTPUT_LOCK_ERROR;

    enum native_window_output_result result = NATIVE_WINDOW_OUTPUT_OK;
    if (buffer.format != WINDOW_FORMAT_RGBA_8888) {
        result = NATIVE_WINDOW_OUTPUT_FORMAT_ERROR;
    } else if (buffer.width < (int32_t)width ||
               buffer.height < (int32_t)height ||
               buffer.stride < (int32_t)width) {
        result = NATIVE_WINDOW_OUTPUT_BUFFER_TOO_SMALL;
    } else if (!rgb24_copy_rgba8888(
                   pixels, width, height, pixel_capacity,
                   buffer.bits, (size_t)buffer.stride,
                   (size_t)buffer.height)) {
        result = NATIVE_WINDOW_OUTPUT_CONVERSION_ERROR;
    }

    if (ANativeWindow_unlockAndPost(window) != 0)
        return NATIVE_WINDOW_OUTPUT_POST_ERROR;
    return result;
}
