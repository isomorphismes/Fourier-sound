#ifndef FOURIER_PPM_H
#define FOURIER_PPM_H

#include "rgb24.h"

#include <stdbool.h>
#include <stddef.h>

bool rgb24_write_ppm(const char *path, const struct rgb24 *pixels,
                     size_t width, size_t height, size_t capacity);

#endif
