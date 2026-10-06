#ifndef FOURIER_RGB24_RGBA8888_H
#define FOURIER_RGB24_RGBA8888_H

#include "rgb24.h"

#include <stdbool.h>
#include <stddef.h>

/* Copy tightly packed RGB24 pixels into an RGBA8888 destination whose rows may
 * have platform padding. Padding pixels are left unchanged. */
bool rgb24_copy_rgba8888(const struct rgb24 *pixels,
                         size_t width, size_t height, size_t pixel_capacity,
                         void *destination, size_t stride_pixels,
                         size_t destination_height);

#endif
